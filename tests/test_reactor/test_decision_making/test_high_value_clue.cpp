// Decision-quality tests for the v0.34 high-value-clue gate. When the
// bot has a pending play (an unduplicated obvious-playable in its own
// hand), `state.clue_tokens < 3`, and `state.pace() >= 3`, the gate
// only lets through clues that satisfy the strict "high value"
// definition (state_eval.cpp::is_high_value_clue). Three conditions
// — any ONE makes a clue high-value:
//
//   (1) Bob is not locked AND has no safe discard (no obvious play,
//       no known trash, no CTD) AND has a unique non-trash chop card
//       (no visible same-id in Cathy's hand and no singleton-inferred
//       same-id in the giver's own hand).
//   (2) The clue gets a critical "low" card played — rank 1 or 2 on
//       a normal suit, rank 4 or 5 on a reversed suit.
//   (3) The clue gets ≥ 2 new plays AND at least one of them is the
//       clue-regain rank (5 on normal, 1 on reversed).
//
// Each test calls `hanabi::reactor::eval_action(g, clue_action)`
// directly with a hand-crafted ClueAction and inspects the returned
// value. The gate's rejection signature is exactly `-1.0`; anything
// else means the gate passed (the clue may still be valued lower
// than a play depending on `get_result`, but the gate didn't veto).
#include <gtest/gtest.h>

#include <stdexcept>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

int order_of(const Game& g, TestPlayer p, int slot) {
  return g.state.hands[static_cast<int>(p)][slot - 1];
}

// Build a ClueAction that touches every card a given (kind, value)
// would touch on `target` in the current state.
//
// Refuses an empty touch list, mirroring the shared harness
// (`tests/test_harness.cpp:256-258`). This helper used to build such a clue
// silently, which is how a fixture ended up asserting against a clue that
// touches nothing — an illegal move that `try_stable` can only read as a
// MISTAKE, so the test was measuring the wrong rejection.
Action make_clue(const Game& g, int giver, int target, ClueKind kind,
                int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
  if (touched.empty()) {
    throw std::invalid_argument("No cards touched by clue");
  }
  return Action{ClueAction{giver, target, std::move(touched),
                              BaseClue{kind, value}}};
}

// Verifies the gate's gating preconditions are met (low ct + high
// pace + Alice has a real pending play).
void expect_gate_preconditions(const Game& g) {
  ASSERT_LT(g.state.clue_tokens, 3) << "guard: low clue count";
  ASSERT_GE(g.state.pace(), 3) << "guard: high pace";
  auto plays = g.me().obvious_playables(g, g.state.our_player_index);
  ASSERT_FALSE(plays.empty())
      << "guard: Alice must have a pending play for the gate to fire";
}

}  // namespace

