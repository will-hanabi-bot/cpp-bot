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
Action make_clue(const Game& g, int giver, int target, ClueKind kind,
                int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
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
TEST(HighValueClueGate, AllowsClueWithCriticalTwoToPlay) {
  SetupOptions opts;
  opts.hands = {
      {"y2", "r4", "g4", "b4", "p4"},
      // Bob: slot 1 = r2 (critical with 1 r2 discarded). r1 already
      // played so r2 is currently playable.
      {"r2", "g3", "p3", "y3", "g4"},
      {"y3", "r4", "b3", "g3", "p4"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  // r stack at 1 so r2 is playable; y stack at 1 so Alice's y2 is
  // playable too.
  opts.play_stacks = {1, 1, 0, 0, 0};
  opts.discarded = {"r2"};  // 1 r2 discarded → Bob's r2 is critical.
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y2");
  g.meta[order_of(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  expect_gate_preconditions(g);
  ASSERT_TRUE(g.state.is_critical(Identity{0, 2}));

  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 2);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "rank-2 to Bob CTPs his critical r2 (first/second-rank in "
         "normal play direction) → condition (2) fires. Got eval="
      << v;
}

// Condition (3) positive: a clue that gets 2 new plays, one of which
// is a 5 (the clue-regain rank for normal suits), passes the gate
// even when conditions (1) and (2) don't fire.
TEST(HighValueClueGate, AllowsClueWithTwoPlaysIncludingFive) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r3", "g3", "b3", "p3"},
      // Bob: slot 1 = r5 (rank 5 → clue-regain rank). slot 2 = r4.
      // With r stack=3, rank-4 clue touches slot 2 (r4) and slot 1
      // is not a rank-4. Actually we want both to be CTP'd by the
      // colour-red clue so the stable interp sees a focus + chain.
      // For 2 plays via a single clue we use colour-red touching r4
      // (playable on r=3) and r5 (chain).
      {"r5", "r4", "g3", "b3", "p3"},
      {"y2", "g4", "b4", "p4", "y3"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {3, 0, 0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.meta[order_of(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  expect_gate_preconditions(g);

  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Red=*/0);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_GT(v, -1.0)
      << "colour-red to Bob CTPs r4 (playable on r=3) with r5 chained; "
         "≥ 2 plays incl. a rank-5 (clue-regain) → condition (3) fires. "
         "Got eval=" << v;
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
TEST(HighValueClueGate, AllowsClueWithUniqueGoodChop) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "p4", "b4"},
      // Bob: filler that ensures chop (slot 5) is a non-trash card
      // and Bob has no safe discard / CTD.
      {"r3", "g3", "p3", "y3", "b3"},
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
    auto bob_chop_id = g.state.deck[order_of(g, TestPlayer::BOB, 5)].id();
    ASSERT_TRUE(bob_chop_id.has_value());
    ASSERT_EQ(bob_chop_id->suit_index, 3);
    ASSERT_EQ(bob_chop_id->rank, 3);
  }

  // Any clue Alice can give Bob — even one that does nothing — must
  // pass the gate because condition (1) is satisfied unconditionally
  // by the hand shape. We use rank-3 → Bob (touches slot 1 r3, slot 2
  // g3, slot 4 y3, slot 5 b3) as a representative.
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
TEST(HighValueClueGate, RejectsClueWithDupedChop) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "r4", "g4", "p4", "p3"},
      {"r3", "g3", "p3", "y3", "b3"},  // Bob chop = b3.
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

  // Colour-blue → Bob touches only b3 on slot 5 (Bob's chop). The
  // convention may save it, but that's not productive enough on its
  // own for the gate; what matters here is that condition (1) fails
  // because b3 is duped in Cathy.
  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Blue=*/3);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_EQ(v, -1.0)
      << "Bob's chop b3 is duped in Cathy (slot 4) → condition (1)'s "
         "uniqueness check fails. No critical low-rank played, no "
         "2-plays-incl-5 → conditions (2) and (3) also fail. Gate "
         "must reject. Got eval=" << v;
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

  // Colour-yellow → Bob touches nothing (Bob has no yellow). 0 plays,
  // no save effect, Bob safe → all three conditions fail.
  Action clue = make_clue(g, 0, 1, ClueKind::COLOUR, /*Yellow=*/1);
  double v = hanabi::reactor::eval_action(g, clue);
  EXPECT_EQ(v, -1.0)
      << "Bob safe (CTD'd chop), no plays from the clue, no critical "
         "rank touched → all conditions fail. Gate must reject. Got "
         "eval=" << v;
}
