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
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
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
using hanabi::reactor0::slot_has_spare_inverted;
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
  // On an inverted suit the receiver sheds this card with PLAY (a pitch), not
  // DISCARD, so the parity hands the reacter the opposite button to the plain
  // case. Carried on the target because that is where it is decided.
  bool inverted = false;
};

// The reactor0 dc-target: the LEFTMOST card whose actual identity is basic
// trash or duplicated elsewhere in the same hand — always, regardless of
// cluedness or of any status already stamped on it. This is a deliberate
// divergence from reactor, which reorders and filters: here the receiver
// derives the target from hand position alone, so a second red 1 further
// right cannot move it and an existing CALLED_TO_DISCARD cannot skip it.
//
// Inverted (orange) cards are IN the pool, and `DcTarget::inverted` says so.
// Until v10.6.0 they were excluded outright, on the grounds that a CTD on
// orange is a chuck-as-play-attempt and strikes on trash. True of the DISCARD
// button, and blind to the other one: an expendable orange is thrown by
// pressing PLAY -- a pitch -- which `receiver_ctp_set` has always admitted
// (`!playable && !critical`, interpret_clue.cpp). So such a card can be named;
// what changes is the receiver's button, and by parity the reacter's with it.
//
// Replay 1974257 T30: will-bot67 held o3, o3, g5, y5, o4 with orange on 1 and
// nothing playable. Every expendable card he had was orange, so the pool came
// back EMPTY, the green clue read as a MISTAKE, and will-bot69 threw its chop.
// The leftmost duped o3 was the target all along.
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
    const bool inverted = variants::is_inverted_id(state, *id);
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
      // A pitch throws the card away, so it must be one the receiver can
      // SPARE -- the same question `receiver_ctp_set` asks. A critical orange
      // is trash to nobody and must not be named. (Basic trash is never
      // critical, so this only ever bites the same-hand-dupe arm, where the
      // other copy makes it non-critical anyway; it is here as the statement
      // of the rule rather than as a live filter.)
      if (inverted && state.is_critical(*id)) continue;
      bool lock = rlocks && static_cast<int>(i) == oldest_index;
      found.push_back(DcTarget{o, static_cast<int>(i), lock, inverted});
      if (!all_trash_targets) return found;
    }
  }
  if (!found.empty()) return found;

  if (rlocks) {
    if (oldest_index < 0) return {};
    return {DcTarget{hand[oldest_index], oldest_index, /*lock=*/true,
                     /*inverted=*/false}};
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
    // The SACRIFICE list is a different question -- throwing a card that is
    // still useful -- and is left plain-only. Nothing in v10.6.0 turns on it.
    if (id && variants::is_inverted_id(game.state, *id)) continue;
    sac.push_back(DcTarget{o, i, /*lock=*/false, /*inverted=*/false});
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
// Is pressing PLAY on this card a PITCH rather than a play?
//
// Only when EVERY reading is inverted. That is the whole condition, and it is
// what makes a pitch safe to reason about: Play on an orange sends it to the
// discard pile, so it cannot strike, and the question stops being "is it
// playable?" and becomes "can it be spared?".
//
// Reads `common`, so giver, reacter and observer agree.
bool react_slot_is_a_pitch(const Game& game, int react_order) {
  const State& s = game.state;
  const IdentitySet poss = game.common.thoughts[react_order].possible;
  return poss.non_empty() && poss.forall([&s](Identity i) {
    return variants::is_inverted_id(s, i);
  });
}

ReactVet vet_react_slot(const Game& prev, const Game& game, int react_order,
                        const std::vector<std::pair<int, Identity>>& conns,
                        bool reacter_plays) {
  const State& state = game.state;

  if (!reacter_plays) {
    // The reacter is being told to press DISCARD. It need not be playable — it
    // must merely be safe to throw away. Common knowledge, so a failure
    // retargets.
    //
    // "Throw away" is the PLAIN-suit reading of that button. On an inverted
    // suit Discard is a CHUCK, which puts the card on its stack — so a reading
    // that is inverted AND playable is not a loss at all, it is the play the
    // call is asking for.
    //
    // Without the exception this is unreachable in Dark Orange, where every
    // card is one-of-each and therefore critical by construction: a reacter
    // slot that could be dark always answered "all critical" and was always
    // retargeted. Replay 1967491 T36 — will-bot67's slot 5 was {d2, d4} with
    // the dark stack on 1, so chucking it would have stacked the d2, and the
    // clue came out a MISTAKE instead.
    const bool every_reading_loses =
        game.common.thoughts[react_order].possible.forall([&](Identity i) {
          if (variants::is_inverted_id(state, i) && state.is_playable(i)) {
            return false;  // the chuck stacks it — nothing is lost
          }
          return state.is_critical(i);
        });
    if (every_reading_loses) return ReactVet::RETARGET;
    return ReactVet::OK;
  }

  // A play call on a card the holder knows is an expendable INVERTED card is
  // not a blind play at all — it is a PITCH. Pressing Play on an orange sends
  // it to the discard pile, so the vet must ask "can you afford to throw this
  // away?", not "is it playable?". A known-trash orange answers yes, and the
  // playability vet below would always answer no (a basic-trash id is never in
  // `playable_set` and can never be a connector), which is how a free pitch
  // came to be skipped — bug_report_4_1_0.txt 4.1.0b, replay 1957942 T19.
  //
  // `slot_is_pitchable` is the shared definition (interpret_reaction.h), the
  // same one the deferred negatives ask: ANY playable plain reading, or ANY
  // NON-CRITICAL inverted one. Until v10.3.0 this asked
  // `variants::can_pitch_for_free` instead, which requires EVERY reading to be
  // a dead orange -- so a known orange that might still be the playable one was
  // called unpitchable and the target was retargeted away. Replay 1973976 T12:
  // will-bot69's slot 3 was {o1,o2,o3,o4} with orange on 1, so o2 was playable
  // and o3/o4 were merely non-critical; the pitch that would have chucked
  // will-bot67's playable o2 onto the stack was skipped.
  // GATED ON `react_slot_is_a_pitch`, and that gate is load-bearing. The tests
  // below this point are about STRIKING -- the playability retarget, and the
  // giver-only reject that reads the card's real id. Neither applies to a pitch,
  // which discards rather than plays. But letting `slot_is_pitchable` alone
  // short-circuit them would disable the strike checks for almost every unclued
  // card in an Orange variant, since a wide empathy always admits some
  // non-critical orange.
  if (react_slot_is_a_pitch(game, react_order) &&
      slot_is_pitchable(state, effective_possible_for(game, react_order))) {
    return ReactVet::OK;
  }

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

// Is `target` a viable receiver call? A VETO ONLY -- nothing is stamped and
// nothing is written to `inferred`.
//
// Until v8.0.0 this function also did the stamping, which is why the receiver
// carried a call and a narrowed set from the moment the clue landed, a turn
// before the reacter had moved. The set it built was then thrown away and
// rebuilt at resolution against the live stacks, which is how an inference
// gained members it never had (replay 1967558: `{r1,y1,b1,p1,w1}` became
// `{r1,y1,b1,p2,w1}`). The call is made when the reacter acts (§1d); all this
// does now is tell target selection whether the pairing is worth choosing.
//
// The two-stack reading is used here as well as at resolution, and from the
// same helper, so clue-time selection and reaction-time stamping cannot
// disagree about what "after the reaction" means.
bool receiver_call_is_viable(const Game& prev, const Game& game,
                             const ClueAction& action, int target,
                             int react_order, CardStatus reacter_button) {
  const State& state = game.state;
  int holder = state.holder_of(target);
  auto receiver_conns = delayed_plays(game, action.giver, holder, /*stable=*/false);
  const State after = state_after_reacter(prev.state, react_order, reacter_button);
  const CardStatus button = variants::target_is_inverted(state, target)
                                ? CardStatus::CALLED_TO_DISCARD
                                : CardStatus::CALLED_TO_PLAY;
  IdentitySet allowed = button == CardStatus::CALLED_TO_DISCARD
                            ? receiver_ctd_set(prev.state, after)
                            : receiver_ctp_set(prev.state, after);
  const Thought& target_thought = game.common.thoughts[target];
  IdentitySet base = target_thought.inferred.non_empty()
                         ? target_thought.inferred
                         : target_thought.possible;
  return base.exists([&](Identity i) {
    if (allowed.contains(i)) return true;
    // A delayed play is still a play: it lands once the connector ahead of it
    // does, which is exactly what a finesse promises.
    for (const auto& [_, c] : receiver_conns) {
      if (c == i) return true;
    }
    return false;
  });
}

// --- rank: even plays -----------------------------------------------------

std::optional<ClueInterp> reactive_rank(const Game& prev, Game& game,
                                        const ClueAction& action, int anchor,
                                        int reacter, int receiver) {
  hanabi::instr::ScopedTimer st("reactor0.reactive_rank");
  hanabi::logging::LogScope ls(
      "reactor0.reactive_rank",
      {{"anchor", anchor}, {"reacter", reacter}, {"receiver", receiver}});
  const State& state = game.state;
  // PASSED IN, never re-derived from `action.target`. The clued seat and the
  // receiver are the same player in every ordinary variant, and are NOT under
  // target parity, where a clue to Bob has Cathy as its receiver. Deriving it
  // here walked the reacter's own hand -- all `nullopt` from his own seat -- so
  // the pool came back empty and the clue read as a MISTAKE (replay 1973971).
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
    // not be playable at all. `stamp_react_play_button` is the shared ladder
    // that decides between the two (see its comment); `free_pitch` keeps its
    // own arm above it because a card whose every reading is a DEAD orange is
    // a pitch before any of that is asked.
    //
    // The inverted-target arm goes through `stamp_react_discard_button` for the
    // mirror-image reason: `target_discard` alone narrows to the NON-critical
    // plain reading, which cannot describe a react slot that is itself an
    // orange the stack is waiting for.
    auto interp =
        target_inverted
            ? stamp_react_discard_button(game, action, react_order)
        : free_pitch
            ? stamp_orange_pitch(game, action, react_order, /*urgent=*/true)
            : stamp_react_play_button(game, action, react_order);
    // Build the reacter's inference to match whichever button was stamped.
    if (interp) narrow_to_stamped_button(game, react_order);
    if (!interp) continue;
    // Selection only. The receiver is stamped when the reacter acts (§1d).
    if (!receiver_call_is_viable(prev, game, action, target, react_order,
                                 game.meta[react_order].status)) {
      continue;
    }
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
    //
    // Asked of BOTH sets, and it has to be. `effective_possible_for` filters
    // `possible` by the copies every other seat can see; the stamp below is
    // `target_play`, which narrows `inferred`. Where those disagree the guard
    // passes and the stamp then refuses -- and worse, the pin at the bottom of
    // the loop writes `inferred = {prev_id}` unconditionally, which WIDENS a
    // reading that had already excluded the connector (§1i forbids exactly
    // that). Replay 1973406 T18: yagami_blue's slot 2 was inferred `{g1}` while
    // its `possible` was still the whole green suit, so the g5 pairing passed
    // this guard on `possible` and died at the stamp on `inferred`.
    //
    // Both sets read `common`, so giver, reacter and receiver walk in step.
    IdentitySet effective = effective_possible_for(game, react_order);
    if (!effective.contains(*prev_id) ||
        !game.common.thoughts[react_order].possibilities().contains(*prev_id)) {
      continue;
    }
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
                      ? stamp_react_discard_button(game, action, react_order)
                      : target_play(game, action, react_order, /*urgent=*/true,
                                    /*stable=*/false);
    // Same for Phase B's blind-play call.
    if (interp) narrow_to_stamped_button(game, react_order);
    // WALK, do not give up. A stamp that refuses means the reacter's side does
    // not work on THIS pairing -- shared knowledge, so every seat walks to the
    // next one-away target together. Phase A, Phase C and both colour modes
    // have handled it this way since v10.6.0; Phase B was the last site still
    // carrying the pre-v10.6.0 give-up, which killed the whole clue on the
    // first unusable pairing even when a later one was perfectly good.
    //
    // Replay 1973406 T18 (`/set 1 even 5`, so anchor 5): Neema's slot 3 (g5,
    // green on 3) is one away and comes first, pairing with yagami_blue's slot 2
    // -- inferred `{g1}`, which cannot be the g4. Phase B returned MISTAKE there
    // instead of walking on to her slot 4 (b3, blue on 1), whose pairing is
    // yagami_blue's slot 1: an unclued card that can hold the b2. The clue read
    // as nothing at all and the bot locked Neema instead of blind-playing.
    if (!interp) continue;
    Identity pi = *prev_id;
    game.with_thought(react_order, [pi](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet::single(pi);
      return out;
    });
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }

  // Phase C — the trash targets, walked leftmost-first exactly as colour
  // mode 2 walks them. Normally a double discard (0 plays): the reacter
  // discards the react slot and the receiver discards the dc-target, or locks.
  //
  // v10.6.0 made this WALK. It used to ask for one candidate and give up if
  // the reacter's side did not work on it, which was a documented asymmetry
  // with the odd bucket and is now gone: the reading is target-first in both
  // buckets -- playable, then finesse (even only), then trash, each
  // leftmost-first, moving on when the reacter's own reaction does not work.
  for (const auto& cand : dc_candidates(prev, game, receiver,
                                        game.allow_reactive_locks,
                                        /*all_trash_targets=*/true)) {
    rb.undo();
    int target_slot = cand.index + 1;
    int react_slot = calc_slot(anchor, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    // An INVERTED target is shed with PLAY (a pitch), and EVEN parity matches
    // the two buttons -- so the reacter presses Play too. That is a genuine
    // blind play when his own card is plain, and a pitch of his own when it is
    // a known orange, which is why the stamp mirrors Phase A's inverted arm.
    // A plain target keeps the old reading: both press Discard.
    const bool target_inverted = cand.inverted;
    if (vet_react_slot(prev, game, react_order, conns,
                       /*reacter_plays=*/target_inverted) != ReactVet::OK) {
      continue;
    }
    rb.arm();
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    auto interp =
        target_inverted
            ? stamp_react_play_button(game, action, react_order)
            : stamp_react_discard_button(game, action, react_order);
    // Same as Phase A and Phase B above. `target_discard` narrows to the
    // NON-CRITICAL ids, which is the plain-suit reading of "throw this away";
    // on an inverted suit a chuck is a play attempt, so the card must be
    // PLAYABLE -- and a playable orange 5 is critical, so that narrowing keeps
    // the trash orange and drops the one the call actually meant. That is why
    // the plain arm is the shared discard ladder and not a bare `target_discard`:
    // the chuck reading has to be reachable here too.
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
                                          int reacter, int receiver) {
  hanabi::instr::ScopedTimer st("reactor0.reactive_colour");
  hanabi::logging::LogScope ls(
      "reactor0.reactive_colour",
      {{"anchor", anchor}, {"reacter", reacter}, {"receiver", receiver}});
  const State& state = game.state;
  // Passed in, never re-derived -- see `reactive_rank`.
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
      // A reacter told to press Discard on a card that could be a playable
      // INVERTED one is CHUCKING it, not throwing it away. `target_discard`
      // narrows to the non-critical ids -- the plain-suit reading -- which in
      // Dark Orange empties outright, since every card there is one-of-each, so
      // it refuses to stamp and the whole clue reads as a MISTAKE. Replay
      // 1967491 T36: will-bot67's slot 5 was {d2, d4} with the dark stack on 1,
      // and chucking it stacks the d2. `stamp_react_discard_button` tries the
      // chuck first and falls back to the ordinary throw-away, so both readings
      // are reachable and the call is refused only when neither exists.
      //
      // An inverted target swaps the reacter onto the PLAY button -- and when
      // his own card is a known orange, pressing Play is a PITCH, not a play.
      // `reactor::target_play` narrows `inferred` to the playable set and bails
      // when that empties, so it can never stamp one; Phase A already reaches
      // for `stamp_orange_pitch` in the mirror-image case and this is the same
      // need on the odd-parity side.
      //
      // Replay 1973976 T12: will-bot69's slot 3 was a known orange {o1..o4}
      // with orange on 1, inferred {o3, o4}. Neither is playable, so
      // `target_play` refused to stamp, the target was skipped, and the pitch
      // that would have chucked will-bot67's playable o2 onto the stack never
      // happened -- the clue degraded to naming his r1 instead.
      auto interp =
          target_inverted
              ? stamp_react_play_button(game, action, react_order)
              : stamp_react_discard_button(game, action, react_order);
      // As Phase A and Phase B do. Replay 1967376: this is where an Odds and
      // Evens RANK clue lands (the odd bucket runs this ruleset), and without
      // the narrowing the reacter-CTD kept a trash o1 and lost the playable o5.
      if (interp) narrow_to_stamped_button(game, react_order);
      if (!interp) continue;
      // Selection only. The receiver is stamped when the reacter acts (§1d).
      if (!receiver_call_is_viable(prev, game, action, target, react_order,
                                   game.meta[react_order].status)) {
        continue;
      }
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

    // An INVERTED target is shed with PLAY (a pitch), not DISCARD -- so odd
    // parity, which opposes the two buttons, puts the reacter on DISCARD
    // rather than on the blind play the plain case asks for. Everything below
    // the branch is about that blind play and does not apply.
    //
    // `slot_elims` (interpret_reaction.cpp, category 3) has always computed
    // this side of the reading for the deferred negatives -- "the receiver
    // sheds trash with the OTHER button, so the parity test flips" -- and
    // `resolve_reaction` derives the receiver's button from the one the
    // reacter presses, so it already stamps a CTP and narrows with
    // `receiver_ctp_set`. Only the SELECTION here was missing. Replay 1974257
    // T30.
    if (cand.inverted) {
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
      // The same ladder mode 1 uses for its Discard arm: pressing Discard on a
      // card that could be a playable orange is a CHUCK, and `target_discard`
      // narrows to the non-critical plain reading, which cannot describe one.
      auto interp = stamp_react_discard_button(game, action, react_order);
      if (interp) narrow_to_stamped_button(game, react_order);
      if (!interp) continue;
      if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
      return ClueInterp::REACTIVE;
    }

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

// Record, on the reacter's called card, which RECEIVER order its slot is
// paired with.
//
// Every reactive branch below ends by writing `react_order` onto the waiting
// connection, so this runs once at the top level instead of at all six stamp
// sites. The pairing is symmetric -- `calc_slot(anchor, react_slot, h)` is the
// target slot exactly as `calc_slot(anchor, target_slot, h)` was the react slot
// -- so the branch's own arithmetic is reproduced here rather than plumbed out
// of it.
//
// Why persist it at all: `decide.cpp`'s urgent scan needs to know whether the
// receiver is still decoding against our slot choice, and a DEFERRAL clears
// `Game::waiting` while keeping the call, so the connection is not there to ask
// by the time the question matters. See `ConvData::react_target_order`.
void record_react_target(Game& game) {
  if (game.waiting.empty()) return;
  const ReactorWC& wc = game.waiting.front();
  const int react_order = wc.react_order;
  if (react_order < 0) return;  // no reacter slot was computed
  const auto& reacter_hand = game.state.hands[wc.reacter];
  auto it = std::find(reacter_hand.begin(), reacter_hand.end(), react_order);
  if (it == reacter_hand.end()) return;
  const int react_slot = static_cast<int>(it - reacter_hand.begin()) + 1;
  const int target_slot =
      calc_slot(wc.focus_slot, react_slot, kHandSize[game.state.num_players]);
  if (target_slot < 1 ||
      target_slot > static_cast<int>(wc.receiver_hand.size())) {
    return;
  }
  game.meta[react_order].react_target_order = wc.receiver_hand[target_slot - 1];
}

}  // namespace

// --- the two button ladders ---------------------------------------------

// Stamp the reacter's slot when the call is the PLAY button. Three steps, in
// this order, shared by every site that issues one so they cannot drift.
//
//   1. EVERY reading inverted -> a pitch, unambiguously. This stays first and is
//      not folded into "play first": `state.is_playable` is true of an o2 on an
//      inverted suit, so `target_play` would happily stamp a PLAY reading on a
//      card whose Play button actually discards it. That is the v10.3.0 defect
//      (replay 1973976 T12) and step 1 is what prevents it.
//   2. Otherwise the ordinary play reading.
//   3. Otherwise, for a CLUED or STAMPED card with an inverted reading it can
//      spare, a pitch. Play first, pitch as a fallback: the pitch is read only
//      where no reading can play, so every strike check above keeps its force.
//
// Step 3 is v10.8.0, replay 1974331 T8: will-bot69's slot 4 was clued to
// {r3,y1,y3,g3,b3,o3} and was the o3 -- two copies, none discarded, so free to
// throw. Nothing in its `inferred` could play, `target_play` refused to stamp,
// Phase A walked past the pairing, and the second candidate blind-played a b2
// onto an empty blue stack. The cluedness condition is where v10.3.0's worry
// lives: an UNCLUED card in an Orange variant has a wide enough empathy to
// always admit some non-critical orange, and letting that alone license a pitch
// would disable the strike checks across the board.
std::optional<ClueInterp> stamp_react_play_button(Game& game,
                                                  const ClueAction& action,
                                                  int react_order,
                                                  bool urgent, bool stable) {
  const State& state = game.state;
  if (react_slot_is_a_pitch(game, react_order)) {
    return stamp_orange_pitch(game, action, react_order, urgent);
  }
  if (auto interp = target_play(game, action, react_order, urgent, stable)) {
    return interp;
  }
  const bool invested = state.deck[react_order].clued ||
                        game.meta[react_order].status != CardStatus::NONE;
  if (!invested) return std::nullopt;
  // `slot_has_spare_inverted`, NOT `slot_is_pitchable`: `target_play` has just
  // established that no reading plays, so the plain half of the wider predicate
  // could only be satisfied by something `inferred` excludes -- at 1974331 T8
  // that is a y1 the reading had already ruled out, which has nothing to do
  // with why the card can be pitched.
  if (!slot_has_spare_inverted(state,
                               effective_possible_for(game, react_order))) {
    return std::nullopt;
  }
  return stamp_orange_pitch(game, action, react_order, urgent);
}

// Stamp the reacter's slot when the call is the DISCARD button. The mirror of
// `stamp_react_play_button` above, and it exists for the same reason: one ladder,
// shared by every site that issues the button, so they cannot drift.
//
//   1. A CHUCK, when some reading is an inverted card the stack is waiting for --
//      on an inverted suit the Discard button stacks the card rather than
//      throwing it.
//   2. Otherwise the ordinary throw-away reading.
//
// Those two arms are exactly `slot_is_chuckable` (interpret_reaction.h) asked of
// the slot's own inferences -- a playable inverted reading, or a non-critical
// plain one -- which is what makes the refusal meaningful: the call is declined
// when NEITHER reading exists, never because the wrong one was picked. The
// predicate is deliberately not called here; the two stamps already answer their
// own halves of it, and a separate gate is precisely what drifted before.
//
// v10.9.0, replay 1974342 T13. There used to be a `react_could_chuck` gate that
// chose between the two arms instead of cascading, and it asked its question of
// `possible` while `stamp_orange_chuck` asks it of `possibilities()` -- i.e. of
// `inferred` once that is non-empty. reactor0's own deferred negatives
// (`slot_elims`) strip the PLAYABLE identities from a reacter's unpaired slots,
// which is that exact axis, so on any reacter that had already been through one
// reactive the gate said "chuck", the chuck stamp found nothing playable and
// inverted left to name, and there was no path back to the ordinary discard.
// will-bot69's slots 4 and 5 were both `{r2,r3,r4,y3,y4,g2,g3,g4,o2,o3,o4}` with
// orange on 0: `possible` still held the o1, `inferred` did not, and both
// pairings died on it. The clue -- a plain reactive discard -- read as a MISTAKE
// and the bot threw the Blue 5 off its chop instead. Both slots were chuckable
// the whole time through the plain arm (r2, two copies, none discarded).
//
// `stamp_orange_chuck` computes its keep-set before it mutates anything
// (reactor0/interpret_clue.cpp), so a refusal by arm 1 is free and the cascade
// is safe to write in this order.
std::optional<ClueInterp> stamp_react_discard_button(Game& game,
                                                     const ClueAction& action,
                                                     int react_order,
                                                     bool urgent) {
  if (auto interp = stamp_orange_chuck(game, action, react_order, urgent)) {
    return interp;
  }
  return target_discard(game, action, react_order, urgent);
}

// --- top level ------------------------------------------------------------

bool bob_clue_is_reactive(const State& state) {
  if (!variants::uses_target_parity(*state.variant)) return false;
  // A CLUE TO BOB IS STABLE AT PACE 1 OR LESS (v14.0.0).
  //
  // Replaces the score rule -- reactive below half the variant maximum, stable at
  // or above -- which shipped at 60% in v11.0.0 and moved to 50% in v11.4.0. Pace
  // is the better trigger for the same reason it gates section 4 of the clue list:
  // it counts the turns still available rather than the points already banked, and
  // it is turns running out that makes a reactive's two-seat co-ordination too
  // expensive to spend one on.
  //
  // THE 8-CLUE ARM IS GONE (also v14.0.0). Through v13.5.0 a clue to Bob was also
  // stable at 8 tokens, whatever the score -- a deadlock guard, not a tuning knob:
  // at 8 tokens a discard is illegal, so the turn must produce a clue, and under
  // target parity every clue is reactive, so if every reactive candidate reads
  // MISTAKE the clue phase declines with an empty set (replay 1977786 T35).
  // Removing it does NOT bring that deadlock back: phase 2 closed the same hole
  // variant-independently -- `choose_action` empties the chuck list at 8 tokens
  // and rungs 12/13 answer with a PITCH, legal at any token count. What is lost
  // is the readable clue, not the legal move. CONVENTION.md §1f spells it out.
  //
  // NO `prev.state` DISCIPLINE IS NEEDED for this arm, unlike the token one it
  // replaces. `pace()` reads score, cards_left, num_players and max_score, and a
  // clue touches none of them -- so the interpreting seat and the deciding seat
  // agree whether or not `on_clue` has already run. Callers still pass
  // `prev.state`, which is harmless and keeps one discipline for every arm.
  //
  // `pace()` DOES use `max_score()`, which is live: it shrinks as criticals are
  // discarded, so unlike the old constant `5 * suits` cap the threshold can move
  // mid-game. That is safe because a clue's reading is fixed when `interpret_clue`
  // runs and is never recomputed -- a later shift cannot retroactively change what
  // a past clue meant. The old comment's worry was two seats reading the same past
  // clue differently, and they still cannot: they read it at the same moment, off
  // the same state.
  return state.pace() > 1;
}

bool colour_is_never_stable(const Variant& variant) {
  return bob_colour_joins_even(variant);
}

bool clue_is_reactive(const State& state, const ClueAction& action, int bob) {
  if (action.target != bob) return true;
  // WITH ONLY TWO CLUE COLOURS A COLOUR CLUE IS NEVER STABLE (v13.0.0).
  //
  // Two colours leave two colour anchors, so v11.5.0 gave a colour clue to Bob
  // an EVEN bucket of its own (Red 2, Blue 5). The stable exemptions below cut
  // straight across that second bucket, standing it down at 8 clues or once the
  // score passes half -- exactly where the extra bucket is worth having.
  //
  // Unlike the token arm this depends on the VARIANT alone, so it cannot read
  // differently either side of `on_clue` spending the token: giver and reader
  // agree by construction, with no `prev.state` discipline needed.
  if (action.clue.kind == ClueKind::COLOUR &&
      colour_is_never_stable(*state.variant)) {
    return true;
  }
  return bob_clue_is_reactive(state);
}

int reactive_receiver(const State& state, const ClueAction& action, int reacter) {
  if (!variants::uses_target_parity(*state.variant)) return action.target;
  // Cathy, whoever was clued. reactor0 runs at exactly three players
  // (net/commands.cpp), so the seat after the reacter is always the third one.
  return state.next_player_index(reacter);
}

std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter, int receiver) {
  hanabi::instr::ScopedTimer st("reactor0.interpret_reactive");
  hanabi::logging::LogScope ls("reactor0.interpret_reactive",
                               {{"giver", action.giver},
                                {"target", action.target},
                                {"reacter", reacter},
                                {"receiver", receiver}});
  const State& state = game.state;
  int giver = action.giver;
  const auto& clue = action.clue;

  // The clue's reactive assignment: which parity bucket it is in, and its
  // anchor within that bucket. `/set` overrides the value; with no overrides
  // this is the variant's built-in table.
  //
  // Asked with the TARGET rather than the receiver. The two are the same seat
  // outside a target-parity variant, and inside one the target is precisely
  // what decides the parity -- a clue to Bob (the reacter) is odd.
  const ReactiveAssignment assign = reactive_assignment_for(
      *state.variant, game.reactive_overrides, clue.kind, clue.value,
      /*target_is_bob=*/action.target == reacter);
  int anchor = assign.value;
  // `wc.clue.target` records the seat that was CLUED, which is not always the
  // receiver -- in a target-parity variant a clue to Bob has Cathy as receiver.
  // Every later parity lookup keys on it, so it must not be rewritten here.
  ReactorWC wc{giver,
               reacter,
               receiver,
               state.hands[receiver],
               to_clue(clue, action.target),
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
  // The frame the giver chose this clue in. A DEFERRED reaction resolves turns
  // later, against stacks that have moved, and the receiver must still read the
  // target as it was meant (§1e).
  wc.clue_play_stacks = state.play_stacks;
  game.waiting.clear();
  game.waiting.push_back(wc);

  // Rule 2: at most one pending reaction per receiver, so a fresh reactive clue
  // to this seat replaces whatever they were still owed and "the reacter's next
  // non-clue action" is never ambiguous.
  //
  // Indexed by receiver rather than held in one slot because a clue to a
  // DIFFERENT receiver must leave the old one standing -- which is exactly what
  // replay 1975464 turns on. There the deferring clue is itself reactive, so it
  // owes yagami_black a reaction while will-bot67 is still owed the original.
  if (static_cast<int>(game.pending_reactions.size()) != state.num_players) {
    game.pending_reactions.assign(state.num_players, std::nullopt);
  }
  game.pending_reactions[receiver] = std::move(wc);

  // The receiver decodes positionally at reaction time — never at clue
  // time (POV invariance: selection reads deck ids of the receiver's own
  // hand).
  if (receiver == state.our_player_index) return ClueInterp::REACTIVE;

  // Dispatch on the PARITY, not the clue kind: `reactive_rank` is the
  // even-parity ruleset (double play / double discard) and `reactive_colour`
  // the odd one (exactly one play). Odds and Evens swaps which kind carries
  // which, and `/set` can move a single clue, so the bucket is read off the
  // assignment rather than from the kind.
  auto interp = !assign.even
                    ? reactive_colour(prev, game, action, anchor, reacter,
                                      receiver)
                    : reactive_rank(prev, game, action, anchor, reacter,
                                    receiver);
  record_react_target(game);
  // An unread reactive leaves the WC pushed above in place, with
  // `react_order == -1` and no urgent stamp on any card. `take_action` reports
  // it -- see `decide.cpp`'s `reactor0.reactive_unreadable` branch, and the
  // comment there for why it cannot be reported from here.
  return interp;
}

}  // namespace hanabi::reactor0
