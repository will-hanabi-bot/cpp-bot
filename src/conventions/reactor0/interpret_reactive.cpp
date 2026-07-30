#include "hanabi/conventions/reactor0/interpret_reactive.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;
using hanabi::reactor::calc_slot;
using hanabi::reactor::delayed_plays;
using hanabi::reactor::effective_possible_for;
using hanabi::reactor::target_discard;
using hanabi::reactor::target_play;

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

// (order, 0-based hand index) pairs, slot-ascending (leftmost first).
using SlotList = std::vector<std::pair<int, int>>;

// The reactor0 play pool: slots ascending, every card whose actual identity
// is playable on the PRE-CLUE stacks — including cards already CTP'd (the
// convention deliberately re-targets them; the spec's "whether or not it
// has already been called to play"). No dupe-primary reordering, no
// hypo-advance through the receiver's queue. POV: deck ids of the
// receiver's hand are visible to the giver and the reacter; the receiver
// never runs selection (interpret_reactive returns early for them).
SlotList play_pool(const Game& prev, const Game& game, int receiver) {
  SlotList pool;
  const State& state = game.state;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    auto id = state.deck[o].id();
    if (id && prev.state.is_playable(*id)) {
      pool.emplace_back(o, static_cast<int>(i));
    }
  }
  return pool;
}

struct DcTarget {
  int order = -1;
  int index = -1;  // 0-based hand index
  bool lock = false;
};

// The reactor0 dc-target pool: slots ascending, every card whose actual
// identity is basic trash OR duplicated elsewhere in the same hand. Cards
// already CTD'd are skipped (re-CTD conveys nothing), as are inverted
// (orange) cards — a CTD on orange is a chuck-as-play-attempt and strikes
// on trash. If the pool is empty: with rlocks on, the single candidate is
// the OLDEST slot with lock=true (the reactive-lock reading); with rlocks
// off, the reactor sacrifice ordering applies. A trash candidate that
// happens to sit on the oldest slot is also lock=true under rlocks — the
// receiver cannot tell trash-on-slot-5 apart from the lock signal and must
// take the conservative reading, so the giver must account for it too.
std::vector<DcTarget> dc_candidates(const Game& prev, const Game& game,
                                    int receiver, bool rlocks) {
  const State& state = game.state;
  const auto& hand = state.hands[receiver];
  int oldest_index = static_cast<int>(hand.size()) - 1;

  std::vector<DcTarget> pool;
  for (size_t i = 0; i < hand.size(); ++i) {
    int o = hand[i];
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    auto id = state.deck[o].id();
    if (!id) continue;
    if (variants::is_inverted_id(state, *id)) continue;
    bool trash = state.is_basic_trash(*id);
    bool dupe = false;
    if (!trash) {
      for (int o2 : hand) {
        if (o2 == o) continue;
        auto id2 = state.deck[o2].id();
        if (id2 && *id2 == *id) {
          dupe = true;
          break;
        }
      }
    }
    if (trash || dupe) {
      bool lock = rlocks && static_cast<int>(i) == oldest_index;
      pool.push_back(DcTarget{o, static_cast<int>(i), lock});
    }
  }
  if (!pool.empty()) return pool;

  if (rlocks) {
    if (oldest_index < 0) return {};
    return {DcTarget{hand[oldest_index], oldest_index, /*lock=*/true}};
  }
  // rlocks off: the receiver's hand is all good/unique/unplayable —
  // sacrifice, using reactor's ordering.
  auto prev_kt = prev.common.thinks_trash(prev, receiver);
  std::vector<DcTarget> sac;
  for (const auto& [o, i] :
       hanabi::reactor::sacrifice_targets(game, receiver, prev_kt)) {
    auto id = game.state.deck[o].id();
    if (id && variants::is_inverted_id(game.state, *id)) continue;
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    sac.push_back(DcTarget{o, i, /*lock=*/false});
  }
  return sac;
}

// Narrow the receiver play-target's inferred against the playable set plus
// delayed-play successors and stamp it CTP (CTD for inverted suits), the
// same double-stamp reactor's reactive paths perform so hypo_plays sees
// the second play. Returns false when the narrowing empties.
bool stamp_receiver_play(const Game& prev, Game& game, const ClueAction& action,
                         int target) {
  const State& state = game.state;
  int holder = state.holder_of(target);
  auto receiver_conns = delayed_plays(game, action.giver, holder, /*stable=*/false);
  IdentitySet ps = prev.state.playable_set;
  const Thought& target_thought = game.common.thoughts[target];
  IdentitySet base = target_thought.inferred.non_empty()
                         ? target_thought.inferred
                         : target_thought.possible;
  IdentitySet narrowed = base.filter([&](Identity i) {
    if (ps.contains(i)) return true;
    for (const auto& [_, c] : receiver_conns) {
      if (c == i) return true;
    }
    return false;
  });
  if (narrowed.is_empty()) return false;
  game.with_thought(target, [&narrowed](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = narrowed;
    return out;
  });
  int turn = state.turn_count;
  int giver = action.giver;
  CardStatus target_status = variants::target_is_inverted(state, target)
                                 ? CardStatus::CALLED_TO_DISCARD
                                 : CardStatus::CALLED_TO_PLAY;
  game.with_meta(target, [turn, giver, target_status](ConvData& m) {
    m.status = target_status;
    m.by = giver;
    m = m.reason(turn).signal(turn);
  });
  return true;
}

