// Under reactor0, giving a clue does not cancel your own pending reacter call.
//
// `Game::interpret_clue` opens with `check_missed(action.giver, 99)`
// (basics/decide.cpp), which scans the GIVER's own hand for an urgent card and
// clears it -- stamp, `inferred` narrowing and `info_lock` all reverted
// (basics/game.cpp:95-118). That is reactor's rule: a clue instead of a reaction
// means the chain broke.
//
// Reactor0 is exempt, because there a reacter is ALLOWED to defer. Its
// Precedence puts an H4 clue above the urgent return, so a clue is exactly what
// a legitimate deferral looks like, and the call has to survive it and be
// actioned on a later turn.
//
// Replay 1966745: will-bot69 was stamped an urgent CTD on an Orange 1 with the
// orange stack on 0, deferred at T2 to give a finesse, and lost the call. At T5
// `requires_high_tier` therefore reported it unoccupied, the unoccupied MEDIUM
// gate let a non-HIGH clue through instead of the chuck, and the team struck on
// that same card at T16.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Cathy gets the reactive, so BOB is the reacter. Bob is us on the next turn,
// which is the seat that has to keep its call across a clue.
//
// Stacks r=1 leave Cathy with no playable and no one-away, so rank 4 to Cathy
// is a Phase C double discard: anchor 4, dc-target slot 3, react_slot
// (4 + 5 - 3) % 5 = 1. Bob's slot 1 is stamped CTD and urgent.
SetupOptions defer_opts() {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"y2", "y3", "g2", "g3", "b2"},  // Alice (us, the giver)
      {"y2", "y3", "b3", "g4", "y4"},  // Bob   (reacter)
      {"y4", "b4", "r1", "g4", "p4"},  // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

// The same position rotated so the REACTER is us. `take_action` decides for
// `our_player_index` (Alice), not for `current_player_index`, so a test that
// asks what the reacter does has to put the reacter in Alice's seat.
//
// Alice is the reacter when the giver is the player before her: Cathy clues her
// own third seat, which is Bob. Same three hands as above, rotated one seat.
SetupOptions defer_opts_reacter_is_us() {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.starting = TestPlayer::CATHY;
  opts.hands = {
      {"y2", "y3", "b3", "g4", "y4"},  // Alice (us, the REACTER)
      {"y4", "b4", "r1", "g4", "p4"},  // Bob   (receiver)
      {"y2", "y3", "g2", "g3", "b2"},  // Cathy (giver)
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0DeferralKeepsCall, AClueDoesNotCancelTheGiversOwnReacterCall) {
  Game g = setup(defer_opts());
  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  const int called = order_at(g, TestPlayer::BOB, 1);
  ASSERT_EQ(g.meta[called].status, CardStatus::CALLED_TO_DISCARD);
  ASSERT_TRUE(g.meta[called].urgent) << "guard: this is a REACTER call";
  const IdentitySet promised = g.common.thoughts[called].inferred;

  // Bob defers: instead of chucking, he gives a clue of his own.
  g = take_turn(std::move(g), "Bob clues 1 to Cathy");

  EXPECT_EQ(g.meta[called].status, CardStatus::CALLED_TO_DISCARD)
      << "the call stands until it is ACTIONED -- a deferral is not a miss";
  EXPECT_TRUE(g.meta[called].urgent) << "and it is still the reacter's";
  EXPECT_EQ(g.common.thoughts[called].inferred, promised)
      << "check_missed also reverts `inferred` to old_inferred; that must not "
         "happen either";
}

// The companion: reactor's rule is untouched, and the guard is the only thing
// that differs. Stamping the urgent call by hand rather than through a clue is
// deliberate -- reactor reads this position differently from reactor0, and what
// is under test is `interpret_clue`'s guard, not reactor's clue reading.
TEST(Reactor0DeferralKeepsCall, ReactorStillCancelsOnDeferral) {
  SetupOptions opts = defer_opts();
  opts.init = nullptr;  // leave the convention at reactor
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  // `check_missed` throws without `old_inferred`, so set it the way a real
  // reacter call would have.
  g.with_thought(mine, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });
  g.with_meta(mine, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.urgent = true;
    m = m.reason(0).signal(0);
  });

  g = take_turn(std::move(g), "Alice clues 4 to Cathy");

  EXPECT_FALSE(g.meta[mine].urgent)
      << "reactor's cancel-on-defer rule is deliberate and must stay";
}

// The consequence the bug report was actually about: with the call alive, the
// holder is OCCUPIED, so the HIGH-tier gate applies and phase 2's urgent return
// chucks the called slot rather than letting a clue through.
TEST(Reactor0DeferralKeepsCall, TheSurvivingCallIsActionedOnTheNextTurn) {
  Game g = setup(defer_opts_reacter_is_us());
  g = take_turn(std::move(g), "Cathy clues 4 to Bob");
  const int called = order_at(g, TestPlayer::ALICE, 1);
  ASSERT_EQ(g.meta[called].status, CardStatus::CALLED_TO_DISCARD);
  ASSERT_TRUE(g.meta[called].urgent) << "guard: we are the reacter";

  // We defer, then the table comes back round to us.
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  g = take_turn(std::move(g), "Bob discards p4 (slot 5)", "r2");
  g = take_turn(std::move(g), "Cathy discards b2 (slot 5)", "r2");
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE))
      << "guard: it is the deferring reacter's turn again";
  ASSERT_TRUE(g.meta[called].urgent) << "guard: the call is still standing";
  // Zero tokens, so no clue can be offered at all. The Precedence puts an H4
  // clue ABOVE the urgent return, so a second deferral would be legitimate and
  // would say nothing about the call; this isolates the question being asked,
  // which is whether the surviving call gets actioned.
  g.state.clue_tokens = 0;

  PerformAction action = g.take_action();

  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(action))
      << "a standing reacter-CTD is chucked, not deferred a second time";
  EXPECT_EQ(std::get<PerformDiscard>(action).target, called);
}
