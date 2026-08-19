// Which reactives reactor0 refuses to propose (DECISION_MAKING.md priority 2,
// src/conventions/reactor0/decision.cpp).
//
// v7.0.0 REWRITE. This file used to test the §2b "pointless double discard"
// filter: a standalone pass over the candidate set that deleted a rank Phase C
// reactive when a stable play clue to Bob survived alongside it. That filter and
// its three predicates — `receiver_is_safe`, `is_pointless_double_discard`,
// `is_stable_play_clue_for_bob` — are gone, replaced by two properties of the
// priority list that need no separate pass:
//
//   * priority 2 is "a reactive DISCARD clue", which the spec defines as one
//     card played and one card discarded. A double discard is a different shape
//     and is not admitted there at all; it can only ever enter at rung 3.6 or
//     4.8, both of which sit below rung 3.1's stable play clue to Bob. That
//     ordering is what the old filter was hand-coding.
//   * priority 2 additionally requires the discarded card to be affordable —
//     trash, a same-hand-dupe, or a card Alice can see a second copy of. So a
//     reactive that costs the team a real card is never proposed.
//
// One deliberate semantic change is recorded here rather than papered over. The
// old filter asked about the RECEIVER'S POSITION ("do they already have
// something safe to do?"), so a standing CTP or CTD on the receiver suppressed
// the clue. The new rule asks about the CARD BEING THROWN. The two agree on
// every fixture below, but they are not the same question, and there is no
// successor to the two `receiver_is_safe` CTP/CTD cases — the rule they pinned
// no longer exists.
//
// The POSITIVE double-discard case is still pinned on the real position it came
// from, in test_replay_1942181_prefers_stable_play_over_double_discard.cpp:
// Phase A and Phase B can hard-reject a rank reactive outright (returning
// MISTAKE rather than falling through to Phase C), so hand-built fixtures very
// easily produce no double discard at all — and a test that silently stops
// exercising the branch is worse than no test.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueShape;

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

ClueShape shape_of(const Game& g, const ClueAction& ca) {
  Game hypo = g.simulate(Action{ca});
  return hanabi::reactor0::read_clue(g, hypo, ca).shape;
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

// --- discard_is_affordable: what priority 2 will consent to throw ---------

// The case from replay 1942181: a basic-trash chop is thrown away whether or
// not anybody spends a token saying so, so a reactive that throws it costs the
// team nothing.
TEST(Reactor0DoubleDiscardFilter, TrashChopIsAffordable) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};  // g1 played → Cathy's g1 chop is trash
  Game g = setup(std::move(opts));
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, cathy_of(g), order_at(g, TestPlayer::CATHY, 1)));
}

// A same-hand duplicate is equally expendable: Cathy holds g1 twice, so losing
// one of them loses nothing. This is the clause reactor's `chop_is_nontrash`
// lacks, and priority 2 inherits it.
TEST(Reactor0DoubleDiscardFilter, SameHandDupeIsAffordable) {
  Game g = setup(base_position());  // Cathy holds g1, g1
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, cathy_of(g), order_at(g, TestPlayer::CATHY, 1)));
}

// The third arm: not trash and not duplicated in hand, but Alice can see the
// other copy somewhere else, so the team still does not lose the card.
TEST(Reactor0DoubleDiscardFilter, CopyVisibleElsewhereIsAffordable) {
  SetupOptions opts = base_position();
  opts.hands[1] = {"r1", "y3", "g3", "b3", "y2"};  // Bob also holds a y2
  Game g = setup(std::move(opts));
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, cathy_of(g), order_at(g, TestPlayer::CATHY, 5)))
      << "Cathy's y2 is duplicated in Bob's hand, where Alice can see it";
}

