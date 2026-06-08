// Exception test for the "discard when a known play exists" penalty in
// eval_action (src/conventions/reactor/state_eval.cpp). The penalty must
// NOT fire when the known-play's identity is already CALLED_TO_PLAY in
// another player's hand — discarding (or cluing) is the right move there,
// since the teammate will resolve the play.
//
// Bookend: with no dupe CTP, the bot should pick its known play.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Find the order of the (slot-th) card in `player`'s hand. Slot 1 = newest.
int order_of(const Game& g, TestPlayer player, int slot) {
  const auto& hand = g.state.hands[static_cast<int>(player)];
  return hand[slot - 1];
}

// 3-player "No Variant" position: blue stack at 2 (so b3 plays); Alice
// holds b3 at slot 1 (fully known empathy → singleton inferred = b3); Bob
// holds b3 at his slot 1 with no convention status; the rest of Bob/Cathy
// hold non-playable filler. Alice acts next.
Game make_position() {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer): real b3 in slot 1, hidden filler elsewhere.
      {"b3", "xx", "xx", "xx", "xx"},
      // Bob: real b3 in slot 1, non-playable filler elsewhere.
      {"b3", "y4", "g4", "r4", "p4"},
      // Cathy: non-playable filler.
      {"y3", "g3", "y2", "g2", "p3"},
  };
  opts.variant_name = "No Variant";
  // R=0, Y=1, G=2, B=3, P=4. Blue stack at 2 → b3 playable.
  opts.play_stacks = std::vector<int>{0, 0, 0, 2, 0};
  opts.clue_tokens = 1;  // discard isn't suppressed; clue is still possible.
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));
  // Make Alice empathy-know her own b3.
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "b3");
  g.elim();
  return g;
}

}  // namespace

TEST(DiscardPenalty, PlaysKnownB3WhenDuplicateNotCTPdElsewhere) {
  Game g = make_position();
  int alice_b3 = order_of(g, TestPlayer::ALICE, 1);

  // Sanity: Alice's b3 is empathy-known.
  auto inferred = g.me().thoughts[alice_b3].id(/*infer=*/true);
  ASSERT_TRUE(inferred.has_value());
  EXPECT_EQ(inferred->suit_index, 3);
  EXPECT_EQ(inferred->rank, 3);
  // Sanity: Bob's b3 is NOT CTP'd.
  for (int o : g.state.hands[1]) {
    EXPECT_NE(g.meta[o].status, CardStatus::CALLED_TO_PLAY);
  }

  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "with no dupe CTP elsewhere, bot must play its known b3";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, alice_b3);
}

TEST(DiscardPenalty, DoesNotForcePlayWhenDuplicateB3IsCTPdInBobHand) {
  Game g = make_position();
  int alice_b3 = order_of(g, TestPlayer::ALICE, 1);
  int bob_b3 = order_of(g, TestPlayer::BOB, 1);

  // Manually CTP Bob's b3. The new helper should detect this via the
  // visible-deck branch (alice can see bob's card identity).
  g.meta[bob_b3].status = CardStatus::CALLED_TO_PLAY;

  PerformAction perform = g.take_action();
  // Penalty must be suppressed: bot should NOT race Bob's known play.
  if (std::holds_alternative<PerformPlay>(perform)) {
    EXPECT_NE(std::get<PerformPlay>(perform).target, alice_b3)
        << "bot should yield — bob's b3 is already CTP'd, so playing alice's b3 would be a duplicate";
  }
}
