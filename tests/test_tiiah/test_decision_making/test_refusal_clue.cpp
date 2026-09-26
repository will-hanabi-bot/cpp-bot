// GIVING the refusal (tiiah/CONVENTION.md §1c, v16.14.0).
//
// Alice reads her own reactive against her own stacks, and those are stale in
// exactly one way: she cannot see what she threw in the hole. So she can name a
// card of Cathy's that is already down. We are Bob, we can see it, and the
// convention's way to say so is to give Cathy a STABLE clue instead of reacting.
//
// The reading half is pinned by
// `tests/test_tiiah/test_replay_2009367_stable_clue_to_cathy_refuses_the_reactive.cpp`;
// this is the half where the bot has to produce the move. The standing reactive
// is built directly rather than choreographed through four turns of clue-giving:
// what is under test is the decision, and the interpretation that would have
// left this connection is covered by the replay.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// The harness always seats us at index 0, and the reacter is the seat AFTER the
// giver — so the giver is index 2 and the receiver index 1.
constexpr int kUs = 0;      // Bob, the reacter
constexpr int kReceiver = 1;
constexpr int kGiver = 2;

// Red 1 is already down. The receiver holds the other one on slot 3 — trash,
// which we can see and the giver cannot, because the first r1 was its own hole
// play.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // us
      {"y1", "g1", "r1", "b2", "p2"},  // the receiver; slot 3 is the dead r1
      {"y2", "g2", "b1", "p1", "r4"},  // the giver
  };
  opts.play_stacks = std::vector<int>{1, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

// A standing ordinary reactive from the giver, naming `target_order` in the
// receiver's hand, with the urgent call on OUR slot that answering it would mean
// playing.
Game with_standing_reactive(Game g, int target_order, int our_slot) {
  ReactorWC wc;
  wc.giver = kGiver;
  wc.reacter = kUs;
  wc.receiver = kReceiver;
  wc.receiver_hand = g.state.hands[kReceiver];
  wc.clue = Clue{ClueKind::RANK, 1, kReceiver};
  wc.focus_slot = 1;
  wc.inverted = false;
  wc.turn = g.state.turn_count;
  wc.even_parity = true;
  wc.receiver_target_order = target_order;
  const int react_order = g.state.hands[kUs][our_slot - 1];
  wc.react_order = react_order;
  g.waiting.push_back(wc);
  if (static_cast<int>(g.pending_reactions.size()) == g.state.num_players) {
    g.pending_reactions[kReceiver] = wc;
  }
  // The call the refusal has to outrank: answering the reaction means playing
  // this card, and we are declining to.
  g.with_meta(react_order, [&g](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m.by = kGiver;
    m = m.signal(g.state.turn_count);
  });
  return g;
}

}  // namespace

// The refusal outranks our own pending reaction, because refusing is done
// INSTEAD of reacting: it comes in at Precedence step 1 rather than as a rung,
// which would have sat below the urgent return and never fired.
TEST(TiiahRefusalClue, WeClueCathyRatherThanAnswerADeadReactive) {
  Game g = setup(opts_for());
  const int dead = order_at(g, TestPlayer::BOB, 3);  // the receiver's spare r1
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 1}))
      << "red 1 is down, so the card the giver named is behind the stacks";
  g = with_standing_reactive(std::move(g), dead, /*our_slot=*/2);

  const PerformAction action = g.take_action();
  const auto* rank = std::get_if<PerformRank>(&action);
  const auto* colour = std::get_if<PerformColour>(&action);
  ASSERT_TRUE(rank || colour)
      << "we refuse with a clue, not by answering a reaction whose target has "
         "already been played";
  EXPECT_EQ(rank ? rank->target : colour->target, kReceiver)
      << "and the clue goes to the RECEIVER -- that is what makes it the signal "
         "rather than an ordinary deferral";
}

// The control. Same position, except the card the giver named is genuinely
// playable — so there is nothing to refuse and our urgent call stands.
TEST(TiiahRefusalClue, ALiveTargetIsNotRefused) {
  SetupOptions opts = opts_for();
  // Slot 3 is now a y1, which red-1-is-down says nothing about.
  opts.hands[kReceiver] = {"g1", "b1", "y1", "b2", "p2"};
  Game g = setup(std::move(opts));
  const int live = order_at(g, TestPlayer::BOB, 3);
  ASSERT_FALSE(g.state.is_basic_trash(Identity{1, 1}));
  g = with_standing_reactive(std::move(g), live, /*our_slot=*/2);

  const PerformAction action = g.take_action();
  const auto* play = std::get_if<PerformPlay>(&action);
  ASSERT_NE(play, nullptr)
      << "nothing is wrong with the pairing, so the urgent call is answered";
  EXPECT_EQ(play->target, g.state.hands[kUs][1])
      << "which is the card the reactive stamped";
}
