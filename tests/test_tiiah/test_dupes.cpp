// The playable-dupe conventions (tiiah/CONVENTION.md §1j, §1k, v16.25.0).
//
//   * PASSBACK: a seat that can name its card as X, seeing another seat's called
//     card that really is X while that seat reads it wider, throws its own copy;
//     the other holder reads the throw as "yours is X".
//   * DISCHARGE: a reacter whose reaction card is X, already played by the giver
//     without the giver knowing, throws it instead of playing; the receiver still
//     plays, and the giver learns what it threw in the hole.

#include <gtest/gtest.h>

#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions opts_for(std::vector<std::vector<std::string>> hands,
                      std::vector<int> stacks, TestPlayer starting) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = std::move(hands);
  opts.play_stacks = std::move(stacks);
  opts.starting = starting;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return opts;
}

void read_as(Game& g, int order, const IdentitySet& set) {
  g.state.deck[order].clued = true;
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
}

IdentitySet ids(std::initializer_list<Identity> list) {
  IdentitySet out = IdentitySet::empty();
  for (auto i : list) out = out.add(i);
  return out;
}

constexpr Identity kR1{0, 1}, kY1{1, 1}, kG1{2, 1}, kR4{0, 4};

int discarded(const PerformAction& a) {
  const auto* d = std::get_if<PerformDiscard>(&a);
  return d ? d->target : -1;
}

}  // namespace

// --- passback ---------------------------------------------------------------

// Our called r4, and Cathy's called card that we can see is the r4 while she reads
// it {g1, r4}: playing ours leaves her to play hers as the g1 and strike.
TEST(TiiahPassback, WeThrowOurCopyOfAnUnnamedCalledDupe) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"y3", "g3", "b3", "p3", "y4"},
                           {"r4", "y5", "g5", "b5", "p5"}},
                          {3, 0, 0, 0, 0}, TestPlayer::ALICE));
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  const int hers = order_at(g, TestPlayer::CATHY, 1);
  read_as(g, ours, IdentitySet::single(kR4));
  g.meta[ours].status = CardStatus::CALLED_TO_PLAY;
  read_as(g, hers, ids({kG1, kR4}));
  g.meta[hers].status = CardStatus::CALLED_TO_PLAY;

  EXPECT_EQ(discarded(g.take_action()), ours);
}

// No strike is coming when, with our copy down, the rest of her reading is trash:
// she would throw it away herself. Replay 2010296 T9.
TEST(TiiahPassback, NoPassbackWhenTheOtherReadingIsOtherwiseTrash) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"y3", "g3", "b3", "p3", "y4"},
                           {"r4", "y5", "g5", "b5", "p5"}},
                          {3, 2, 0, 0, 0}, TestPlayer::ALICE));
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  const int hers = order_at(g, TestPlayer::CATHY, 1);
  read_as(g, ours, IdentitySet::single(kR4));
  g.meta[ours].status = CardStatus::CALLED_TO_PLAY;
  read_as(g, hers, ids({Identity{1, 2}, kR4}));  // {y2, r4}, yellow already on 2
  g.meta[hers].status = CardStatus::CALLED_TO_PLAY;

  EXPECT_NE(discarded(g.take_action()), ours);
}

// The other holder's side: Bob throws the r4 the team knew, and Cathy's called
// {g1, r4} is named by the throw.
TEST(TiiahPassback, TheThrowNamesTheOtherCopy) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"r4", "g3", "b3", "p3", "y4"},
                           {"r4", "y5", "g5", "b5", "p5"}},
                          {3, 0, 0, 0, 0}, TestPlayer::BOB));
  read_as(g, order_at(g, TestPlayer::BOB, 1), IdentitySet::single(kR4));
  const int hers = order_at(g, TestPlayer::CATHY, 1);
  read_as(g, hers, ids({kG1, kR4}));
  g.meta[hers].status = CardStatus::CALLED_TO_PLAY;

  g = hidden_action(std::move(g), TestPlayer::BOB, 1, /*reached_the_hole=*/false, "y5");
  EXPECT_EQ(g.common.thoughts[hers].inferred, IdentitySet::single(kR4));
}

// --- discharge --------------------------------------------------------------

