#include "hanabi/conventions/reactor/interpret_reaction.h"

#include <algorithm>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"

namespace hanabi::reactor {

int calc_slot(int focus_slot, int slot, int hand_size) {
  int other = (focus_slot + hand_size - slot) % hand_size;
  return other == 0 ? hand_size : other;
}

namespace {

// Compute (react_slot, target_slot) for the reacter's played/discarded order.
// Returns nullopt if the mapping fails (order not in reacter's prev hand or
// target slot out of range).
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
  IdentitySet critical = prev.state.critical_set;
  IdentitySet new_inferred = game.common.thoughts[order].inferred.filter(
      [&](Identity i) { return !critical.contains(i); });
  game.with_thought(order, [&](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = new_inferred;
    return out;
  });
  int turn = game.state.turn_count;
  int giver = wc.giver;
  bool is_empty = new_inferred.is_empty();
  game.with_meta(order, [turn, giver, is_empty](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.by = giver;
    m.trash = is_empty;
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
  if (wc.clue.kind == ClueKind::COLOUR) {
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
  if (wc.all_plays || wc.clue.kind == ClueKind::RANK) {
    target_i_play(prev, game, wc, target_slot);
    elim_play_play(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  } else {
    target_i_discard(prev, game, wc, target_slot);
    elim_play_dc(prev.state, game, wc.receiver_hand, wc.reacter, wc.focus_slot, target_slot);
  }
  return false;
}

}  // namespace hanabi::reactor
