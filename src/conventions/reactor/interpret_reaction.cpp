#include "hanabi/conventions/reactor/interpret_reaction.h"

#include <algorithm>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor {

int calc_slot(int focus_slot, int slot, int hand_size) {
  int other = (focus_slot + hand_size - slot) % hand_size;
  return other == 0 ? hand_size : other;
}

// Compute (react_slot, target_slot) for the reacter's played/discarded order.
// Returns nullopt if the mapping fails (order not in reacter's prev hand or
// target slot out of range). Exported: reactor0's reaction resolution uses
// the same mapping with a clue-value anchor in wc.focus_slot.
std::optional<std::pair<int, int>> calc_target_slot(const Game& prev, const Game& game,
                                                       int order, const ReactorWC& wc) {
  const auto& prev_hand = prev.state.hands[wc.reacter];
  auto it = std::find(prev_hand.begin(), prev_hand.end(), order);
  if (it == prev_hand.end()) return std::nullopt;
  int react_slot = static_cast<int>(it - prev_hand.begin()) + 1;
  int target_slot = calc_slot(wc.focus_slot, react_slot,
                                kHandSize[prev.state.num_players]);
  if (target_slot < 1 || target_slot > static_cast<int>(wc.receiver_hand.size())) {
    return std::nullopt;
  }
  int receive_order = wc.receiver_hand[target_slot - 1];
  const auto& cur_hand = game.state.hands[wc.receiver];
  if (std::find(cur_hand.begin(), cur_hand.end(), receive_order) == cur_hand.end()) {
    return std::nullopt;
  }
  return std::make_pair(react_slot, target_slot);
}

namespace {

// Mark receive_order as trash in game.meta if its inferred set is empty.
void mark_trash_if_empty(Game& game, int receive_order) {
  if (game.common.thoughts[receive_order].inferred.is_empty()) {
    game.meta[receive_order].trash = true;
  }
}

}  // namespace

void target_i_discard(const Game& prev, Game& game, const ReactorWC& wc,
                       int target_slot) {
  int order = wc.receiver_hand[target_slot - 1];
  const State& state = game.state;

  // "Press Discard" is a PHYSICAL instruction, and on an INVERTED (Orange /
  // Dark Orange) suit that button is the CHUCK — a play attempt that advances
  // the stack. The discard semantics below are exactly backwards for it:
  // narrowing to the NON-CRITICAL ids asks "which of these can you afford to
  // throw away?", and in Dark Orange — every rank `oneOfEach`, so every card
  // critical — that empties the set outright. The card is then marked trash,
  // and `Game::elim`'s step-1 sweep (src/basics/game.cpp:498-509) sees the
  // empty `inferred`, resets the card to its global empathy and clears the
  // status, so the chuck signal is destroyed the moment it is given.
  // bug_report_6_2_0.txt, replay 1959065 T5-T6.
  //
  // reactor0's stable side already knows this trap and refuses to reuse
  // `reactor::target_discard` for precisely this reason
  // (reactor0/interpret_clue.cpp:139-142, `stamp_orange_chuck`). The
  // resolution side never learned it. When the ordinary reading self-destructs
  // on a card the holder knows is orange, narrow to what a chuck would
  // actually stack instead, and never call the result trash.
  //
  // The "is it orange?" test is gated on COMMON knowledge so every seat
  // computes the same reading. The receiver cannot see their own card, and
  // `variants::target_is_inverted` reads `state.deck`, which is nullopt for the
  // card's own holder — branching on that here would desync the table.
  //
  // The ordinary reading runs FIRST and is kept verbatim whenever it yields
  // anything at all, so this change is confined to the case that destroys
  // itself: Dark Orange always hits it, an ordinary suit essentially never
  // does, and a non-dark orange with a spare copy keeps its old reading.
  IdentitySet critical = prev.state.critical_set;
  IdentitySet new_inferred = game.common.thoughts[order].inferred.filter(
      [&](Identity i) { return !critical.contains(i); });

  bool chuck = false;
  if (new_inferred.is_empty()) {
    const IdentitySet& poss = game.common.thoughts[order].possible;
    const bool holder_knows_inverted =
        poss.non_empty() &&
        poss.forall([&](Identity i) { return variants::is_inverted_id(state, i); });
    if (holder_knows_inverted) {
      chuck = true;
      new_inferred = game.common.thoughts[order].inferred.filter([&](Identity i) {
        return variants::is_inverted_id(state, i) && state.is_playable(i);
      });
    }
  }

  const bool is_empty = new_inferred.is_empty();
  if (chuck && is_empty) {
    // Nothing this card could be would reach a stack, so there is nothing to
    // promise — but do NOT empty `inferred` to say so, because that trips the
    // reset above and voids the call we are stamping. An empty narrowing
    // teaches nothing. (`target_i_play` was hardened the same way below.)
  } else {
    game.with_thought(order, [&](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      out.inferred = new_inferred;
      if (chuck) {
        // Pin it, as `stamp_orange_chuck` does. This is what lets
        // `decide.cpp:889-899` resolve the card to a single playable inverted
        // identity and dispatch the chuck as a `PerformDiscard`.
        out.info_lock = std::optional<IdentitySet>{new_inferred};
      }
      return out;
    });
  }
  int turn = game.state.turn_count;
  int giver = wc.giver;
  // A chuck is a play call, so it is never "revealed trash". Only the ordinary
  // discard reading can conclude that, and only when it found nothing safe.
  const bool mark_trash = !chuck && is_empty;
  game.with_meta(order, [turn, giver, mark_trash](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.by = giver;
    m.trash = mark_trash;
    m = m.reason(turn).signal(turn);
  });
}