namespace {

// We (Alice) throw a {r1,y1} into the hole -- it was the r1 -- and later give Cathy
// a reactive naming her r2 as a finesse through Bob's r1.
Game discharge_position(const IdentitySet& our_hole) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"r1", "y3", "g3", "b3", "p3"},
                           {"r2", "y4", "g4", "b4", "p4"}},
                          {0, 0, 0, 0, 0}, TestPlayer::ALICE));
  read_as(g, order_at(g, TestPlayer::ALICE, 1), our_hole);
  g = hidden_action(std::move(g), TestPlayer::ALICE, 1, /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "y5");
  g = hidden_action(std::move(g), TestPlayer::CATHY, 5, /*reached_the_hole=*/false, "r5");
  // Cathy's r2 is on slot 2 now, Bob's r1 on slot 2; rank 4 pairs slot 2 with 2.
  return take_turn(std::move(g), "Alice clues 4 to Cathy");
}

}  // namespace

TEST(TiiahDischarge, TheReceiverStillPlaysAndTheGiverLearnsItsHoleCard) {
  Game g = discharge_position(ids({kR1, kY1}));
  const int hole = [&] {
    for (int o = 0; o < static_cast<int>(g.meta.size()); ++o) {
      if (g.meta[o].superposed()) return o;
    }
    return -1;
  }();
  ASSERT_GE(hole, 0) << "guard: our {r1,y1} is in the hole";
  const int r2 = order_at(g, TestPlayer::CATHY, 2);

  // Bob throws the r1: it was already played -- by us -- so he discharges it.
  g = hidden_action(std::move(g), TestPlayer::BOB, 2, /*reached_the_hole=*/false, "g5");
  EXPECT_EQ(g.meta[r2].status, CardStatus::CALLED_TO_PLAY) << "Cathy still plays the r2";
  EXPECT_FALSE(g.meta[hole].superposed()) << "our hole card was the r1";
  EXPECT_EQ(g.state.play_stacks[0], 1);
}

TEST(TiiahDischarge, WithoutTheCardInTheGiversHoleTheDiscardIsADiscard) {
  Game g = discharge_position(ids({kG1, kY1}));
  const int r2 = order_at(g, TestPlayer::CATHY, 2);
  g = hidden_action(std::move(g), TestPlayer::BOB, 2, /*reached_the_hole=*/false, "g5");
  EXPECT_NE(g.meta[r2].status, CardStatus::CALLED_TO_PLAY)
      << "no hole card of ours could be the r1, so the reaction reads as a discard";
}

// The reacter's side: we are called to play an r1 that Cathy, the giver, already
// played into the hole without knowing it. We throw it instead.
TEST(TiiahDischarge, TheReacterThrowsACardTheGiverAlreadyPlayed) {
  Game g = setup(opts_for({{"xx", "xx", "xx", "xx", "xx"},
                           {"r2", "y4", "g4", "b4", "p4"},
                           {"r1", "y3", "g3", "b3", "p3"}},
                          {0, 0, 0, 0, 0}, TestPlayer::CATHY));
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  // {r1, r3}: not a KNOWN play (the r3 is not playable), so the position does not
  // flip Cathy's clue to Bob into a stable one; the finesse then names the r1.
  read_as(g, ours, ids({kR1, Identity{0, 3}}));
  read_as(g, order_at(g, TestPlayer::CATHY, 1), ids({kR1, kY1}));
  g = hidden_action(std::move(g), TestPlayer::CATHY, 1, /*reached_the_hole=*/true, "y5");
  g = hidden_action(std::move(g), TestPlayer::ALICE, 5, /*reached_the_hole=*/false);
  g = hidden_action(std::move(g), TestPlayer::BOB, 5, /*reached_the_hole=*/false, "r5");
  ASSERT_EQ(order_at(g, TestPlayer::ALICE, 2), ours);
  g = take_turn(std::move(g), "Cathy clues 4 to Bob");
  // The pairing names our card as the r1 (the finesse's connector), and the r1 is
  // already down on our own stacks: we are called to throw it, not play it.
  EXPECT_EQ(g.meta[ours].status, CardStatus::CALLED_TO_DISCARD);
  EXPECT_EQ(g.common.thoughts[ours].inferred, IdentitySet::single(kR1));

  EXPECT_EQ(discarded(g.take_action()), ours) << "the r1 is already down: discharge it";
}
