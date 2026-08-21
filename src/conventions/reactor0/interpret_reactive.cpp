#include "hanabi/conventions/reactor0/interpret_reactive.h"

#include <algorithm>
#include <optional>
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
#include "hanabi/conventions/reactor0/reactive_assignment.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/conventions/variants/reversed.h"
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

// Candidate loops stamp the reacter before they know the whole candidate
// works: `target_play` / `target_discard` mutate even when they return
// nullopt (documented in reactor/interpret_clue.h), and a later stage can
// still fail after a successful stamp.
//
// An abandoned stamp is a play call no clue ever made. The reacter's hand
// may legitimately carry several CALLED_TO_PLAY cards — they are actioned
// most-recently-stamped first — which is exactly why a stray one is
// dangerous: it is same-turn, so it competes with the real call for being
// actioned first, and when it is played the receiver has nothing to
// interpret, because no clue ever named it.
//
// So each phase captures the game lazily, immediately before its first
// mutation, and restores on every abandoning path — including the terminal
// nullopt, which is the MISTAKE case that used to leave marks behind.
class Rollback {
 public:
  explicit Rollback(Game& game) : game_(game) {}
  Rollback(const Rollback&) = delete;
  Rollback& operator=(const Rollback&) = delete;

  // Call immediately before the first mutation of a candidate.
  void arm() {
    if (!clean_) clean_ = game_;
  }
  // Undo every mutation since arm(). Cheap no-op until something armed.
  void undo() {
    if (clean_) game_ = *clean_;
  }

 private:
  Game& game_;
  std::optional<Game> clean_;
};

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

// The reactor0 dc-target: the LEFTMOST card whose actual identity is basic
// trash or duplicated elsewhere in the same hand — always, regardless of
// cluedness or of any status already stamped on it. This is a deliberate
// divergence from reactor, which reorders and filters: here the receiver
// derives the target from hand position alone, so a second red 1 further
// right cannot move it and an existing CALLED_TO_DISCARD cannot skip it.
//
// The one exclusion is inverted (orange) cards: a CTD on orange is a
// chuck-as-play-attempt, which strikes on trash, so such a card can never be
// named — it is not a naming preference but an illegal target.
//
// With no such card: under rlocks the single candidate is the OLDEST slot
// with lock=true (the reactive-lock reading); otherwise the reactor
// sacrifice ordering applies. A trash candidate that happens to sit on the
// oldest slot is also lock=true under rlocks — the receiver cannot tell
// trash-on-slot-5 apart from the lock signal and must take the conservative
// reading, so the giver must account for it too.
// `all_trash_targets` (colour mode 2 only, §1d): collect EVERY trash/dupe
// card slot-ascending instead of stopping at the leftmost, so mode 2 can walk
// on to the next one when a pairing is dead by shared knowledge. Rank Phase C
// leaves it false and keeps the strict leftmost rule.
std::vector<DcTarget> dc_candidates(const Game& prev, const Game& game,
                                    int receiver, bool rlocks,
                                    bool all_trash_targets) {
  const State& state = game.state;
  const auto& hand = state.hands[receiver];
  int oldest_index = static_cast<int>(hand.size()) - 1;

  std::vector<DcTarget> found;
  for (size_t i = 0; i < hand.size(); ++i) {
    int o = hand[i];
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
      found.push_back(DcTarget{o, static_cast<int>(i), lock});
      if (!all_trash_targets) return found;
    }
  }
  if (!found.empty()) return found;

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
    // Inverted cards can never be named (see above). A standing CTD is NOT
    // a reason to skip: a new call simply replaces it, since a player holds
    // at most one CALLED_TO_DISCARD at a time (call_invariants.h).
    if (id && variants::is_inverted_id(game.state, *id)) continue;
    sac.push_back(DcTarget{o, i, /*lock=*/false});
  }
  return sac;
}