// Condition (2): a clue that gets a critical first-rank (rank 1)
// played is high-value even when Bob has a safe discard. Bob's slot 5
// is pre-stamped CTD so condition (1) cannot fire; only condition (2)
// can save the clue.
TEST(HighValueClueGate, AllowsClueWithCriticalOneToPlay) {
  SetupOptions opts;
  opts.hands = {
      // Alice (POV): slot 1 = y1 (fully_known → pending play).
      {"y1", "r3", "r4", "g4", "p4"},
      // Bob: slot 1 = r1 (critical with 2 r1 discarded). Other slots
      // fillers chosen so rank-1 only touches slot 1.
      {"r1", "g2", "p2", "y3", "g3"},
      {"y3", "r3", "b3", "g4", "p4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.discarded = {"r1", "r1"};  // makes Bob's slot-1 r1 critical.
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  // Give Bob a safe discard so condition (1) cannot fire; only
  // condition (2) can make the rank-1 clue high-value.
  g.meta[order_of(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  expect_gate_preconditions(g);
  ASSERT_TRUE(g.state.is_critical(Identity{0, 1}))
      << "test setup: r1 must be critical (2 discarded + Bob holds 1)";

  Action clue = make_clue(g, /*giver=*/0, /*target=*/1, ClueKind::RANK, 1);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "rank-1 to Bob CTPs his critical r1 → condition (2) fires → "
         "gate must allow. Got eval=" << v;
}

// Condition (2): same as above but rank 2 (critical r2 → first-or-
// second rank in normal play direction).
//
// Two things this fixture has to get right, both of which it originally got
// wrong:
//
//   * **Every stack sits at 1**, so that every non-trash rank-2 identity is
//     playable and the clue is a genuine *playable rank* — a direct play call
//     (`interpret_clue.cpp:469-509`). At stacks {1,1,0,0,0} the unplayable
//     g2/b2/p2 made `playable_rank` false, the clue degraded to a referential
//     discard, nothing was ever CTP'd, and condition (2) could not fire.
//   * **Bob's chop is trash**, which is what disables condition (1) so that
//     only condition (2) can save the clue. Bob's chop is his *newest* unclued
//     card — slot 1, not slot 5 (`Game::chop`, `decide.cpp:453-458`).
TEST(HighValueClueGate, AllowsClueWithCriticalTwoToPlay) {
  SetupOptions opts;
  opts.hands = {
      {"y2", "r4", "g4", "b4", "p4"},
      // Bob: slot 1 = r1, trash on a red stack of 1, so the chop is not
      // worth saving and condition (1) is dead. Slot 2 = the critical r2,
      // and it is his only rank-2 so the clue focuses it.
      {"r1", "r2", "g3", "y3", "p3"},
      {"y3", "r4", "b3", "g3", "p4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  // Every stack at 1: rank 2 is a playable rank, and Alice's y2 is playable.
  opts.play_stacks = {1, 1, 1, 1, 1};
  opts.discarded = {"r2"};  // 1 r2 discarded → Bob's r2 is critical.
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y2");
  g.elim();

  expect_gate_preconditions(g);
  ASSERT_TRUE(g.state.is_critical(Identity{0, 2}));
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 1}))
      << "test setup: Bob's chop must be trash so condition (1) cannot fire";

  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 2);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "rank-2 to Bob CTPs his critical r2 (first/second-rank in "
         "normal play direction) → condition (2) fires. Got eval="
      << v;
}

// Condition (3) positive: a clue that causes 2 plays, one of which is a 5
// (the clue-regain rank for normal suits), passes the gate even when
// conditions (1) and (2) don't fire.
//
// This has to be a REACTIVE RANK finesse. Two reasons the original stable
// colour clue could never work:
//
//   * a stable colour clue is a referential play with exactly ONE target
//     (`ref_play`, `interpret_clue.cpp:276-313`) — "red promises r4 and chains
//     r5 behind it" is not a reading reactor has, which is why the original
//     fixture resolved inconsistently and scored -100 as a MISTAKE;
//   * the finesse fallback lives only in `interpret_reactive_rank`
//     (`interpret_reactive.cpp:789-875`). Colour reactives have no finesse
//     branch at all, so a red clue here reaches a different path entirely.
//
// So: Alice clues **5** to CATHY, which dispatches reactive with Bob as
// reacter (`decide.cpp:202`) and falls through to the finesse fallback,
// because Cathy has no *currently* playable card — only her r5, one away on a
// red stack of 3. The clue touches only her slot 5, so `reactive_focus` is 5.
// The fallback walks react_slot in the fixed order {1, 5, 4, 3, 2}: react 1
// maps to target slot 4 (a p3, two away — not a finesse target) and is
// skipped; react 5 maps to target slot 5, the r5. So Bob's slot 5 — the r4 —
// is the called card, which conveniently leaves his slot 1 free to be the
// trash chop that disables condition (1).
//
// The finesse stamps only the reacter at clue time; Cathy's r5 is stamped a
// turn later when the reaction resolves. `is_high_value_clue` credits that
// promised play, so the pair counts as two.
TEST(HighValueClueGate, AllowsFinesseWithTwoPlaysIncludingFive) {
  SetupOptions opts;
  opts.hands = {
      // Alice: fully_known y1 on slot 1 is her pending play (y stack 0).
      {"y1", "g4", "b4", "p4", "y4"},
      // Bob (reacter): slot 1 = r1, trash on a red stack of 3, so his chop is
      // not worth saving and condition (1) is dead. Slot 5 = the r4 the
      // finesse points at.
      {"r1", "g3", "b3", "p3", "r4"},
      // Cathy (receiver): r5 on slot 5 is her ONLY one-away-from-playable
      // card and her only red, so the clue focuses it.
      {"y3", "g3", "b3", "p3", "r5"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {3, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.elim();

  expect_gate_preconditions(g);
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 1}))
      << "test setup: Bob's chop must be trash so condition (1) cannot fire";
  ASSERT_EQ(g.state.playable_away(Identity{0, 5}), 1)
      << "test setup: Cathy's r5 must be exactly one away to be a finesse "
         "target";

  Action clue = make_clue(g, /*giver=*/0, /*target=*/2, ClueKind::RANK, 5);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "the finesse calls Bob's r4 now and promises Cathy's r5 on the "
         "reaction — 2 plays incl. a rank-5 (clue-regain) → condition (3) "
         "fires. Got eval=" << v;
}

// Condition (3) negative: 2 new plays but NEITHER is a rank-5 → the
// regain-clue check fails and the gate rejects (no other condition
// holds because Bob is safe and no critical 1/2 is touched).
TEST(HighValueClueGate, RejectsClueWithTwoPlaysNoFive) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "b4", "p4"},
      // Bob: slot 1 = r2, slot 2 = r3 (both playable on r=1). Neither
      // is rank 5 → condition (3) won't fire. r2 and r3 aren't
      // critical (multiple copies) → condition (2) won't fire.
      {"r2", "r3", "g3", "p3", "y3"},
      {"y2", "g4", "b3", "p4", "y3"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  // Give Bob a safe discard so condition (1) can't save.
  g.meta[order_of(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  expect_gate_preconditions(g);

  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Red=*/0);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_EQ(v, -1.0)
      << "colour-red CTPs r2 + r3 (≥ 2 plays) but neither is rank-5, "
         "so condition (3) fails. Bob has a CTD'd safe discard so (1) "
         "fails. No critical 1/2 touched so (2) fails. Gate must "
         "reject. Got eval=" << v;
}

// Condition (1) positive: Bob is not locked, has no safe discard,
// and his chop is a non-trash card whose identity is unique (no copy
// in Cathy's hand, no singleton-inferred copy in Alice's hand). The
// clue itself doesn't need to do anything productive — just keeping
// Bob from discarding the unique chop is sufficient.
//
// The chop is Bob's NEWEST unclued card — slot 1, not slot 5 (`Game::chop`,
// `decide.cpp:432-459`) — so the b3 the fixture intends as "the unique chop"
// has to sit there. On slot 5 the real chop was Bob's r3, which Cathy also
// holds, so `chop_id_is_unique` failed and condition (1) never fired.
TEST(HighValueClueGate, AllowsClueWithUniqueGoodChop) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "p4", "b4"},
      // Bob: chop (slot 1) is the b3 — non-trash at empty stacks, and the
      // only copy anyone can see. The rest is filler that leaves Bob with
      // no obvious play, no known trash and no CTD, so he has no safe
      // discard.
      {"b3", "r3", "g3", "p3", "y3"},
      // Cathy: NO copy of Bob's chop (b3).
      {"y2", "r3", "g3", "p3", "p4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.elim();

  expect_gate_preconditions(g);
  {
    auto bob_chop_id = g.state.deck[order_of(g, TestPlayer::BOB, 1)].id();
    ASSERT_TRUE(bob_chop_id.has_value());
    ASSERT_EQ(bob_chop_id->suit_index, 3);
    ASSERT_EQ(bob_chop_id->rank, 3);
  }

  // Any clue Alice can give Bob — even one that does nothing productive —
  // must pass the gate, because condition (1) reads the *pre-clue* state
  // (`is_high_value_clue` takes Bob's chop from `game`, not `hypo`) and is
  // therefore satisfied by the hand shape alone. Rank-3 → Bob is the
  // representative: every card Bob holds is a rank 3, so it touches all five
  // slots and produces no plays whatsoever. Conditions (2) and (3) are dead;
  // only (1) can carry it.
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 3);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "Bob has no safe discard and a unique non-trash chop (b3); "
         "condition (1) must let the gate pass any clue to Bob. Got "
         "eval=" << v;
}