// --- rank: even plays -----------------------------------------------------

std::optional<ClueInterp> reactive_rank(const Game& prev, Game& game,
                                        const ClueAction& action, int anchor,
                                        int reacter) {
  hanabi::instr::ScopedTimer st("reactor0.reactive_rank");
  hanabi::logging::LogScope ls("reactor0.reactive_rank",
                               {{"anchor", anchor}, {"reacter", reacter}});
  const State& state = game.state;
  int receiver = action.target;
  int hand_size = kHandSize[state.num_players];
  auto conns = delayed_plays(game, action.giver, receiver, /*stable=*/false);

  // Phase A — double play. Leftmost playable first, next-leftmost when the
  // react slot is visibly unworkable.
  for (const auto& [target, index] : play_pool(prev, game, receiver)) {
    int target_slot = index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // POV-invariant vetting: the reacter's card must possibly be playable
    // (or a connector) from the holder's own knowledge.
    IdentitySet effective = effective_possible_for(game, react_order);
    bool ok = effective.exists([&](Identity i) {
      if (prev.state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    if (variants::would_lose_inverted_reacter(
            state, react_order, variants::target_is_inverted(state, target),
            /*standard_is_target_play=*/true)) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = variants::target_is_inverted(state, target)
                      ? target_discard(game, action, react_order, /*urgent=*/true)
                      : target_play(game, action, react_order, /*urgent=*/true,
                                    /*stable=*/false);
    if (!interp) continue;
    if (!stamp_receiver_play(prev, game, action, target)) continue;
    auto target_id = state.deck[target].id();
    if (target_id) {
      Identity ti = *target_id;
      game.with_thought(react_order, [ti](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ti);
        return out;
      });
    }
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }

  // Phase B — finesse. Walk TARGETS leftmost-first (one-away cards in the
  // receiver's hand); the reacter must hold the connector at the computed
  // react slot.
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int receive_order = state.hands[receiver][i];
    auto deck_id = state.deck[receive_order].id();
    if (!deck_id || state.playable_away(*deck_id) != 1) continue;
    auto prev_id = deck_id->prev();
    if (!prev_id) continue;
    int target_slot = static_cast<int>(i) + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // "Bob knows his card does not connect" → try the next one-away.
    IdentitySet effective = effective_possible_for(game, react_order);
    if (!effective.contains(*prev_id)) continue;
    // POV-invariant abort, as in reactor: if a visible actual id of the
    // reacter's card is NOT the prereq, the whole reactive is a MISTAKE —
    // no "try the next target", because the reacter (who cannot see their
    // own card) will still act on THIS pairing.
    auto react_actual_id = state.deck[react_order].id();
    if (react_actual_id && *react_actual_id != *prev_id) {
      return std::nullopt;
    }
    if (variants::would_lose_inverted_reacter(
            state, react_order,
            variants::target_is_inverted(state, receive_order),
            /*standard_is_target_play=*/true)) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = variants::target_is_inverted(state, receive_order)
                      ? target_discard(game, action, react_order, /*urgent=*/true)
                      : target_play(game, action, react_order, /*urgent=*/true,
                                    /*stable=*/false);
    if (!interp) return std::nullopt;
    Identity pi = *prev_id;
    game.with_thought(react_order, [pi](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet::single(pi);
      return out;
    });
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }

  // Phase C — double discard (0 plays): the reacter discards the react
  // slot, the receiver discards the dc-target (or locks).
  for (const auto& cand : dc_candidates(prev, game, receiver,
                                        game.allow_reactive_locks)) {
    int target_slot = cand.index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // Don't make the reacter discard a known critical.
    if (game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return state.is_critical(i); })) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_discard(game, action, react_order, /*urgent=*/true);
    if (!interp) continue;
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }
  return std::nullopt;
}

// --- colour: one play -----------------------------------------------------

