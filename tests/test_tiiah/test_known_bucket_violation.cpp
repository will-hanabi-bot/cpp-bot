// THE BUCKET RULE AS A LEGALITY LAYER (tiiah/CONVENTION.md §1d, v22.4.0, the user's
// ruling; replay 2022760 T22).
//
// A direct target the reacter can answer only with a card outside the bucket the
// relation names is no pairing -- every walking seat goes past it -- unless the
// violation is GLOBALLY KNOWN: the receiver, reading the bucket from the card it
// watched, finds nothing among its target's possibilities and falls back to its
// playable. A finesse is not affected.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Six suits (r y g b p t; buckets {r,y} {g,b} {p,t}). We are BOB, the reacter, and
// cannot see our hand; our slot 5 was clued purple. Alice's 1 to Cathy pairs our
// slot 5 with Cathy's slot 1, the t1. A rank clue names bucket 0 (red/yellow) for a
// purple reacter: the t1 breaks it. (The giver, who sees our card, does not give such
// a clue yet -- v22.4.0 -- so the walk is read at the reacter's seat.)
Game position(std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (6 Suits)";
  opts.play_stacks = std::move(stacks);
  opts.hands = {{"r3", "g3", "b3", "y4", "g4"},
                {"xx", "xx", "xx", "xx", "xx"},
                {"t1", "b2", "b3", "b5", "b4"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  opts.init = [](Game& g) { g.state.our_player_index = 1; };
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"purple"});
  return take_turn(std::move(g), "Alice clues 1 to Cathy");
}

// The same shape in four suits (r y g b; buckets {r,y} {g} {b}), for the
// three-bucket rule. Our slot 5 was clued red. Alice's 1 to Cathy pairs our slot 5
// with Cathy's slot 1, the y1. A rank clue names bucket 1 (green) for a red
// reacter: the y1 breaks it.
Game position_4_suits(std::vector<int> stacks) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.play_stacks = std::move(stacks);
  opts.hands = {{"r3", "g3", "b3", "y4", "g4"},
                {"xx", "xx", "xx", "xx", "xx"},
                {"y1", "b2", "b3", "b5", "b4"}};
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  opts.init = [](Game& g) { g.state.our_player_index = 1; };
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"red"});
  return take_turn(std::move(g), "Alice clues 1 to Cathy");
}

}  // namespace

// The user's example: r1, y1 and p1 are down for everyone. Cathy's touched 1 cannot
// be a bucket-0 playable (the r2 or the y2), and our purple card can be no bucket-1
// playable, so both of us fall back to our playable: the violation is globally known,
// and the pairing stands. Our slot 5 answers, and Cathy's t1 is the target.
TEST(TiiahKnownBucketViolation, AGloballyKnownViolationStands) {
  Game g = position({1, 1, 0, 0, 1, 0});
  ASSERT_FALSE(g.waiting.empty()) << "a reactive";
  EXPECT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 5));
  EXPECT_EQ(g.waiting.front().receiver_target_order, order_at(g, TestPlayer::CATHY, 1));
}

// Control, in four suits: green is not down, so Cathy's 1 could be the bucket-1 g1.
// She would name her card the g1 from our red, not the y1: the violation is not one
// she can see through, and the pairing is no pairing.
TEST(TiiahKnownBucketViolation, AViolationTheReceiverWouldMisreadIsNoPairing) {
  Game g = position_4_suits({1, 0, 0, 0});
  EXPECT_TRUE(g.waiting.empty() ||
              g.waiting.front().receiver_target_order != order_at(g, TestPlayer::CATHY, 1))
      << "Bob's red does not answer Cathy's y1";
}