void target_i_play(const Game& /*prev*/, Game& game, const ReactorWC& wc,
                    int target_slot) {
  const State& state = game.state;
  int order = wc.receiver_hand[target_slot - 1];
  IdentitySet self_playables = state.playable_set;
  for (int o : game.common.obvious_playables(game, state.holder_of(order))) {
    for (Identity inf : game.common.thoughts[o].inferred) {
      if (auto nxt = inf.next()) self_playables = self_playables.add(*nxt);
    }
  }
  IdentitySet new_inferred = game.common.thoughts[order].inferred.intersect(self_playables);
  // Stamp CTP unconditionally — the convention's "play this slot" signal must
  // reach the reacter even when no currently-playable identity overlaps the
  // card's inferred set (e.g. a delayed-play chain where the prerequisite
  // hasn't fired yet). Only narrow `inferred` / `info_lock` when there's a
  // non-empty intersection, so we don't leave the card in an empty-inferred
  // state that elim would later sweep.
  if (!new_inferred.is_empty()) {
    game.with_thought(order, [&](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      out.inferred = new_inferred;
      out.info_lock = std::optional<IdentitySet>{new_inferred};
      return out;
    });
  }
  int turn = state.turn_count;
  int giver = wc.giver;
  game.with_meta(order, [turn, giver](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.by = giver;
    m.focused = true;
    m = m.reason(turn).signal(turn);
  });
}

// --- elim helpers --------------------------------------------------------

void elim_play_play(const State& prev_state, Game& game,
                     const std::vector<int>& receiver_hand,
                     int reacter, int focus_slot, int target_slot) {
  int hand_size = kHandSize[prev_state.num_players];
  for (int i = 0; i < target_slot - 1; ++i) {
    if (i >= static_cast<int>(receiver_hand.size())) break;
    int receive_order = receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    int react_slot = calc_slot(focus_slot, i + 1, hand_size);
    if (status == CardStatus::CALLED_TO_PLAY || status == CardStatus::CALLED_TO_DISCARD) continue;
    if (react_slot < 1 || react_slot > static_cast<int>(prev_state.hands[reacter].size())) continue;
    int react_order = prev_state.hands[reacter][react_slot - 1];
    IdentitySet intersect = game.common.thoughts[react_order].possible.intersect(prev_state.playable_set);
    if (intersect.length() == 0) continue;
    if (intersect.length() == 1) {
      Identity id = intersect.head();
      IdentitySet ps = prev_state.playable_set;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.filter(
            [&](Identity iid) { return !ps.contains(iid) || iid == id; });
        return out;
      });
    } else {
      IdentitySet ps = prev_state.playable_set;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ps);
        return out;
      });
    }
    mark_trash_if_empty(game, receive_order);
  }
}