std::optional<ClueInterp> reactive_colour(const Game& prev, Game& game,
                                          const ClueAction& action, int anchor,
                                          int reacter) {
  hanabi::instr::ScopedTimer st("reactor0.reactive_colour");
  hanabi::logging::LogScope ls("reactor0.reactive_colour",
                               {{"anchor", anchor}, {"reacter", reacter}});
  const State& state = game.state;
  int receiver = action.target;
  int hand_size = kHandSize[state.num_players];

  // Mode 1 — the receiver has a playable: the reacter DISCARDS the react
  // slot and the receiver plays the target.
  SlotList plays = play_pool(prev, game, receiver);
  if (!plays.empty()) {
    for (const auto& [target, index] : plays) {
      int target_slot = index + 1;
      int react_slot = calc_slot(anchor, target_slot, hand_size);
      if (react_slot < 1 ||
          react_slot > static_cast<int>(state.hands[reacter].size())) {
        continue;
      }
      int react_order = state.hands[reacter][react_slot - 1];
      // "If the target would make Bob discard a known critical card, Bob
      // targets the next leftmost playable."
      if (game.common.thoughts[react_order].possible.forall(
              [&](Identity i) { return state.is_critical(i); })) {
        continue;
      }
      if (variants::would_lose_inverted_reacter(
              state, react_order, variants::target_is_inverted(state, target),
              /*standard_is_target_play=*/false)) {
        continue;
      }
      game.with_thought(react_order, [](const Thought& t) {
        Thought out = t;
        out.old_inferred = t.inferred;
        return out;
      });
      auto interp = variants::target_is_inverted(state, target)
                        ? target_play(game, action, react_order, /*urgent=*/true,
                                      /*stable=*/false)
                        : target_discard(game, action, react_order, /*urgent=*/true);
      if (!interp) continue;
      if (!stamp_receiver_play(prev, game, action, target)) continue;
      if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
      return ClueInterp::REACTIVE;
    }
    return std::nullopt;
  }

  // Mode 2 — no playable: the reacter BLIND-PLAYS the react slot to point
  // at the receiver's dc-target (or the lock). The target is determined by
  // the receiver's hand alone; the giver only gives this clue when the
  // react-slot card is visibly playable, and any observer who can see the
  // reacter's card rejects the clue when it isn't (MISTAKE). The reacter's
  // own POV sees no id and trusts the giver.
  auto cands = dc_candidates(prev, game, receiver, game.allow_reactive_locks);
  if (cands.empty()) return std::nullopt;
  const DcTarget& cand = cands.front();
  int target_slot = cand.index + 1;
  int react_slot = calc_slot(anchor, target_slot, hand_size);
  if (react_slot < 1 ||
      react_slot > static_cast<int>(state.hands[reacter].size())) {
    return std::nullopt;
  }
  int react_order = state.hands[reacter][react_slot - 1];
  auto react_actual_id = state.deck[react_order].id();
  if (react_actual_id && !prev.state.is_playable(*react_actual_id)) {
    return std::nullopt;
  }
  if (variants::would_lose_inverted_reacter(
          state, react_order, /*receiver_target_inverted=*/false,
          /*standard_is_target_play=*/true)) {
    return std::nullopt;
  }
  game.with_thought(react_order, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });
  auto interp = target_play(game, action, react_order, /*urgent=*/true,
                            /*stable=*/false);
  if (!interp) return std::nullopt;
  if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
  return ClueInterp::REACTIVE;
}

}  // namespace

// --- top level ------------------------------------------------------------

std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter) {
  hanabi::instr::ScopedTimer st("reactor0.interpret_reactive");
  hanabi::logging::LogScope ls(
      "reactor0.interpret_reactive",
      {{"giver", action.giver}, {"target", action.target}, {"reacter", reacter}});
  const State& state = game.state;
  int giver = action.giver;
  int receiver = action.target;
  const auto& clue = action.clue;

  int anchor = clue.kind == ClueKind::RANK
                   ? clue.value
                   : colour_clue_value(*state.variant, clue.value);
  ReactorWC wc{giver,
               reacter,
               receiver,
               state.hands[receiver],
               to_clue(clue, receiver),
               /*focus_slot=*/anchor,
               /*inverted=*/false,
               state.turn_count,
               /*all_plays=*/game.all_plays};
  wc.rlocks = game.allow_reactive_locks;
  game.waiting.clear();
  game.waiting.push_back(std::move(wc));

  // The receiver decodes positionally at reaction time — never at clue
  // time (POV invariance: selection reads deck ids of the receiver's own
  // hand).
  if (receiver == state.our_player_index) return ClueInterp::REACTIVE;

  if (clue.kind == ClueKind::COLOUR) {
    return reactive_colour(prev, game, action, anchor, reacter);
  }
  return reactive_rank(prev, game, action, anchor, reacter);
}

}  // namespace hanabi::reactor0
