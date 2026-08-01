// Reactor0 §2b — the predicates behind the pointless-double-discard filter
// (src/conventions/reactor0/state_eval.cpp).
//
// A rank Phase C reactive (interpret_reactive.cpp:366-393) buys nothing when
// the receiver was already going to act safely: it spends a token telling
// them to discard a card they would discard anyway. When a stable clue to Bob
// would instead get a card PLAYED, reactor0 drops the double discard from its
// candidate set.
//
// This file covers `receiver_is_safe` and `is_stable_play_clue_for_bob` on
// synthetic positions, plus the negatives of `is_pointless_double_discard`.
// The POSITIVE double-discard case is pinned on the real position it came
// from, in test_replay_1942181_prefers_stable_play_over_double_discard.cpp:
// Phase A and Phase B can hard-reject a rank reactive outright (returning
// MISTAKE rather than falling through to Phase C), so hand-built fixtures
// very easily produce no double discard at all — and a test that silently
// stops exercising the branch is worse than no test.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/state_eval.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

int alice_of(const Game& g) { return g.state.our_player_index; }
int bob_of(const Game& g) { return g.state.next_player_index(alice_of(g)); }
int cathy_of(const Game& g) { return g.state.next_player_index(bob_of(g)); }

ClueAction clue_to(const Game& g, int target, ClueKind kind, int value) {
  return ClueAction{alice_of(g), target,
                    g.state.clue_touched(g.state.hands[target], kind, value),
                    BaseClue{kind, value}};
}

std::optional<ClueInterp> interp_of(const Game& hypo) {
  auto m = hypo.last_move();
  if (!m || !std::holds_alternative<ClueInterp>(*m)) return std::nullopt;
  return std::get<ClueInterp>(*m);
}

bool is_pointless(const Game& g, const ClueAction& ca) {
  Game hypo = g.simulate(Action{ca});
  return hanabi::reactor0::is_pointless_double_discard(g, hypo, ca);
}

// Alice inert; Bob holds a playable r1 on slot 1; Cathy's hand is
// caller-overridable.
SetupOptions base_position() {
  SetupOptions opts;
  opts.hands = {
      {"y4", "g4", "b4", "p4", "y5"},
      {"r1", "y3", "g3", "b3", "p3"},
      {"g1", "g1", "b2", "p2", "y2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

}  // namespace

// --- receiver_is_safe ----------------------------------------------------

// The case from replay 1942181: a basic-trash chop gets discarded whether or
// not anybody spends a token saying so.
TEST(Reactor0DoubleDiscardFilter, ReceiverWithTrashChopIsSafe) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};  // g1 played → Cathy's g1 chop is trash
  Game g = setup(std::move(opts));
  EXPECT_TRUE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
}

// A same-hand duplicate also makes the chop expendable. `at_risk_chop` folds
// this in, so `receiver_is_safe` inherits it — the clause reactor's
// `chop_is_nontrash` lacks.
TEST(Reactor0DoubleDiscardFilter, ReceiverWithDupedChopIsSafe) {
  Game g = setup(base_position());  // Cathy holds g1, g1
  EXPECT_TRUE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
}

// A standing CALLED_TO_DISCARD is a safe action; `order_trash` folds CTD into
// `thinks_trash`, which is what the predicate consults.
TEST(Reactor0DoubleDiscardFilter, ReceiverWithCtdIsSafe) {
  SetupOptions opts = base_position();
  opts.hands[2] = {"r3", "y3", "b3", "p3", "g3"};  // nothing expendable
  Game g = setup(std::move(opts));
  ASSERT_FALSE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)))
      << "guard: this hand must not already be safe";
  g.meta[order_at(g, TestPlayer::CATHY, 4)].status =
      CardStatus::CALLED_TO_DISCARD;
  g.elim();
  EXPECT_TRUE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
}

// So is an outstanding play call.
TEST(Reactor0DoubleDiscardFilter, ReceiverWithCtpIsSafe) {
  SetupOptions opts = base_position();
  opts.hands[2] = {"r3", "y3", "b3", "p3", "g3"};
  Game g = setup(std::move(opts));
  ASSERT_FALSE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
  g.meta[order_at(g, TestPlayer::CATHY, 4)].status = CardStatus::CALLED_TO_PLAY;
  g.elim();
  EXPECT_TRUE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
}

