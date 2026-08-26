// Two consequences of the same fact: on an inverted suit, Discard puts the card
// on its stack and Play throws it away.
//
//   * A PLAYABLE ORANGE ON CHOP costs its holder nothing. Discarding it is not
//     a loss, it is the play. So it belongs with basic trash and a same-hand
//     dupe in every predicate that asks whether a chop is worth spending a clue
//     on. Replay 1973974 T10, where a bot LOCKED its partner over one.
//   * PRESSING PLAY ON A KNOWN ORANGE is a pitch, not a play. It cannot strike
//     and need not be playable, so the vet's question is affordability. Replay
//     1973976 T12, where a pitch that would have chucked a playable o2 onto the
//     stack was skipped twice over.
//
// Both are per-seat facts about the ACTUAL card, so the chop predicates read
// `state.deck[o].id()` and return their old answers whenever the caller cannot
// see it.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/variants/inverted.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::at_risk_chop;
using hanabi::reactor0::chop_is_expendable;
using hanabi::reactor0::has_playable_chop;
using hanabi::reactor0::slot_is_pitchable;

namespace {

// "Orange (5 Suits)" -- r/y/g/b/o, orange being the inverted suit. Orange is on
// 1, so o2 is playable and o1 is dead. `bob_chop` is the card on Bob's chop
// (his slot 1, since nothing of his is clued).
SetupOptions chop_opts(std::string bob_chop) {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 1};  // orange on 1
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},                    // Alice (us, giver)
      {std::move(bob_chop), "y4", "g4", "b4", "y3"},     // Bob
      {"r4", "g3", "b3", "y2", "r3"},                    // Cathy
  };
  use_reactor0(opts);
  return opts;
}

const int kBob = static_cast<int>(TestPlayer::BOB);

}  // namespace

// --- the chop predicates --------------------------------------------------

// The case from replay 1973974: Bob's chop is a playable orange.
TEST(Reactor0OrangeChop, APlayableOrangeChopCostsNothing) {
  Game g = setup(chop_opts("o2"));
  auto chop = g.chop(kBob);
  ASSERT_TRUE(chop.has_value());
  ASSERT_EQ(g.state.deck[*chop].id(), (Identity{4, 2})) << "guard: the o2";
  ASSERT_TRUE(g.state.is_playable(Identity{4, 2})) << "guard: orange on 1";

  EXPECT_FALSE(at_risk_chop(g, 0, kBob))
      << "discarding it CHUCKS it onto the stack -- nothing is at risk";
  EXPECT_FALSE(has_playable_chop(g, kBob))
      << "and it collects itself, so there is no play to arrange";
  EXPECT_TRUE(chop_is_expendable(g, kBob)) << "expendable, like trash";
}

// Control 1: a DEAD orange. Already expendable and not at risk before this
// change, so it must read exactly the same -- the new clause must not be doing
// the work here.
TEST(Reactor0OrangeChop, ADeadOrangeChopIsUnchanged) {
  Game g = setup(chop_opts("o1"));
  ASSERT_TRUE(g.state.is_basic_trash(Identity{4, 1})) << "guard: o1 is dead";

  EXPECT_FALSE(at_risk_chop(g, 0, kBob));
  EXPECT_FALSE(has_playable_chop(g, kBob));
  EXPECT_TRUE(chop_is_expendable(g, kBob));
}

// Control 2, and the discriminator: a PLAIN playable on chop still counts. If
// the new clause were testing playability alone rather than playability on an
// inverted suit, this would flip and N5 would go silent everywhere.
TEST(Reactor0OrangeChop, APlainPlayableChopStillCounts) {
  Game g = setup(chop_opts("r1"));
  ASSERT_TRUE(g.state.is_playable(Identity{0, 1})) << "guard: red on 0";

  EXPECT_TRUE(at_risk_chop(g, 0, kBob))
      << "a plain playable really is lost if it is discarded";
  EXPECT_TRUE(has_playable_chop(g, kBob));
  EXPECT_FALSE(chop_is_expendable(g, kBob));
}

// Control 3: a plain USEFUL-but-unplayable chop, the ordinary endangered case.
TEST(Reactor0OrangeChop, APlainUsefulChopIsStillEndangered) {
  Game g = setup(chop_opts("r2"));
  EXPECT_TRUE(at_risk_chop(g, 0, kBob));
  EXPECT_FALSE(has_playable_chop(g, kBob)) << "useful but not playable";
  EXPECT_FALSE(chop_is_expendable(g, kBob));
}

// An UNPLAYABLE orange is not a free chuck either -- chucking it would strike.
TEST(Reactor0OrangeChop, AnUnplayableOrangeChopIsNotAFreeChuck) {
  Game g = setup(chop_opts("o4"));
  ASSERT_FALSE(g.state.is_playable(Identity{4, 4})) << "guard: orange is on 1";
  ASSERT_FALSE(g.state.is_basic_trash(Identity{4, 4})) << "guard: still useful";

  EXPECT_TRUE(at_risk_chop(g, 0, kBob))
      << "only a PLAYABLE orange chucks for free";
  EXPECT_FALSE(chop_is_expendable(g, kBob));
}

// --- pitchability ---------------------------------------------------------

// The shape from replay 1973976: a known orange whose readings are neither
// playable nor trash. `can_pitch_for_free` says no -- it wants every reading to
// be a dead orange -- but the card can plainly be spared.
TEST(Reactor0OrangePitch, AKnownOrangeWithSpareReadingsIsPitchable) {
  Game g = setup(chop_opts("o2"));
  const State& s = g.state;
  const IdentitySet oranges = IdentitySet::from_iter(
      {Identity{4, 1}, Identity{4, 2}, Identity{4, 3}, Identity{4, 4}});

  EXPECT_TRUE(slot_is_pitchable(s, oranges))
      << "o3 and o4 have two copies each, so throwing one away is affordable";
  EXPECT_FALSE(hanabi::reactor::variants::can_pitch_for_free(g, 0))
      << "guard: the old test is about a card whose every reading is dead";
}

// A plain card is pitchable only by being genuinely PLAYABLE -- pressing Play on
// a plain card plays it, so affordability is not the question there.
TEST(Reactor0OrangePitch, APlainSlotIsPitchableOnlyWhenPlayable) {
  Game g = setup(chop_opts("o2"));
  const State& s = g.state;
  EXPECT_TRUE(slot_is_pitchable(s, IdentitySet::from_iter({Identity{0, 1}})))
      << "r1 is playable";
  EXPECT_FALSE(slot_is_pitchable(s, IdentitySet::from_iter({Identity{0, 4}})))
      << "r4 is not, and pressing Play on it strikes";
}

// Dark Orange is one-of-each, so every reading is critical and nothing there can
// be pitched. This is the boundary the existential rule must not cross.
TEST(Reactor0OrangePitch, DarkOrangeIsNeverPitchable) {
  SetupOptions opts;
  opts.variant_name = "Dark Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 1};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "g4", "b4", "y3"},
      {"r4", "g3", "b3", "y2", "r3"},
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  const State& s = g.state;

  const IdentitySet darks = IdentitySet::from_iter(
      {Identity{4, 2}, Identity{4, 3}, Identity{4, 4}, Identity{4, 5}});
  for (Identity i : darks) {
    ASSERT_TRUE(s.is_critical(i)) << "guard: Dark Orange is one-of-each";
  }
  EXPECT_FALSE(slot_is_pitchable(s, darks))
      << "every reading is the last copy, so there is nothing to spare";
}