// The negative that makes the rest meaningful. Without it priority 2 would
// propose reactives that cost the team a card outright — which is the failure
// the old filter existed to prevent, expressed as an admissibility rule
// instead of a post-hoc pass.
TEST(Reactor0DoubleDiscardFilter, UnduplicatedUsefulCardIsNotAffordable) {
  SetupOptions opts = base_position();
  opts.hands[2] = {"r3", "y3", "b3", "p3", "g3"};
  Game g = setup(std::move(opts));
  EXPECT_FALSE(hanabi::reactor0::discard_is_affordable(
      g, cathy_of(g), order_at(g, TestPlayer::CATHY, 1)))
      << "r3 is useful, unduplicated and invisible elsewhere";
}

// --- shape facts the priority list is written on -------------------------

// Positional dispatch: a clue to Bob is stable, so it can never be any of the
// reactive shapes priority 1 or 2 select on.
TEST(Reactor0DoubleDiscardFilter, StableClueToBobIsNeverAReactiveShape) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(bob_of(g))) {
    ClueShape sh = shape_of(g, clue_to(g, bob_of(g), c.kind, c.value));
    EXPECT_NE(sh, ClueShape::REACTIVE_PLAY);
    EXPECT_NE(sh, ClueShape::REACTIVE_DISCARD);
    EXPECT_NE(sh, ClueShape::DOUBLE_DISCARD);
  }
}

// Colour reactives always call a play (mode 1 stamps the receiver's target,
// mode 2 the reacter's blind play), so no colour clue is ever a double
// discard.
TEST(Reactor0DoubleDiscardFilter, ColourReactiveIsNeverADoubleDiscard) {
  SetupOptions opts = base_position();
  opts.play_stacks = {0, 0, 1, 0, 0};
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(cathy_of(g))) {
    if (c.kind != ClueKind::COLOUR) continue;
    EXPECT_NE(shape_of(g, clue_to(g, cathy_of(g), c.kind, c.value)),
              ClueShape::DOUBLE_DISCARD)
        << "colour reactives are one-play by construction";
  }
}

// Rank 1 to Bob names his playable r1 — a stable play clue, which is the shape
// rung 3.1 selects and the one a double discard must stay ranked below.
TEST(Reactor0DoubleDiscardFilter, DirectPlayClueToBobIsAStablePlay) {
  Game g = setup(base_position());
  ClueAction ca = clue_to(g, bob_of(g), ClueKind::RANK, 1);
  Game hypo = g.simulate(Action{ca});
  ASSERT_EQ(interp_of(hypo), ClueInterp::PLAY) << "guard: direct play reading";
  EXPECT_EQ(hanabi::reactor0::read_clue(g, hypo, ca).shape,
            ClueShape::STABLE_PLAY);
}

// A clue to Cathy is reactive, so it is never a stable play clue to Bob however
// much it achieves — rung 3.1 selects on the target as well as the shape.
TEST(Reactor0DoubleDiscardFilter, ClueToCathyIsNeverAStablePlay) {
  Game g = setup(base_position());
  for (const Clue& c : g.state.all_valid_clues(cathy_of(g))) {
    EXPECT_NE(shape_of(g, clue_to(g, cathy_of(g), c.kind, c.value)),
              ClueShape::STABLE_PLAY);
  }
}

// The load-bearing negative: a stable clue to Bob that creates no playable is
// NOT a stable play. This is what stops rung 3.1 firing on the strength of a
// stall, a referential discard or a trash reveal — exactly the readings a
// reactive is meant to stay ranked above.
TEST(Reactor0DoubleDiscardFilter, PlaylessStableClueToBobIsNotAStablePlay) {
  SetupOptions opts = base_position();
  opts.hands[1] = {"y3", "g3", "b3", "p3", "r3"};  // nothing playable for Bob
  Game g = setup(std::move(opts));
  for (const Clue& c : g.state.all_valid_clues(bob_of(g))) {
    EXPECT_NE(shape_of(g, clue_to(g, bob_of(g), c.kind, c.value)),
              ClueShape::STABLE_PLAY)
        << "no card of Bob's is playable, so no clue to him can create a play";
  }
}