void elim_play_dc(const State& prev_state, Game& game,
                   const std::vector<int>& receiver_hand,
                   int reacter, int focus_slot, int target_slot) {
  int hand_size = kHandSize[prev_state.num_players];
  // First run elim_play_play across all slots.
  elim_play_play(prev_state, game, receiver_hand, reacter, focus_slot,
                  static_cast<int>(receiver_hand.size()) + 1);

  for (int i = 0; i < target_slot - 1; ++i) {
    if (i >= static_cast<int>(receiver_hand.size())) break;
    int receive_order = receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    int react_slot = calc_slot(focus_slot, i + 1, hand_size);
    int target_card = (target_slot - 1 < static_cast<int>(receiver_hand.size()))
                          ? receiver_hand[target_slot - 1]
                          : -1;
    bool skip = status == CardStatus::CALLED_TO_PLAY ||
                status == CardStatus::CALLED_TO_DISCARD ||
                (target_card != -1 && prev_state.deck[target_card].clued &&
                  !prev_state.deck[receive_order].clued);
    if (skip) continue;
    if (react_slot < 1 || react_slot > static_cast<int>(prev_state.hands[reacter].size())) continue;
    int react_order = prev_state.hands[reacter][react_slot - 1];
    bool can_elim = game.meta[react_order].status != CardStatus::CALLED_TO_PLAY &&
                     game.common.thoughts[react_order].possible.exists(
                         [&](Identity i) { return prev_state.is_playable(i); });
    if (can_elim) {
      IdentitySet ts = prev_state.trash_set;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ts);
        return out;
      });
    }
  }
}

// Decide the fate of a held receiver-chuck inference, or leave it held.
//
// The inference is owed only if the receiver's called card was a real DISCARD.
// It was not, if the card was an INVERTED identity that was playable at the time
// the reactive clue was given -- then the chuck put it on its stack, the walk
// passed over nothing, and nothing is owed. So the question is exactly "was it
// the playable orange", asked against the clue-time snapshot rather than the
// current stacks (they can differ by several turns of stack movement, and it is
// the clue-time reading the convention spoke about).
//
// Judged from THIS SEAT'S knowledge, deliberately, not from common knowledge.
// Every player except the receiver can see the card outright, so they know the
// answer at reaction time and should have the correct inference immediately;
// the receiver falls back to empathy and catches up when they work it out.
// `state.deck[o].id()` is already that accessor -- populated for a card this
// seat can see, `nullopt` for its own -- so no new machinery is needed, and the
// revealed-on-discard case falls out for free.
//
// Three outcomes: every reading says it was a play (void it), no reading says so
// (apply it), or it is still open (keep waiting). The card need NOT be discarded
// for this to fire -- knowing is enough.
void resolve_pending_dc_elim(Game& game) {
  Game::PendingDcElim& p = game.pending_dc_elim;
  if (!p.active) return;
  if (p.target_order < 0 ||
      p.target_order >= static_cast<int>(game.state.deck.size())) {
    p = Game::PendingDcElim{};
    return;
  }

  IdentitySet live;
  if (auto id = game.state.deck[p.target_order].id()) {
    live = IdentitySet::single(*id);
  } else {
    const Thought& t = game.common.thoughts[p.target_order];
    live = t.inferred.non_empty() ? t.inferred : t.possible;
  }
  if (live.is_empty()) return;  // nothing to reason from yet

  const State& s = game.state;
  IdentitySet playable = p.playable;
  auto was_play = [&s, playable](Identity i) {
    return variants::is_inverted_id(s, i) && playable.contains(i);
  };

  if (live.forall(was_play)) {
    // The chuck was a play. The inference was never owed.
    p = Game::PendingDcElim{};
    return;
  }
  if (!live.exists(was_play)) {
    // The chuck was a discard, so the walk really did pass those slots over.
    apply_pending_dc_elim(game);
    p = Game::PendingDcElim{};
  }
  // Otherwise still open -- hold it and ask again on the next information.
}

