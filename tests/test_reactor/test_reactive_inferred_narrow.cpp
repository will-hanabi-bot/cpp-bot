// Tests for the v0.22 receiver-inferred narrowing in the reactor's
// reactive interpretation.
//
// Before v0.22, the reactive paths stamped the receiver's play target
// with `CALLED_TO_PLAY` via `with_meta` but did NOT narrow the card's
// `inferred` set. The card retained the wide post-basic-clue-elim
// empathy, which polluted [f] notes and downstream finesse /
// connection inference. See replay 1890204#7.
//
// v0.22 mirrors `target_play`'s narrowing pattern
// (`src/conventions/reactor/interpret_clue.cpp:189-260`) on the
// receiver target: `inferred ∩ (playable_set ∪ delayed-play
// next-ranks)`.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

int order_of(const Game& g, TestPlayer p, int slot) {
  return g.state.hands[static_cast<int>(p)][slot - 1];
}

bool inferred_contains(const Game& g, int order, int suit, int rank) {
  return g.common.thoughts[order].inferred.contains(Identity{suit, rank});
}

}  // namespace

// Reactive colour clue: stacks at 0, Cathy's slot 1 is r1 (currently
// playable), Bob has filler slot 5 = r1 so target_play succeeds. After
// Alice's red clue, the reactive interp CTPs Cathy's slot 1. The
// narrowed inferred must be a subset of (playable_set ∪
// next-ranks-of-reacter-inferred) = {r1,r2,y1,y2,g1,g2,b1,b2,p1,p2}.
TEST(ReactiveReceiverInferredNarrow, ColourNarrowsToPlayablePlusNextRanks) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer, giver): filler.
      {"y4", "g4", "b4", "p3", "y3"},
      // Bob (reacter, P1): filler with slot 5 = r1 so the reactive
      // interp's react_slot pick (= calc_slot(focus=1, target=1, hs=5)
      // = 5) lands on a currently-playable identity for target_play to
      // succeed.
      {"y4", "g4", "b4", "p4", "r1"},
      // Cathy (receiver, P2): slot 1 = r1 (currently playable on R=0).
      {"r1", "y3", "g2", "b2", "p2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  ASSERT_EQ(std::get<ClueInterp>(g.move_history.back()), ClueInterp::REACTIVE)
      << "test setup must produce a reactive interpretation";

  int cathy_r1 = order_of(g, TestPlayer::CATHY, 1);
  ASSERT_EQ(g.meta[cathy_r1].status, CardStatus::CALLED_TO_PLAY)
      << "Cathy's slot 1 should be the reactive play target (CTP)";

  // The narrowed set should exclude any rank-3+ identity (none reachable
  // by playable_set or next-ranks from a stacks-at-0 state).
  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 3; rank <= 5; ++rank) {
      EXPECT_FALSE(inferred_contains(g, cathy_r1, suit, rank))
          << "narrowed inferred must not contain rank-" << rank << " ids; "
          << "stacks at 0 → max reachable rank via reaction is 2. Got "
          << "suit=" << suit << " rank=" << rank;
    }
  }
  // The actual id (r1) must remain in the narrowed inferred.
  EXPECT_TRUE(inferred_contains(g, cathy_r1, /*suit=*/0, /*rank=*/1))
      << "narrowed inferred must keep the actual identity";
}

// Reactive rank clue: force the rank-reactive path by setting
// `next_interp = REACTIVE` before the clue. (Without forcing, a
// rank-1 clue touching a currently-playable slot would dispatch
// through the stable `playable_rank` branch in try_stable instead of
// reactive.) The narrowing logic in interpret_reactive_rank is
// structurally identical to the colour path, so this test pins that
// the rank path narrows too.
TEST(ReactiveReceiverInferredNarrow, RankNarrowsToPlayablePlusNextRanks) {
  SetupOptions opts;
  opts.hands = {
      // Alice (P0, giver): filler.
      {"y4", "g4", "b4", "p3", "y3"},
      // Bob (P1, reacter): non-playable filler, slot 5 = r1 to make
      // the reactive react_slot land on a currently-playable identity.
      {"y4", "g4", "b4", "p4", "r1"},
      // Cathy (P2, receiver): slot 1 = r1 (currently playable).
      {"r1", "y3", "g2", "b2", "p2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  opts.init = [](Game& g) { g.next_interp = ClueInterp::REACTIVE; };
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  ASSERT_EQ(std::get<ClueInterp>(g.move_history.back()), ClueInterp::REACTIVE)
      << "next_interp = REACTIVE must force the reactive rank path";

  int cathy_r1 = order_of(g, TestPlayer::CATHY, 1);
  ASSERT_EQ(g.meta[cathy_r1].status, CardStatus::CALLED_TO_PLAY)
      << "Cathy's slot 1 should be the rank-reactive play target";

  // No rank-3+ identity should survive the narrowing.
  for (int suit = 0; suit < 5; ++suit) {
    for (int rank = 3; rank <= 5; ++rank) {
      EXPECT_FALSE(inferred_contains(g, cathy_r1, suit, rank))
          << "narrowed inferred must not contain rank-" << rank
          << " ids; got suit=" << suit << " rank=" << rank;
    }
  }
  EXPECT_TRUE(inferred_contains(g, cathy_r1, /*suit=*/0, /*rank=*/1))
      << "narrowed inferred must keep the actual identity (r1)";
}

// Explicit assertion: the v0.22 narrowing excludes obviously-
// unreachable identities (high ranks with stacks at 0). Pre-v0.22 the
// receiver target's [f] note included the full post-elim empathy set
// — this test pins the regression.
TEST(ReactiveReceiverInferredNarrow, ExcludesUnreachableHighRanks) {
  SetupOptions opts;
  opts.hands = {
      {"y4", "g4", "b4", "p3", "y3"},
      {"y4", "g4", "b4", "p4", "r1"},
      {"r1", "y3", "g2", "b2", "p2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0, 0};
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues Red to Cathy");

  int cathy_r1 = order_of(g, TestPlayer::CATHY, 1);
  if (g.meta[cathy_r1].status != CardStatus::CALLED_TO_PLAY) {
    GTEST_SKIP() << "this test depends on a CTP on Cathy slot 1";
  }

  // Pin specific identities that should NOT survive: y4 (rank 4), b5
  // (rank 5), p5 (rank 5). All unreachable: stacks at 0, max reachable
  // rank via a single reaction is 2.
  EXPECT_FALSE(inferred_contains(g, cathy_r1, /*suit=*/1, /*rank=*/4));
  EXPECT_FALSE(inferred_contains(g, cathy_r1, /*suit=*/3, /*rank=*/5));
  EXPECT_FALSE(inferred_contains(g, cathy_r1, /*suit=*/4, /*rank=*/5));
}
