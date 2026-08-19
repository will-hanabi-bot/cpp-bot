// Reactor0 clue-tier classification (src/conventions/reactor0/state_eval.cpp,
// CONVENTION.md §2a). These call `reactor0::clue_tier` directly rather than
// going through `eval_action`, so each condition is pinned in isolation from
// the pace/clue-count window (that is test_pace_clue_gate.cpp's job).
//
//   HIGH iff any of:
//     H1  Bob is unlocked, has no safe action, and his chop is endangered.
//     H2  the clue gets a critical 1 or 2 played (5 or 4 reversed).
//     H3  the clue gets 2 new plays, one at the clue-regain rank.
//   NOT LOW iff any of: H1..H3, or
//     N2  Cathy's chop is endangered, the clue is reactive, and Bob has no
//         colour stable play clue he could give Cathy.
//     N3  Cathy's chop is endangered and the clue gets 2 new plays.
//
// "Endangered chop" is stricter than reactor's `chop_id_is_unique`
// (reactor/state_eval.cpp:82-103) in two ways this file pins explicitly: a
// same-hand duplicate counts as trash, and a copy Alice can prove she holds
// disqualifies it whether that proof is a singleton inference OR a group
// ("sudoku") elim.
//
// These are setup()-built fixtures, not replays, so the `"convention":
// "reactor0"` snapshot key does not apply — `use_reactor0(opts)` sets the
// convention. A replay test added here would need that key.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/state_eval.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueTier;

namespace {

Action make_clue(const Game& g, int giver, int target, ClueKind kind,
                 int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
  return Action{
      ClueAction{giver, target, std::move(touched), BaseClue{kind, value}}};
}

ClueTier tier_of(const Game& g, const Action& clue) {
  Game hypo = g.simulate(clue);
  return hanabi::reactor0::clue_tier(g, hypo, std::get<ClueAction>(clue));
}

const char* name_of(ClueTier t) {
  switch (t) {
    case ClueTier::HIGH: return "HIGH";
    case ClueTier::MEDIUM: return "MEDIUM";
    default: return "LOW";
  }
}

// A clue to Bob that touches his rank-4 fillers — deliberately inert, so the
// tier reflects the fixture's chop/play facts rather than the clue itself.
Action inert_clue(const Game& g) {
  return make_clue(g, 0, 1, ClueKind::RANK, 4);
}

}  // namespace

// --- H1: Bob's chop is endangered ----------------------------------------

// Bob is unlocked, holds nothing clued (no play, no trash, no CTD), and his
// chop r3 is useful, unduplicated in his own hand, absent from Cathy's, and
// not provable in Alice's. Condition H1 fires regardless of what the clue
// itself achieves.
TEST(Reactor0ClueTier, BobEndangeredChopIsHigh) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "y4", "g4", "b4", "p4"},  // chop (slot 1) = r3
      {"y2", "g2", "b2", "p2", "g5"},  // no r3 here
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.chop(1), order_at(g, TestPlayer::BOB, 1)) << "guard: Bob's chop";
  EXPECT_EQ(tier_of(g, inert_clue(g)), ClueTier::HIGH)
      << "Bob has no safe action and an endangered r3 chop → H1.";
}

// Extension (a): a second copy of the chop identity in Bob's OWN hand makes
// the chop expendable, so H1 must not fire. Reactor's `chop_is_nontrash`
// (reactor/state_eval.cpp:44-49) tests only `is_basic_trash` and misses this.
TEST(Reactor0ClueTier, BobChopWithSameHandDupeIsNotHigh) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "r3", "g4", "b4", "p4"},  // chop r3 duplicated in-hand
      {"p2", "p2", "b2", "g2", "g5"},  // Cathy's chop duped too → N2/N3 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "Bob can pitch one r3 safely → chop not endangered. Got "
      << name_of(t);
}

// The chop identity is visible in Cathy's hand, so losing Bob's copy costs
// nothing. (This is reactor's existing clause, kept.)
TEST(Reactor0ClueTier, BobChopCopyInCathyIsNotHigh) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "y4", "g4", "b4", "p4"},  // Bob chop r3
      {"r3", "g2", "b2", "p2", "g5"},  // Cathy chop is the other r3
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "each r3 covers the other → neither chop endangered. Got "
      << name_of(t);
}

