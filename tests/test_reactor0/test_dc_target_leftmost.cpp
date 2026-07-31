// reactor0's dc-target is the LEFTMOST trash or same-hand dupe, always.
//
// Unlike reactor, which reorders and filters its dc-target pool, reactor0
// selects on hand position alone: neither an existing CALLED_TO_DISCARD nor
// cluedness nor a second copy further right can move the target. The receiver
// derives it from position, so anything else would desync.
//
// The bug this pins (finding #5): the pool used to skip cards already CTD'd
// on the grounds that re-CTD conveys nothing. That is a preference about
// which card to NAME, not evidence about what the hand CONTAINS -- and it was
// consulted before deciding whether the pool was empty. If the receiver's
// only trash was already CTD'd, the pool came back empty and the rlocks
// branch invented a whole-hand lock, chop-moving over the correct CTD, while
// the receiver plainly did hold trash.
#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Red stack at 1, so a red 1 in Cathy's hand is basic trash. Green = anchor
// 3, so a dc-target on Cathy's slot 2 maps to react_slot calc_slot(3,2,5) = 1
// -- Bob's slot 1, which holds a visibly playable y1 for the blind play.
SetupOptions dc_target_opts(std::vector<std::string> cathy) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g4", "y3", "b3", "p4"},   // Bob: slot 1 = y1, playable
      std::move(cathy),
  };
  use_reactor0(opts, /*rlocks=*/true);
  return opts;
}

}  // namespace

TEST(Reactor0DcTargetLeftmost, AlreadyCtdTrashDoesNotFabricateALock) {
  //                         slot 2 = r1, trash; rest good, unique, unplayable
  Game g = setup(dc_target_opts({"y4", "r1", "g3", "p3", "b4"}));

  // That trash was already called to discard by an earlier reactive.
  g.with_meta(order_at(g, TestPlayer::CATHY, 2),
              [](ConvData& m) { m.status = CardStatus::CALLED_TO_DISCARD; });

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE)
      << "the standing CTD must not remove slot 2 from consideration and "
         "push the clue into the lock reading";
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 1))
      << "slot 2 stays the dc-target, so the react slot is 1";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED))
      << "the receiver holds trash, so the reactive lock must not fire";
}

TEST(Reactor0DcTargetLeftmost, ASecondCopyFurtherRightDoesNotMoveTheTarget) {
  //                         r1 at slot 2 AND slot 4 -- leftmost still wins
  Game g = setup(dc_target_opts({"y4", "r1", "g3", "r1", "b4"}));

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 1))
      << "the target is the LEFTMOST trash (slot 2); a duplicate further "
         "right must not shift it to react_slot 4";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED));
}