void apply_pending_dc_elim(Game& game) {
  const Game::PendingDcElim& p = game.pending_dc_elim;
  if (!p.active) return;

  // elim_play_play's half: a slot the walk passed over is not playable. When
  // the reacter's paired card had exactly one playable identity, that identity
  // is kept -- it is the one the pairing would have named.
  //
  // Only the slots BEFORE the target, deliberately -- narrower than the
  // immediate `elim_play_dc` / `elim_dc_dc`, which run this over the whole hand.
  // Widening it to match was tried in v7.21.0 and is wrong: by the time a
  // deferred inference applies, the receiver may have been told something
  // better about the later slots, and the blanket "not playable" then overwrites
  // it. Replay 1966675 T26 is the case -- order 7 came out of it with common
  // knowledge of {o4} for a card that is really r4, contradicting both the
  // holder's own view and the truth.
  //
  // Snapshotting the reacter's evidence at reaction time does NOT rescue the
  // wider scope; it was tried too. The scope is the problem, not the staleness.
  for (int i = 0; i < p.target_slot - 1; ++i) {
    if (i >= static_cast<int>(p.receiver_hand.size())) break;
    int receive_order = p.receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    if (status == CardStatus::CALLED_TO_PLAY ||
        status == CardStatus::CALLED_TO_DISCARD) {
      continue;
    }
    int react_slot = calc_slot(p.focus_slot, i + 1, p.hand_size);
    if (react_slot < 1 || react_slot > static_cast<int>(p.reacter_hand.size())) {
      continue;
    }
    int react_order = p.reacter_hand[react_slot - 1];
    IdentitySet intersect =
        game.common.thoughts[react_order].possible.intersect(p.playable);
    if (intersect.length() == 0) continue;
    if (intersect.length() == 1) {
      Identity id = intersect.head();
      IdentitySet ps = p.playable;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.filter(
            [&](Identity iid) { return !ps.contains(iid) || iid == id; });
        return out;
      });
    } else {
      IdentitySet ps = p.playable;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ps);
        return out;
      });
    }
    mark_trash_if_empty(game, receive_order);
  }

  // The owning elim's own half: the trash differencing, with its skip rules.
  // The two differ only in the gate -- `elim_play_dc` asks whether the paired
  // reacter card could have been playable, `elim_dc_dc` whether it is not
  // all-critical.
  for (int i = 0; i < p.target_slot - 1; ++i) {
    if (i >= static_cast<int>(p.receiver_hand.size())) break;
    int receive_order = p.receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    int react_slot = calc_slot(p.focus_slot, i + 1, p.hand_size);
    const bool was_clued = i < static_cast<int>(p.receiver_was_clued.size()) &&
                           p.receiver_was_clued[i] != 0;
    bool skip = status == CardStatus::CALLED_TO_PLAY ||
                status == CardStatus::CALLED_TO_DISCARD ||
                (p.target_was_clued && !was_clued);
    if (skip) continue;
    if (react_slot < 1 || react_slot > static_cast<int>(p.reacter_hand.size())) {
      continue;
    }
    int react_order = p.reacter_hand[react_slot - 1];
    IdentitySet ps = p.playable;
    const bool can_elim =
        p.kind == Game::PendingDcElim::Kind::PlayDc
            ? (game.meta[react_order].status != CardStatus::CALLED_TO_PLAY &&
               game.common.thoughts[react_order].possible.exists(
                   [&](Identity id) { return ps.contains(id); }))
            : !game.common.thoughts[react_order].possible.forall(
                  [&](Identity id) { return p.critical.contains(id); });
    if (can_elim) {
      IdentitySet ts = p.trash;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ts);
        return out;
      });
    }
  }
}