// Extension (b), k == 1: a singleton inference in Alice's own hand proves she
// holds the other copy. This is the case reactor already handles
// (reactor/state_eval.cpp:96-101); pinned here so the group-elim
// generalisation cannot regress it.
TEST(Reactor0ClueTier, BobChopSingletonInAliceHandIsNotHigh) {
  SetupOptions opts;
  opts.hands = {
      {"r3", "g1", "b1", "p1", "y5"},  // Alice slot 1 will be known r3
      {"r3", "y4", "g4", "b4", "p4"},  // Bob chop = the other r3
      {"p2", "p2", "b2", "g2", "g5"},  // Cathy chop duped → N2/N3 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "r3");

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "Alice knows she holds the other r3 → Bob's chop is covered. Got "
      << name_of(t);
}

// Extension (b), k == 2: the genuine sudoku/group elim. Alice holds two cards
// both narrowed to {r3, b3}; with one b3 already discarded only a single b3
// is unaccounted for, so the two cards must be one r3 and one b3 — Alice
// provably holds an r3 even though NEITHER card is a singleton. Reactor's
// singleton-only test misses this entirely.
TEST(Reactor0ClueTier, BobChopGroupElimInAliceHandIsNotHigh) {
  SetupOptions opts;
  opts.hands = {
      {"r3", "b3", "y1", "g1", "p1"},  // both 3s clued below
      {"r3", "y4", "g4", "b4", "p4"},  // Bob chop = the other r3
      {"p2", "p2", "b2", "g2", "g5"},  // Cathy chop duped → N2/N3 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  // Every g3/y3/p3 is publicly gone, so a rank-3 clue leaves only {r3, b3};
  // one b3 is gone too, leaving a single unaccounted b3.
  opts.discarded = {"g3", "g3", "y3", "y3", "p3", "p3", "b3"};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"3"});
  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/2, {"3"});
  g.elim();

  // Guard the fixture: if elim ever stops forming this group the test should
  // fail loudly here rather than silently stop testing the group path.
  expect_infs(g, std::nullopt, TestPlayer::ALICE, 1, {"r3", "b3"});
  expect_infs(g, std::nullopt, TestPlayer::ALICE, 2, {"r3", "b3"});

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "two cards pinned to {r3,b3} with one b3 gone → Alice provably holds "
         "an r3, so Bob's chop is covered. Got "
      << name_of(t);
}

// --- H2: a critical low card gets played ---------------------------------

