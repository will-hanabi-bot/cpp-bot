// A play call the holder can see must be trash is skipped, not obeyed.
//
// A reactor0 hand can carry several CALLED_TO_PLAY cards at once, kept in
// play order newest slot -> oldest (conventions/reactor0/ctp_order.h). When
// the first of them turns out to be known trash from the holder's own
// empathy, the urgent scan in Game::take_action moves on to the next call
// rather than abandoning the urgent path and falling through to the general
// action ladder — which would strand a live play call behind a dead one.
//
// Any OTHER unactionable call (e.g. a CALLED_TO_DISCARD the holder knows is
// critical) still stops the scan, exactly as before.
#include <gtest/gtest.h>

#include <algorithm>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Call `order` to play, stamped at `turn`.
void call_to_play(Game& g, int order, int turn) {
  g.with_meta(order, [turn](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m = m.reason(turn).signal(turn);
  });
  g.with_thought(order, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });
}

}  // namespace

TEST(Reactor0UrgentSkipsKnownTrash, KnownTrashCallYieldsToTheNextCall) {
  SetupOptions opts;
  // Red stack at 2, so r1 is basic trash and b1 is playable. Alice is the
  // acting seat, so her own hand is what take_action reasons about.
  opts.play_stacks = {{2, 0, 0, 0, 0}};
  opts.hands = {
      {"r1", "b1", "g4", "p3", "y4"},   // us: slot 1 trash, slot 2 playable
      {"y2", "y3", "b3", "g4", "y4"},
      {"g3", "p2", "b4", "p4", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Slot 1 is fully known r1, which the red stack at 2 makes trash — that
  // call is dead. Slot 2 stays UNCLUED: its call is a blind play, which only
  // the urgent path will make. That is what makes the skip observable; a
  // call on a card we already know is playable would be picked up by the
  // general ladder anyway, hiding the difference.
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r1");
  // apply_pre_clue writes common knowledge only; elim propagates it into our
  // own thoughts, which is what the urgent scan reads.
  g.elim();

  int slot1 = order_at(g, TestPlayer::ALICE, 1);
  int slot2 = order_at(g, TestPlayer::ALICE, 2);

  ASSERT_TRUE(g.me().thoughts[slot1].possible.forall(
      [&](Identity i) { return g.state.is_basic_trash(i); }))
      << "fixture must make slot 1 known trash from our OWN empathy";
  ASSERT_FALSE(g.me().thoughts[slot2].possible.forall(
      [&](Identity i) { return g.state.is_basic_trash(i); }))
      << "fixture must leave slot 2 a live play call";
  {
    auto obvious = g.me().obvious_playables(g, g.state.our_player_index);
    ASSERT_TRUE(std::find(obvious.begin(), obvious.end(), slot2) ==
                obvious.end())
        << "slot 2 must be a BLIND play — if we already knew it was playable "
           "the general ladder would play it and the skip would be invisible";
  }

  // Two standing calls, in play order: slot 1 (newer, stamped later) first.
  call_to_play(g, slot2, /*turn=*/0);
  call_to_play(g, slot1, /*turn=*/1);

  PerformAction action = g.take_action();

  auto* play = std::get_if<PerformPlay>(&action);
  ASSERT_NE(play, nullptr)
      << "the live play call must still be actioned, not abandoned — "
         "abandoning the urgent path strands a blind play the ladder will "
         "never make";
  EXPECT_EQ(play->target, slot2)
      << "the dead call on slot 1 must be skipped and the next call down "
         "played";
}

TEST(Reactor0UrgentSkipsKnownTrash, LiveCallOnTheNewestSlotIsStillTakenFirst) {
  SetupOptions opts;
  opts.play_stacks = {{2, 0, 0, 0, 0}};
  opts.hands = {
      {"r3", "b1", "g4", "p3", "y4"},   // us: slot 1 = r3, playable
      {"y2", "y3", "b3", "g4", "y4"},
      {"g3", "p2", "b4", "p4", "r4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Both live: r3 plays on a red stack of 2, b1 on an empty blue stack.
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r3");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "b1");
  // apply_pre_clue writes common knowledge only; elim propagates it into our
  // own thoughts, which is what the urgent scan reads.
  g.elim();

  int slot1 = order_at(g, TestPlayer::ALICE, 1);
  int slot2 = order_at(g, TestPlayer::ALICE, 2);

  call_to_play(g, slot2, /*turn=*/0);
  call_to_play(g, slot1, /*turn=*/1);

  PerformAction action = g.take_action();

  auto* play = std::get_if<PerformPlay>(&action);
  ASSERT_NE(play, nullptr);
  EXPECT_EQ(play->target, slot1)
      << "with both calls live the most recently stamped one — kept on the "
         "newest slot by the play-order invariant — goes first";
}
