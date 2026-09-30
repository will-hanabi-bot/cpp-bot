// A named stable call on a card the receiver WATCHED the giver play into the hole
// (tiiah/CONVENTION.md §2c's exception, v18.18.0).
//
// The giver may now call a card it cannot rule out having played itself, provided
// the receiver can name the call exactly: the receiver watched the giver's hole card
// go in, so it can see a dupe for itself and throw it. Human diagnostic
// v18_human_vs_bot_diagnostics/2014561.md T56: "yagami_black will toss it if
// yagami_blue already played the other copy." This is the receiver's half.
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Yellow on 3. Cathy throws her y4 into the hole unnamed -- we watch it land -- and
// two turns later gives us Yellow on our slot 2. On the frame she and we share,
// yellow is still on 3, so the call reads as the y4; we know that card is down.
Game position(const char* cathys_hole_card = "y4") {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{0, 3, 0, 0, 0};
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r4", "g4", "b4", "p4", "r3"},
                {cathys_hole_card, "g3", "b3", "p3", "r5"}};
  opts.starting = TestPlayer::CATHY;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "b2");
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "b1");
  return take_turn(std::move(g), "Cathy clues yellow to Alice (slot 2)");
}

}  // namespace

TEST(TiiahNamedCallOnAWatchedDupe, TheReceiverReadsTheDupeAndDoesNotPlayIt) {
  Game g = position();
  const int called = order_at(g, TestPlayer::ALICE, 2);
  ASSERT_EQ(g.state.play_stacks[1], 4) << "guard: we watched the y4 land";

  EXPECT_EQ(g.players[0].thoughts[called].possibilities(),
            IdentitySet::single(Identity{1, 4}))
      << "Cathy's frame names the y4, and we watched her play the y4";
  EXPECT_NE(g.meta[called].status, CardStatus::CALLED_TO_PLAY)
      << "so the call is withdrawn: it can only be the dupe";
  const auto trash = g.players[0].thinks_trash(g, 0);
  EXPECT_NE(std::find(trash.begin(), trash.end(), called), trash.end())
      << "it is known trash, ours to throw";

  const PerformAction a = g.take_action();
  const auto* play = std::get_if<PerformPlay>(&a);
  EXPECT_FALSE(play && play->target == called) << "never played into a strike";
}

// The control: Cathy's hole card was a b1, so the y4 her Yellow names is live and
// the call stands.
TEST(TiiahNamedCallOnAWatchedDupe, ALiveNamedCallStands) {
  Game g = position("b1");
  const int called = order_at(g, TestPlayer::ALICE, 2);
  ASSERT_EQ(g.state.play_stacks[1], 3) << "guard: yellow is still on 3";
  EXPECT_EQ(g.meta[called].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.players[0].thoughts[called].possibilities().contains(Identity{1, 4}));
}
