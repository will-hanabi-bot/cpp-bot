// The stable referential discard narrows its target (v16.11.0).
//
// `reactor::ref_discard` stamps the called slot and narrows nothing — it was
// the one stamping path in the reactor0 family that left `inferred` alone, so a
// note reading "throw this away" still listed every critical the card could be.
// Replay 2008422 T1 is the report: a rank-4 clue called will-bot67's order 6 to
// discard and the note kept all five 5s.
//
// reactor0 now applies `reactor::target_discard`'s plain-suit filter at its own
// call site (`src/conventions/reactor0/interpret_clue.cpp`, `narrow_stable_chuck`).
// Reactor is deliberately untouched: its corpus pins a stable CTD that lands on
// a critical dark null 5 (replay 1916791), and `Game::convention` defaults to
// REACTOR, so `tests/test_basics/test_stable_ref_discard.cpp` keeps testing the
// unnarrowed reading.
//
// Two things are pinned here. The filter itself, and the INVERTED carve-out:
// on an inverted suit Discard is a chuck, which stacks the card rather than
// losing it, so a critical reading there is the one the call wants. Without the
// carve-out the Orange variants — where the playable orange is routinely the
// last copy — would have the set emptied out from under them.

#include <gtest/gtest.h>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

// Plain suits. Alice clues rank 3 to Bob; nothing of rank 3 is playable at
// all-zero stacks, so the ladder falls through to the referential discard.
// Focus is slot 1 (the only card touched), the target is the first unclued slot
// to its right — slot 2 — and that card's readings are "any non-3", which
// includes all five 5s until the filter runs.
TEST(Reactor0StableChuck, TheCalledSlotKeepsNoCriticalReading) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver / POV). No 5s and no 3s: a 5 sitting here would be
      // counted out of Bob's empathy by ordinary elim and the test would pass
      // without the filter ever running.
      {"r4", "y4", "g4", "b4", "p4"},
      // Bob (receiver). Slot 1 = r3, the rank-3 focus; slot 2 = the target.
      {"r3", "g2", "b1", "p1", "y4"},
      {"r1", "y1", "g1", "b1", "p1"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  const int target = order_at(g, TestPlayer::BOB, 2);
  ASSERT_EQ(g.meta[target].status, CardStatus::CALLED_TO_DISCARD)
      << "rank-3 ref_discard stamps the first unclued slot right of the focus";

  const IdentitySet& inferred = g.common.thoughts[target].inferred;
  EXPECT_TRUE(inferred.non_empty())
      << "the filter must not leave the card with no reading at all";
  EXPECT_TRUE(inferred.intersect(g.state.critical_set).is_empty())
      << "a card called to discard cannot be read as a critical — that is the "
         "whole content of the call";
  EXPECT_TRUE(inferred.contains(Identity{2, 2}))
      << "and the readings that ARE safe to throw survive: g2 is the card";
}

// The inverted carve-out. Muddy Rainbow & Orange, orange inverted: pressing
// Discard on an orange puts it ON its stack, so `o5` — critical, and the only
// copy there will ever be — is exactly what a chuck call may mean. It must
// survive a filter that drops the plain criticals beside it.
TEST(Reactor0StableChuck, AnInvertedCriticalIsWhatTheChuckIsFor) {
  SetupOptions opts;
  opts.variant_name = "Muddy Rainbow & Orange (3 Suits)";
  opts.hands = {
      {"r4", "m4", "r2", "m2", "o2"},
      {"r3", "m1", "r1", "o1", "m2"},
      {"o3", "r2", "m3", "r4", "m4"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  const int target = order_at(g, TestPlayer::BOB, 2);
  ASSERT_EQ(g.meta[target].status, CardStatus::CALLED_TO_DISCARD)
      << "rank-3 ref_discard stamps the first unclued slot right of the focus";

  const IdentitySet& inferred = g.common.thoughts[target].inferred;
  EXPECT_TRUE(inferred.contains(Identity{2, 5}))
      << "o5 is critical AND inverted: chucking it stacks it, so the call can "
         "mean it and the filter must leave it alone";
  EXPECT_FALSE(inferred.contains(Identity{0, 5}))
      << "r5 is a plain critical — chucking it loses it for good";
  EXPECT_FALSE(inferred.contains(Identity{1, 5}))
      << "m5 likewise";
}