void elim_dc_play(const State& prev_state, Game& game,
                   const std::vector<int>& receiver_hand,
                   int reacter, int focus_slot, int target_slot) {
  int hand_size = kHandSize[prev_state.num_players];
  for (int i = 0; i < target_slot - 1; ++i) {
    if (i >= static_cast<int>(receiver_hand.size())) break;
    int receive_order = receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    int react_slot = calc_slot(focus_slot, i + 1, hand_size);
    if (status == CardStatus::CALLED_TO_PLAY || status == CardStatus::CALLED_TO_DISCARD) continue;
    if (react_slot < 1 || react_slot > static_cast<int>(prev_state.hands[reacter].size())) continue;
    int react_order = prev_state.hands[reacter][react_slot - 1];
    if (!game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return prev_state.is_critical(i); })) {
      IdentitySet ps = prev_state.playable_set;
      game.with_thought(receive_order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ps);
        return out;
      });
      mark_trash_if_empty(game, receive_order);
    }
  }
}

void elim_dc_dc(const State& prev_state, Game& game,
                 const std::vector<int>& receiver_hand,
                 int reacter, int focus_slot, int target_slot) {
  int hand_size = kHandSize[prev_state.num_players];
  // First run elim_play_play across all slots.
  elim_play_play(prev_state, game, receiver_hand, reacter, focus_slot,
                  static_cast<int>(receiver_hand.size()) + 1);
  for (int i = 0; i < target_slot - 1; ++i) {
    if (i >= static_cast<int>(receiver_hand.size())) break;
    int receive_order = receiver_hand[i];
    CardStatus status = game.meta[receive_order].status;
    int react_slot = calc_slot(focus_slot, i + 1, hand_size);
    int target_card = (target_slot - 1 < static_cast<int>(receiver_hand.size()))
                          ? receiver_hand[target_slot - 1]
                          : -1;
    bool skip = status == CardStatus::CALLED_TO_PLAY ||
                status == CardStatus::CALLED_TO_DISCARD ||
                (target_card != -1 && prev_state.deck[target_card].clued &&
                  !prev_state.deck[receive_order].clued);
    if (skip) continue;
    if (react_slot < 1 || react_slot > static_cast<int>(prev_state.hands[reacter].size())) continue;
    int react_order = prev_state.hands[reacter][react_slot - 1];
    if (game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return prev_state.is_critical(i); })) {
      continue;
    }
    IdentitySet ts = prev_state.trash_set;
    game.with_thought(receive_order, [&](const Thought& t) {
      Thought out = t;
      out.inferred = t.inferred.difference(ts);
      return out;
    });
  }
}

// --- react_discard / react_play -----------------------------------------