// Bob has a safe action (a standing CTD), so H1 cannot fire; only H2 can lift
// this clue. Rank 1 to Bob is a reactor0 stable direct play clue naming his
// critical r1.
TEST(Reactor0ClueTier, CriticalOnePlayedIsHigh) {
  SetupOptions opts;
  opts.hands = {
      {"y2", "g2", "b2", "p2", "y5"},
      {"r1", "y4", "g4", "b4", "p4"},
      {"p3", "p3", "b4", "g4", "g5"},  // Cathy chop duped → N2/N3 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.discarded = {"r1", "r1"};  // the held r1 is now the last one
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g.meta[order_at(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  ASSERT_TRUE(g.state.is_critical(Identity{0, 1})) << "guard: r1 is critical";
  EXPECT_EQ(tier_of(g, make_clue(g, 0, 1, ClueKind::RANK, 1)), ClueTier::HIGH)
      << "rank 1 to Bob plays his critical r1 → H2.";
}

// The same shape with a non-critical 1 must NOT reach HIGH via H2.
TEST(Reactor0ClueTier, NonCriticalOnePlayedIsNotHigh) {
  SetupOptions opts;
  opts.hands = {
      {"y2", "g2", "b2", "p2", "y5"},
      {"r1", "y4", "g4", "b4", "p4"},
      {"p3", "p3", "b4", "g4", "g5"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;  // nothing discarded → r1 not critical
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g.meta[order_at(g, TestPlayer::BOB, 5)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();

  ASSERT_FALSE(g.state.is_critical(Identity{0, 1}))
      << "guard: r1 must not be critical here";
  EXPECT_NE(tier_of(g, make_clue(g, 0, 1, ClueKind::RANK, 1)), ClueTier::HIGH)
      << "a non-critical 1 does not satisfy H2.";
}

// --- N5: Bob has a playable chop -----------------------------------------

// N5 is a property of the POSITION, not of the clue: when Bob's chop is a
// playable card he cannot just pitch a duplicate of, every clue that turn is
// worth at least MEDIUM. Deliberately weaker than `at_risk_chop` — it does
// not care that Cathy holds a copy — which is what makes it fire at replay
// 1942330 T33, where H1/N1 cannot.
TEST(Reactor0ClueTier, BobPlayableChopLiftsAnyClueToMedium) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r1", "y4", "g4", "b4", "p4"},  // chop (slot 1) = r1, playable
      {"r1", "p2", "p2", "b2", "g5"},  // Cathy holds the other r1 → not at risk
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.chop(1), order_at(g, TestPlayer::BOB, 1)) << "guard: Bob's chop";
  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_NE(t, ClueTier::LOW)
      << "Bob's chop r1 is playable and unduplicated in his own hand, so any "
         "clue is at least MEDIUM. Got " << name_of(t);
}

// A same-hand duplicate makes the chop expendable — Bob can pitch one copy
// and still play the other, so N5 must not fire.
TEST(Reactor0ClueTier, BobPlayableChopDupedInOwnHandDoesNotLift) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r1", "r1", "g4", "b4", "p4"},  // chop r1 duplicated in-hand
      {"p2", "p2", "b2", "g2", "g5"},  // Cathy's chop duped → N2/N3 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "a duplicated playable chop does not lift the tier. Got " << name_of(t);
}

// An unplayable chop does not lift the tier either, however useful it is.
TEST(Reactor0ClueTier, BobUnplayableChopDoesNotLift) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "y4", "g4", "b4", "p4"},  // chop r3 — useful but not playable
      {"r3", "p2", "p2", "b2", "g5"},  // Cathy covers r3 → H1/N1 dead
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "r3 on a stack of 0 is not playable, so N5 must stay silent. Got "
      << name_of(t);
}

// --- H4: the clue gets a finesse -----------------------------------------

// H4 is the only *widening* term step 2 added, and the only one that depends on
// the clue rather than the position, so the fixture is built to make MEDIUM the
// answer if H4 does not fire — that is what makes this test a discriminator
// rather than a tautology.
//
// Alice clues rank 5 to Cathy. Cathy holds no playable, so reactive rank Phase A
// is empty and Phase B walks her one-away cards: r2 at slot 3. anchor 5,
// target_slot 3 → react_slot = (5 + 5 - 3) % 5 = 2, and Bob's slot 2 holds the
// r1 connector. Every other HIGH term is deliberately dead:
//
//   H1a  Bob's chop y4 is duplicated in his own hand → not endangered.
//   H2   r1 is not critical (3 copies, none discarded).
//   H3   Phase B stamps only the reacter, so the play count is 1.
//
// while N2 is alive (Cathy's y5 chop is critical → endangered, the clue is
// reactive, and Bob has no colour play clue for a hand with no playable). So a
// working H4 reads HIGH and a dead one reads MEDIUM.
TEST(Reactor0ClueTier, FinesseIsHigh) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "r1", "b4", "g4", "y4"},  // chop y4 duped in-hand; r1 at slot 2
      {"y5", "g3", "r2", "b3", "p4"},  // no playable; leftmost one-away r2 @ 3
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  Action clue = make_clue(g, 0, 2, ClueKind::RANK, 5);
  Game hypo = g.simulate(clue);
  ASSERT_EQ(status_at(hypo, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "guard: the fixture must actually produce a Phase B finesse";

  ClueTier t = tier_of(g, clue);
  EXPECT_EQ(t, ClueTier::HIGH)
      << "a finesse with a non-expendable Cathy chop is H4 → HIGH. Got "
      << name_of(t) << " (MEDIUM means H4 never fired).";
}

// --- H1b / H1c: the Cathy conditions on H1 -------------------------------

// H1 became a CONJUNCTION in v7.0.0 step 2 — H1a AND H1b AND H1c — so both new
// terms can only ever narrow it. Each fixture below keeps H1a firing and kills
// exactly one of H1b / H1c, so a LOW reading is attributable to that term: if
// the term did not exist, both would read HIGH.

// H1b — rescuing Bob's chop is not HIGH when Cathy is herself about to lose
// something. Cathy's chop r5 is critical (one copy), which is the H1b veto.
// H1c is deliberately left TRUE so it cannot be the cause: Cathy holds no
// playable at all, so Bob has no colour stable play clue for her.
TEST(Reactor0ClueTier, CathyCriticalChopBlocksH1) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "y4", "g4", "b4", "p4"},  // chop r3 — endangered, so H1a fires
      {"r5", "y3", "g3", "b3", "p3"},  // chop r5 critical; no playable anywhere
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.chop(1), order_at(g, TestPlayer::BOB, 1)) << "guard: Bob's chop";
  ASSERT_EQ(g.chop(2), order_at(g, TestPlayer::CATHY, 1)) << "guard: Cathy's chop";
  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "Cathy's chop is critical, so H1b vetoes H1. Got " << name_of(t)
      << " (HIGH means H1b never fired).";
}

// H1c — rescuing Bob's chop is not HIGH when Bob could have handled Cathy
// himself. Cathy's chop y3 is neither playable nor critical, so H1b passes;
// what kills H1 here is that Cathy holds a playable g1, which is exactly the
// colour stable play clue Bob could give her.
TEST(Reactor0ClueTier, BobCanColourClueCathyBlocksH1) {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"r3", "y4", "g4", "b4", "p4"},  // chop r3 — endangered, so H1a fires
      {"y3", "g1", "b3", "p3", "r4"},  // chop y3 passes H1b; g1 is Bob's out
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.chop(2), order_at(g, TestPlayer::CATHY, 1)) << "guard: Cathy's chop";
  ClueTier t = tier_of(g, inert_clue(g));
  EXPECT_EQ(t, ClueTier::LOW)
      << "Bob has a colour play clue for Cathy, so H1c vetoes H1. Got "
      << name_of(t) << " (HIGH means H1c never fired).";
}
