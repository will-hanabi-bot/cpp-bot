// TIIAH's ORDINARY reactive, and the dispatch table it belongs to
// (tiiah/CONVENTION.md §1c).
//
// The convention runs BOTH dispatches. reactor0's positional one — a clue to
// Cathy is reactive, Bob reacts — and the reverse, which takes over only while
// Bob holds a known play and Cathy does not. The POSITION is the switch:
//
//   position | clue to Bob                       | clue to Cathy
//   ---------|----------------------------------|--------------------------
//   holds    | REACTIVE, Cathy reacts           | stable
//   else     | stable                           | REACTIVE, Bob reacts
//
// Until v16.8.0 only the top-left square existed and everything else fell to
// the stable ladders, so an ordinary reactive could not be read at all — replay
// 2008177 T3, where a double play read as a lock.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// Replay 2008177 T3, to the card. Green is on 1; Cathy's only playable is the
// b1 on her slot 4, and Bob's slot 3 is the r1 the sum rule will name.
SetupOptions replay_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b4", "b5", "r1", "y1", "p3"},
      {"y4", "b3", "y5", "b1", "y2"},
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 1, 0, 0};
  opts.clue_tokens = 6;
  use_tiiah(opts);
  return opts;
}

}  // namespace

// Nobody holds a known play, so the position does not flip and a clue to CATHY
// is the reactive one, with Bob answering it.
TEST(TiiahOrdinaryReactive, AClueToCathyIsReactiveWithBobReacting) {
  Game g = setup(replay_opts());

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE)
      << "it read as something else entirely — a lock, in the live game";
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, 1) << "Bob reacts";
  EXPECT_EQ(g.waiting.front().receiver, 2) << "Cathy receives, and is clued";
  EXPECT_TRUE(g.waiting.front().even_parity.value_or(false))
      << "every reactive in this convention is even parity";
}

// The sum rule names the slot and the bucket names the card, exactly as they do
// on a reverse reactive: anchor 2, target slot 4, so react_slot + 4 = 2 (mod 5)
// = slot 3. Blue is bucket 1 and this is a RANK clue, so the reacter sits one
// bucket lower — bucket 0, red and yellow — and green is already on 1.
TEST(TiiahOrdinaryReactive, TheSumRuleAndTheBucketNameTheReactersCard) {
  Game g = setup(replay_opts());

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 3));
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(g.meta[order_at(g, TestPlayer::BOB, 3)].urgent);
  expect_infs(g, std::nullopt, TestPlayer::BOB, /*slot=*/3, {"r1", "y1"});
}

// ...and the play that follows from it, end to end.
TEST(TiiahOrdinaryReactive, TheReacterPlaysTheCalledSlot) {
  SetupOptions opts = replay_opts();
  opts.starting = TestPlayer::CATHY;  // so Alice's clue lands on Bob's turn
  Game g = setup(std::move(opts));
  g = take_turn(std::move(g), "Cathy discards y5", "p2");
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);

  // Bob to move, holding an urgent call on his slot 3.
  const int called = order_at(g, TestPlayer::BOB, 3);
  Game bobs_view = g;
  bobs_view.state.our_player_index = 1;
  PerformAction a;
  ASSERT_NO_THROW(a = bobs_view.take_action());
  const auto* play = std::get_if<PerformPlay>(&a);
  ASSERT_NE(play, nullptr) << "a reaction is answered by acting on it";
  EXPECT_EQ(play->target, called);
}

// The other square of the table. Bob holds a known play and Cathy does not, so
// the position flips: the clue to Bob carries the reaction and a clue to Cathy
// is stable again.
TEST(TiiahOrdinaryReactive, TheReversePositionKeepsAClueToCathyStable) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "r3", "y4", "g4", "b4"},  // Bob's known r1
      {"r2", "y3", "g3", "b3", "p3"},  // Cathy: nothing playable
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE)
      << "while Bob holds a known play, the clue that carries a reaction is "
         "the one aimed at HIM";
  EXPECT_TRUE(g.waiting.empty());
}