bool react_discard(const Game& prev, Game& game, int player_index, int order,
                    const ReactorWC& wc) {
  hanabi::instr::ScopedTimer st("reactor.react_discard");
  hanabi::logging::LogScope ls(
      "reactor.react_discard",
      {{"player_index", player_index}, {"order", order}, {"reacter", wc.reacter}});
  if (player_index != wc.reacter) {
    game.with_move(DiscardInterp::NONE);
    return false;
  }

  if (wc.inverted) {
    // Response-inversion: were we expecting them to play but they discarded?
    auto prev_obvious_playables = prev.common.obvious_playables(game, wc.reacter);
    bool unnatural;
    if (!prev_obvious_playables.empty()) {
      unnatural = true;
    } else {
      auto known_trash = prev.common.thinks_trash(prev, wc.reacter);
      if (known_trash.empty()) {
        auto chop = prev.chop(wc.reacter);
        unnatural = !chop || *chop != order;
      } else {
        unnatural = std::find(known_trash.begin(), known_trash.end(), order) ==
                     known_trash.end();
      }
    }
    if (unnatural) {
      // The reacter's discard does not match what a stable interpretation
      // of the previous clue predicted — rewind and re-interpret that clue
      // as reactive. If the rewind succeeds, the replayed game has already
      // processed the current discard end-to-end (with_move + elim included)
      // so we must NOT call with_move again here.
      try {
        Game rewound =
            game.rewind(wc.turn, InterpAction{ClueInterp::REACTIVE});
        game = std::move(rewound);
        return true;
      } catch (const std::exception&) {
        // Rewind couldn't proceed (depth limit, etc.); leave game alone.
      }
      game.with_move(DiscardInterp::NONE);
      return false;
    }
    game.with_move(DiscardInterp::NONE);
    return false;
  }

  auto slots = calc_target_slot(prev, game, order, wc);
  if (!slots) {
    game.with_move(DiscardInterp::NONE);
    return false;
  }
  auto [react_slot, target_slot] = *slots;
  (void)react_slot;
  if (wc.all_plays) {
    // /allplays on set up a play+play expectation; the reacter discarding
    // instead is a convention deviation. Don't apply any further marks.
    game.with_move(DiscardInterp::NONE);
    return false;
  }
  if (!variants::uses_even_parity(*prev.state.variant, wc.clue.kind)) {
    target_i_play(prev, game, wc, target_slot);
    elim_dc_play(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  } else {
    target_i_discard(prev, game, wc, target_slot);
    elim_dc_dc(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  }
  game.with_move(DiscardInterp::NONE);
  return false;
}

bool react_play(const Game& prev, Game& game, int player_index, int order,
                 const ReactorWC& wc) {
  hanabi::instr::ScopedTimer st("reactor.react_play");
  hanabi::logging::LogScope ls(
      "reactor.react_play",
      {{"player_index", player_index}, {"order", order}, {"reacter", wc.reacter}});
  if (player_index != wc.reacter) return false;

  if (wc.inverted) {
    auto known_playables = prev.common.obvious_playables(prev, wc.reacter);
    if (known_playables.empty()) {
      known_playables = prev.players[wc.reacter].thinks_playables(prev, wc.reacter);
    }
    bool ok = std::find(known_playables.begin(), known_playables.end(), order) !=
              known_playables.end();
    if (!ok) {
      // The reacter played an unexpected card (one we hadn't already
      // identified as a known play). Under response-inversion, that's the
      // signal that the prior clue should be interpreted reactively rather
      // than stably — rewind to that turn and re-interpret. On success the
      // replay already handled the current play, so signal the caller to
      // skip its remaining post-react work (with_move, elim).
      try {
        Game rewound =
            game.rewind(wc.turn, InterpAction{ClueInterp::REACTIVE});
        game = std::move(rewound);
        return true;
      } catch (const std::exception&) {
        // Depth limit or other failure: keep the stable interpretation.
      }
    }
    return false;
  }

  auto slots = calc_target_slot(prev, game, order, wc);
  if (!slots) return false;
  auto [react_slot, target_slot] = *slots;
  (void)react_slot;
  // /allplays promotes COLOR reactives to play+play, matching the standard
  // RANK behavior (target_i_play + elim_play_play).
  // `all_plays` is a reactor toggle orthogonal to the variant, so it stays as
  // its own term; only the kind test moves to the parity predicate.
  if (wc.all_plays || variants::uses_even_parity(*prev.state.variant, wc.clue.kind)) {
    target_i_play(prev, game, wc, target_slot);
    elim_play_play(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  } else {
    target_i_discard(prev, game, wc, target_slot);
    elim_play_dc(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  }
  return false;
}

}  // namespace hanabi::reactor