// The negative that makes the rest meaningful: an endangered chop and nothing
// else to do. Without it the filter would suppress a double discard the
// receiver actually needed.
TEST(Reactor0DoubleDiscardFilter, ReceiverWithEndangeredChopIsNotSafe) {
  SetupOptions opts = base_position();
  opts.hands[2] = {"r3", "y3", "b3", "p3", "g3"};
  Game g = setup(std::move(opts));
  EXPECT_FALSE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)));
}

// --- is_pointless_double_discard, negatives ------------------------------

// Positional dispatch: a clue to Bob is stable, so it can never be one.
TEST(Reactor0DoubleDiscardFilter, StableClueToBobIsNeverPointless) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(bob_of(g))) {
    EXPECT_FALSE(is_pointless(g, clue_to(g, bob_of(g), c.kind, c.value)))
        << "a clue to Bob is stable, not reactive";
  }
}

// Colour reactives always call a play (mode 1 stamps the receiver's target,
// mode 2 the reacter's blind play), so no colour clue is a double discard.
TEST(Reactor0DoubleDiscardFilter, ColourReactiveIsNeverPointless) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(cathy_of(g))) {
    if (c.kind != ClueKind::COLOUR) continue;
    EXPECT_FALSE(is_pointless(g, clue_to(g, cathy_of(g), c.kind, c.value)))
        << "colour reactives are one-play by construction";
  }
}

// A reactive aimed at a receiver who genuinely needs the instruction must
// survive, whatever else is true of it.
TEST(Reactor0DoubleDiscardFilter, ReactiveToEndangeredReceiverIsNeverPointless) {
  SetupOptions opts = base_position();
  opts.hands[2] = {"r3", "y3", "b3", "p3", "g3"};
  Game g = setup(std::move(opts));
  ASSERT_FALSE(hanabi::reactor0::receiver_is_safe(g, alice_of(g), cathy_of(g)))
      << "guard: the receiver must be in danger for this test to mean anything";
  for (const Clue& c : g.state.all_valid_clues(cathy_of(g))) {
    EXPECT_FALSE(is_pointless(g, clue_to(g, cathy_of(g), c.kind, c.value)));
  }
}

// --- is_stable_play_clue_for_bob -----------------------------------------

// Rank 1 to Bob names his playable r1 — a direct play clue, the shape a
// double discard gets demoted below.
TEST(Reactor0DoubleDiscardFilter, DirectPlayClueToBobCounts) {
  Game g = setup(base_position());
  ClueAction ca = clue_to(g, bob_of(g), ClueKind::RANK, 1);
  Game hypo = g.simulate(Action{ca});
  ASSERT_EQ(interp_of(hypo), ClueInterp::PLAY) << "guard: direct play reading";
  EXPECT_TRUE(hanabi::reactor0::is_stable_play_clue_for_bob(g, hypo, ca));
}

// A clue to Cathy is reactive, never a stable play clue to Bob.
TEST(Reactor0DoubleDiscardFilter, ClueToCathyIsNotAStablePlayClueForBob) {
  Game g = setup(base_position());
  for (const Clue& c : g.state.all_valid_clues(cathy_of(g))) {
    ClueAction ca = clue_to(g, cathy_of(g), c.kind, c.value);
    Game hypo = g.simulate(Action{ca});
    EXPECT_FALSE(hanabi::reactor0::is_stable_play_clue_for_bob(g, hypo, ca));
  }
}

// The load-bearing negative: a stable clue to Bob that creates no playable
// does NOT qualify. This is what stops the filter firing on the strength of a
// stall, a referential discard or a trash reveal — exactly the readings a
// double discard is meant to stay ranked above.
TEST(Reactor0DoubleDiscardFilter, PlaylessStableClueToBobDoesNotCount) {
  SetupOptions opts = base_position();
  opts.hands[1] = {"y3", "g3", "b3", "p3", "r3"};  // nothing playable for Bob
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(bob_of(g))) {
    ClueAction ca = clue_to(g, bob_of(g), c.kind, c.value);
    Game hypo = g.simulate(Action{ca});
    EXPECT_FALSE(hanabi::reactor0::is_stable_play_clue_for_bob(g, hypo, ca))
        << "no card of Bob's is playable, so no clue to him can create a play";
  }
}