// Narrow the receiver play-target's inferred against the playable set plus
// delayed-play successors and stamp it CTP (CTD for inverted suits), the
// same double-stamp reactor's reactive paths perform so hypo_plays sees
// the second play. Returns false when the narrowing empties.
// --- react-slot vetting ---------------------------------------------------

// What vetting the react slot decided. The three outcomes are §1g's split:
// shared knowledge may RETARGET (the reacter walks to the next candidate too,
// so every seat stays in step), giver-only knowledge may only REJECT (the
// reacter would still act on this pairing, so the clue must not be offered).
enum class ReactVet { OK, RETARGET, REJECT };

// Vet the react slot against the call the reacter will ACTUALLY receive.
//
// Both reactive paths swap the reacter's action when the receiver's target is
// on an inverted suit (`variants::target_is_inverted`), so that the receiver's
// standard reading of (clue kind + reacter action) still lands on the
// physically correct button: rank Phase A normally calls `target_play` but
// calls `target_discard` for an orange target, and colour mode 1 does the
// reverse. **The vetting has to follow that swap**, which is what
// `reacter_plays` carries.
//
// Getting it wrong is bug_report_4.txt 4.1 in both directions. Vetting a
// DISCARD call for playability throws away perfectly good clues — at replay
// 1942777 T10 the receiver's only playable was an Orange 2, whose react slot
// held a Blue card with both Blue 3s visible across the table, so the
// playability vet failed and the clue degraded to the Phase C lock. Vetting a
// PLAY call for criticality is worse: it lets a blind play through with no
// playability check at all, which strikes.
//
// `variants::would_lose_inverted_reacter` is already swap-aware at both call
// sites, so it stays where it is — only this vet was missing the swap.
ReactVet vet_react_slot(const Game& prev, const Game& game, int react_order,
                        const std::vector<std::pair<int, Identity>>& conns,
                        bool reacter_plays) {
  const State& state = game.state;

  if (!reacter_plays) {
    // The reacter is being told to discard. It need not be playable — it must
    // merely be safe to throw away. Common knowledge, so a failure retargets.
    if (game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return state.is_critical(i); })) {
      return ReactVet::RETARGET;
    }
    return ReactVet::OK;
  }

  // A play call on a card the holder knows is an expendable INVERTED card is
  // not a blind play at all — it is a PITCH. Pressing Play on an orange sends
  // it to the discard pile, so the vet must ask "can you afford to throw this
  // away?", not "is it playable?". A known-trash orange answers yes, and the
  // playability vet below would always answer no (a basic-trash id is never in
  // `playable_set` and can never be a connector), which is how a free pitch
  // came to be skipped — bug_report_4_1_0.txt 4.1.0b, replay 1957942 T19.
  // `can_pitch_for_free` reads `common`, so the reacter walks with us.
  if (variants::can_pitch_for_free(game, react_order)) return ReactVet::OK;

  auto is_workable = [&](Identity i) {
    if (prev.state.playable_set.contains(i)) return true;
    for (const auto& [_, c] : conns) {
      if (c == i) return true;
    }
    return false;
  };

  // POV-invariant: the reacter's card must possibly be playable (or a
  // connector) from the holder's own knowledge.
  if (!effective_possible_for(game, react_order).exists(is_workable)) {
    return ReactVet::RETARGET;
  }

  // Beyond this point the vetting reads the reacter's ACTUAL deck id, which
  // the reacter cannot see. If the giver can tell the card is neither playable
  // nor a connector, the call strikes — reject the clue rather than offer it.
  auto react_actual_id = state.deck[react_order].id();
  if (react_actual_id && !is_workable(*react_actual_id)) return ReactVet::REJECT;
  return ReactVet::OK;
}

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
  // The receiver's inference must match the button they were just handed.
  narrow_to_stamped_button(game, target);
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
  Rollback rb(game);

  // Phase A — double play. Leftmost playable first, next-leftmost when the
  // react slot is visibly unworkable.
  for (const auto& [target, index] : play_pool(prev, game, receiver)) {
    rb.undo();
    int target_slot = index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // Phase A normally calls the reacter to PLAY, but swaps to a discard when
    // the receiver's target is inverted (see the `target_discard` branch
    // below) — so the vetting has to ask about the call that will actually be
    // issued, not the default one. Vetting the discard case for playability is
    // bug_report_4.txt 4.1: at replay 1942777 T10 it skipped the receiver's
    // only playable (an Orange 2) and the clue degraded to the Phase C lock.
    const bool target_inverted = variants::target_is_inverted(state, target);
    switch (vet_react_slot(prev, game, react_order, conns,
                           /*reacter_plays=*/!target_inverted)) {
      case ReactVet::RETARGET:
        continue;
      case ReactVet::REJECT:
        rb.undo();
        return std::nullopt;
      case ReactVet::OK:
        break;
    }
    // A free pitch is exempt from the loss guard. `would_lose_inverted_reacter`
    // rejects every play-type call on an orange react card on the grounds that
    // pitching it "loses the copy for nothing" — true for a useful orange,
    // false for one the holder knows is trash, where there is no copy to lose.
    // The exemption reads `common`, unlike the guard itself (which is
    // POV-asymmetric by design and may therefore only reject), so it does not
    // desync the walk.
    const bool free_pitch = variants::can_pitch_for_free(game, react_order);
    if (!free_pitch && variants::would_lose_inverted_reacter(
                           state, react_order, target_inverted,
                           /*standard_is_target_play=*/true)) {
      // Giver-only: the guard reads the react card's suit. The reacter does
      // not know their card is inverted, so they would chuck it and strike.
      rb.undo();
      return std::nullopt;
    }
    rb.arm();
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    // `reactor::target_play` narrows `inferred` to the playable set and bails
    // when that empties, so it can never stamp a pitch — a pitched card need
    // not be playable at all. `stamp_orange_pitch` is the stable side's answer
    // to exactly that (reactor0 §1b) and is reused here.
    auto interp =
        target_inverted
            ? target_discard(game, action, react_order, /*urgent=*/true)
        : free_pitch
            ? stamp_orange_pitch(game, action, react_order, /*urgent=*/true)
            : target_play(game, action, react_order, /*urgent=*/true,
                          /*stable=*/false);
    // Build the reacter's inference to match whichever button was stamped.
    if (interp) narrow_to_stamped_button(game, react_order);
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
    rb.undo();
    int receive_order = state.hands[receiver][i];
    auto deck_id = state.deck[receive_order].id();
    if (!deck_id || state.playable_away(*deck_id) != 1) continue;
    // Direction-aware: on a reversed suit the connector is rank+1, not
    // rank-1 (variants/reversed.h). Hardcoding prev() looked for a card that
    // can never be the prerequisite and rejected every reversed finesse.
    auto prev_id = variants::connector_of(state, *deck_id);
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
      rb.undo();
      return std::nullopt;
    }
    if (variants::would_lose_inverted_reacter(
            state, react_order,
            variants::target_is_inverted(state, receive_order),
            /*standard_is_target_play=*/true)) {
      // Giver-only knowledge (the react card's suit) — reject, never
      // retarget; the reacter cannot see it and would act on this pairing.
      rb.undo();
      return std::nullopt;
    }
    rb.arm();
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = variants::target_is_inverted(state, receive_order)
                      ? target_discard(game, action, react_order, /*urgent=*/true)
                      : target_play(game, action, react_order, /*urgent=*/true,
                                    /*stable=*/false);
    // Same for Phase B's blind-play call.
    if (interp) narrow_to_stamped_button(game, react_order);
    if (!interp) {
      rb.undo();
      return std::nullopt;
    }
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
  // slot, the receiver discards the dc-target (or locks). Unlike colour
  // mode 2 this keeps the strict leftmost dc-target rule: it asks only for
  // ONE candidate, so the loop below still walks nothing but the rlocks-off
  // sacrifice list.
  for (const auto& cand : dc_candidates(prev, game, receiver,
                                        game.allow_reactive_locks,
                                        /*all_trash_targets=*/false)) {
    rb.undo();
    int target_slot = cand.index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // Phase C always discards — there is no target to be inverted, since the
    // dc-target is not a play. Routed through the shared helper for uniformity
    // only; `reacter_plays=false` is exactly the old critical check.
    if (vet_react_slot(prev, game, react_order, conns,
                       /*reacter_plays=*/false) != ReactVet::OK) {
      continue;
    }
    rb.arm();
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_discard(game, action, react_order, /*urgent=*/true);
    // Same as Phase A and Phase B above. `target_discard` narrows to the
    // NON-CRITICAL ids, which is the plain-suit reading of "throw this away";
    // on an inverted suit a chuck is a play attempt, so the card must be
    // PLAYABLE -- and a playable orange 5 is critical, so that narrowing keeps
    // the trash orange and drops the one the call actually meant.
    if (interp) narrow_to_stamped_button(game, react_order);
    if (!interp) continue;
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }
  rb.undo();
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
  // Mirrors reactive_rank's call exactly, `receiver` argument included — it
  // feeds the play half of `vet_react_slot`, which mode 1 reaches when the
  // receiver's target is inverted and the reacter is told to blind-play.
  auto conns = delayed_plays(game, action.giver, receiver, /*stable=*/false);
  Rollback rb(game);

  // Mode 1 — the receiver has a playable: the reacter DISCARDS the react
  // slot and the receiver plays the target.
  SlotList plays = play_pool(prev, game, receiver);
  if (!plays.empty()) {
    for (const auto& [target, index] : plays) {
      rb.undo();
      int target_slot = index + 1;
      int react_slot = calc_slot(anchor, target_slot, hand_size);
      if (react_slot < 1 ||
          react_slot > static_cast<int>(state.hands[reacter].size())) {
        continue;
      }
      int react_order = state.hands[reacter][react_slot - 1];
      // Mode 1 normally calls the reacter to DISCARD — "if the target would
      // make Bob discard a known critical card, Bob targets the next leftmost
      // playable" — but swaps to `target_play` when the receiver's target is
      // inverted (see below). Vetting the swapped case for criticality alone
      // is the mirror of bug_report_4.txt 4.1, and the worse half: it lets a
      // blind play through with no playability check, which strikes.
      const bool target_inverted = variants::target_is_inverted(state, target);
      switch (vet_react_slot(prev, game, react_order, conns,
                             /*reacter_plays=*/target_inverted)) {
        case ReactVet::RETARGET:
          continue;
        case ReactVet::REJECT:
          rb.undo();
          return std::nullopt;
        case ReactVet::OK:
          break;
      }
      if (variants::would_lose_inverted_reacter(
              state, react_order, target_inverted,
              /*standard_is_target_play=*/false)) {
        // Giver-only knowledge (the react card's suit) — reject, never
        // retarget. The critical-card guard above is different: that one is
        // common knowledge, so the reacter walks to the next candidate too.
        rb.undo();
        return std::nullopt;
      }
      rb.arm();
      game.with_thought(react_order, [](const Thought& t) {
        Thought out = t;
        out.old_inferred = t.inferred;
        return out;
      });
      auto interp = variants::target_is_inverted(state, target)
                        ? target_play(game, action, react_order, /*urgent=*/true,
                                      /*stable=*/false)
                        : target_discard(game, action, react_order, /*urgent=*/true);
      // As Phase A and Phase B do. Replay 1967376: this is where an Odds and
      // Evens RANK clue lands (the odd bucket runs this ruleset), and without
      // the narrowing the reacter-CTD kept a trash o1 and lost the playable o5.
      if (interp) narrow_to_stamped_button(game, react_order);
      if (!interp) continue;
      if (!stamp_receiver_play(prev, game, action, target)) continue;
      if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
      return ClueInterp::REACTIVE;
    }
    rb.undo();
    return std::nullopt;
  }

  // Mode 2 — no playable: the reacter BLIND-PLAYS the react slot to point
  // at the receiver's dc-target (or the lock). The target is determined by
  // the receiver's hand alone; the giver only gives this clue when the
  // react-slot card is visibly playable, and any observer who can see the
  // reacter's card rejects the clue when it isn't (MISTAKE). The reacter's
  // own POV sees no id and trusts the giver.
  // The dc-target is walked, not fixed: a pairing whose react slot EVERY seat
  // can already see cannot play teaches nothing, so the reading moves on to
  // the next trash/dupe candidate rightward (replay 1942458 T47 — the
  // leftmost target mapped onto a react slot the reacter knew was {b5,p5},
  // both dead, while the next candidate mapped onto a live slot).
  //
  // The split is §1g's, and it is the whole reason this is safe:
  //   - `effective_possible_for` is SHARED — the reacter computes the same
  //     set for its own card — so a `continue` here keeps every seat walking
  //     in step;
  //   - `state.deck[react_order]` is GIVER-ONLY (the reacter sees no id in
  //     its own hand), so failing that check must still REJECT the whole
  //     clue. Retargeting on it would leave the reacter blind-playing the
  //     original pairing while giver and receiver had agreed on another.
  auto cands = dc_candidates(prev, game, receiver, game.allow_reactive_locks,
                             /*all_trash_targets=*/true);
  for (const DcTarget& cand : cands) {
    rb.undo();
    int target_slot = cand.index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // Shared: nothing the reacter could hold here can play → retarget.
    IdentitySet react_poss =
        hanabi::reactor::effective_possible_for(game, react_order);
    if (!react_poss.exists(
            [&](Identity i) { return prev.state.is_playable(i); })) {
      continue;
    }
    // Giver-only: reject, never retarget.
    auto react_actual_id = state.deck[react_order].id();
    if (react_actual_id && !prev.state.is_playable(*react_actual_id)) {
      rb.undo();
      return std::nullopt;
    }
    if (variants::would_lose_inverted_reacter(
            state, react_order, /*receiver_target_inverted=*/false,
            /*standard_is_target_play=*/true)) {
      rb.undo();
      return std::nullopt;
    }
    rb.arm();
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp = target_play(game, action, react_order, /*urgent=*/true,
                              /*stable=*/false);
    if (!interp) continue;
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }
  rb.undo();
  return std::nullopt;
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

  // The clue's reactive assignment: which parity bucket it is in, and its
  // anchor within that bucket. `/set` overrides both; with no overrides this is
  // the variant's built-in table.
  const ReactiveAssignment assign = reactive_assignment(
      *state.variant, game.reactive_overrides, clue.kind, clue.value);
  int anchor = assign.value;
  ReactorWC wc{giver,
               reacter,
               receiver,
               state.hands[receiver],
               to_clue(clue, receiver),
               /*focus_slot=*/anchor,
               /*inverted=*/false,
               state.turn_count,
               // /allplays is a reactor concept. reactor0's parity is fixed
               // by clue kind, so the flag must never travel in a reactor0
               // WC — carrying it would let reaction resolution contradict
               // the reading every seat already agreed on at clue time.
               /*all_plays=*/false};
  wc.even_parity = assign.even;
  wc.rlocks = game.allow_reactive_locks;
  game.waiting.clear();
  game.waiting.push_back(std::move(wc));

  // The receiver decodes positionally at reaction time — never at clue
  // time (POV invariance: selection reads deck ids of the receiver's own
  // hand).
  if (receiver == state.our_player_index) return ClueInterp::REACTIVE;

  // Dispatch on the PARITY, not the clue kind: `reactive_rank` is the
  // even-parity ruleset (double play / double discard) and `reactive_colour`
  // the odd one (exactly one play). Odds and Evens swaps which kind carries
  // which, and `/set` can move a single clue, so the bucket is read off the
  // assignment rather than from the kind.
  if (!assign.even) {
    return reactive_colour(prev, game, action, anchor, reacter);
  }
  return reactive_rank(prev, game, action, anchor, reacter);
}

}  // namespace hanabi::reactor0