// Condition (1) negative: Bob's chop is non-trash but Cathy holds a
// visible copy of the same identity. Bob discarding his copy isn't a
// loss for the team (Cathy still has one), so condition (1)'s
// "unique" requirement fails.
//
// The chop is Bob's NEWEST unclued card — slot 1 (`Game::chop`,
// `decide.cpp:453-458`) — so the b3 has to sit there. The original fixture put
// it on slot 5 and the premise held only by accident: Cathy's `r3` happened to
// dupe the *real* chop, Bob's slot-1 r3.
TEST(HighValueClueGate, RejectsClueWithDupedChop) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "p4", "p3"},
      {"b3", "r3", "g3", "p3", "y3"},  // Bob chop = slot 1 = b3.
      // Cathy holds b3 as well — Bob's chop b3 is no longer unique.
      {"y2", "r3", "g3", "b3", "p4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.elim();

  expect_gate_preconditions(g);

  // Colour-blue → Bob touches only the b3 on slot 1, his chop. Reactor has no
  // save clues, so nothing here rescues the clue; what matters is that
  // condition (1) fails because b3 is duped in Cathy.
  //
  // `EXPECT_LE` rather than `EXPECT_EQ`: the assertion is that the clue is
  // rejected, not *which* mechanism rejects it. Anything at or below the
  // gate's -1.0 is a rejection — a MISTAKE reading scores -100, which is a
  // harder rejection, and pinning the exact channel made this test fail while
  // its premise held.
  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Blue=*/3);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_LE(v, -1.0)
      << "Bob's chop b3 is duped in Cathy → condition (1)'s uniqueness check "
         "fails. No critical low-rank played, no 2-plays-incl-5 → conditions "
         "(2) and (3) also fail. Gate must reject. Got eval=" << v;
}

// Catch-all negative: Bob has a CTD'd safe discard, the clue
// produces no plays (touches nothing meaningful), no critical play.
// All three conditions fail → reject.
TEST(HighValueClueGate, RejectsClueWhenBobSafeNoConditionsMet) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "p4", "b4"},
      {"r3", "g3", "p3", "y3", "b3"},
      {"y3", "g3", "b3", "p3", "y4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.meta[order_of(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  expect_gate_preconditions(g);

  // Colour-yellow touches Bob's y3 on slot 4 — unplayable at empty stacks, so
  // the clue still produces 0 plays and no critical rank, and Bob is safe via
  // the CTD. All three conditions fail. (The comment here used to claim "Bob
  // has no yellow", which the fixture contradicts; the clue was fine, the
  // description was not.)
  //
  // `EXPECT_LE` for the same reason as the previous test: assert that the clue
  // is rejected, not which mechanism rejects it.
  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Yellow=*/1);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_LE(v, -1.0)
      << "Bob safe (CTD'd chop), no plays from the clue, no critical "
         "rank touched → all conditions fail. Gate must reject. Got "
         "eval=" << v;
}
