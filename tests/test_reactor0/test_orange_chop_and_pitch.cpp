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
using hanabi::reactor0::slot_has_spare_inverted;
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

// --- v10.8.0: a CLUED slot with a spare orange is pitchable ----------------
//
// Pressing Play on an inverted card throws it away, so the question for the
// reacter's slot is affordability, not playability. Until v10.8.0 both pitch
// arms demanded that EVERY reading be inverted, which a clued card with a mixed
// empathy never satisfies -- so replay 1974331 T8 walked past a spare o3 and
// blind-played a b2 onto an empty stack instead.
//
// The three steps of `stamp_react_play_button`, and the boundary between them:
//   1. every reading inverted -> pitch (unchanged; `AKnownOrangeWithSpare...`
//      above and replay 1973976 cover it)
//   2. otherwise the play reading, if any reading can play
//   3. otherwise, for a CLUED or STAMPED card with an inverted reading it can
//      spare, a pitch
//
// `slot_has_spare_inverted` is step 3's test in isolation. Asking
// `slot_is_pitchable` there would be wrong: step 2 has already ruled the play
// reading out, so its plain half could only answer about a play that cannot
// happen.
TEST(Reactor0SparePitch, ASpareInvertedReadingIsWhatLicensesTheP) {
  Game g = setup(chop_opts("o2"));  // orange on 1: o1 dead, o2 playable
  const State& s = g.state;

  // A mixed empathy holding one orange that can be spared -- o3 has two copies
  // and neither is gone.
  const IdentitySet mixed = IdentitySet::from_iter(
      {Identity{0, 3}, Identity{1, 3}, Identity{4, 3}});
  ASSERT_FALSE(s.is_critical(Identity{4, 3})) << "guard: the o3 is spare";
  EXPECT_TRUE(slot_has_spare_inverted(s, mixed))
      << "the o3 can be thrown away, which is what a pitch does";

  // The plain readings are r3 and y3 with both stacks on 0, so neither plays --
  // step 2 has nothing to offer and step 3 is reached on the orange alone.
  EXPECT_FALSE(mixed.exists([&s](Identity i) { return s.is_playable(i); }))
      << "guard: no reading can play, so this really is the fallback case";
}

// The boundary: a CRITICAL orange is not spare, so it licenses nothing. This is
// what stops the rule reaching into Dark Orange, where every card is one-of-each.
TEST(Reactor0SparePitch, ACriticalOrangeIsNotSpare) {
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

  const IdentitySet mixed = IdentitySet::from_iter(
      {Identity{0, 3}, Identity{1, 3}, Identity{4, 3}});
  ASSERT_TRUE(s.is_critical(Identity{4, 3})) << "guard: Dark Orange is one-of-each";
  EXPECT_FALSE(slot_has_spare_inverted(s, mixed))
      << "there is no copy to spare, so pressing Play would lose it outright";
}

// And the split itself: `slot_is_pitchable` still answers yes on a playable
// PLAIN reading, which is exactly the answer step 3 must not accept -- it would
// be reporting a play that step 2 has already ruled out. Replay 1974331's slot
// held a playable y1 in `possible` that `inferred` excluded, so the two
// predicates disagree there and only the narrow one is correct.
TEST(Reactor0SparePitch, TheTwoPredicatesComeApartOnAPlainPlayable) {
  Game g = setup(chop_opts("o2"));
  const State& s = g.state;
  const IdentitySet plain_playable = IdentitySet::from_iter({Identity{0, 1}});

  ASSERT_TRUE(s.is_playable(Identity{0, 1})) << "guard: red on 0, so r1 plays";
  EXPECT_TRUE(slot_is_pitchable(s, plain_playable))
      << "the wide predicate says yes -- pressing Play really does play it";
  EXPECT_FALSE(slot_has_spare_inverted(s, plain_playable))
      << "but there is nothing inverted to pitch, which is step 3's question";
}

// The cluedness boundary, as a controlled pair: the two tests below differ in
// ONE bit -- whether the react slot is clued -- and nothing else.
//
// Step 3 licenses a pitch only for a card the team has CLUED or stamped. That
// condition is where v10.3.0's worry lives: an unclued card in an Orange variant
// has a wide enough empathy to always admit some non-critical orange, so letting
// that alone license a pitch would disable the strike checks across the board.
//
// The seeded shape is replay 1974331's: `possible` holds a playable that
// `inferred` excludes. That is what gets the pairing past `vet_react_slot` --
// which asks its existential of `possible` -- so the stamp is reached and step 3
// is the thing being measured.
namespace {

// Orange on 1. Cathy's slot 1 is her only playable, so Phase A pairs it with
// `calc_slot(3, 1, 5) = 2` -- Bob's slot 2, the slot under test. Nothing else in
// her hand plays or is one away, so the reading cannot wander off elsewhere.
//
// POV is BOB, the reacter, which is the seat replay 1974331 is decided from and
// the only seat this can be asked at. `vet_react_slot`'s last test reads the
// react card's ACTUAL identity, and from the giver's chair a spare orange is
// visibly unable to play -- so the giver rejects the clue outright before any
// stamp is reached. Bob cannot see his own card, so he asks the question the fix
// is about. (That asymmetry is real and is recorded in TODO.md: as things stand
// the bot can READ this clue but would never GIVE it.)
SetupOptions pitch_pair_opts() {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 1};
  opts.hands = {
      {"r4", "y4", "g4", "b4", "y3"},          // Alice (giver)
      {"xx", "xx", "xx", "xx", "xx"},          // Bob (us, the reacter)
      {"r1", "y4", "g4", "b4", "y3"},          // Cathy -- slot 1 is the r1
  };
  opts.init = [](Game& g) {
    g.convention = Convention::REACTOR0;
    g.allow_reactive_locks = true;
    g.state.our_player_index = static_cast<int>(TestPlayer::BOB);
  };
  return opts;
}

// `inferred` = {r3, o3}: neither can play (red on 0, orange on 1), so step 2 has
// nothing. `possible` adds a playable r1, so the vet lets the pairing through.
Game seed_pitch_slot(Game g, bool clued) {
  const int o = g.state.hands[static_cast<int>(TestPlayer::BOB)][1];
  g.with_thought(o, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::from_iter({Identity{0, 3}, Identity{4, 3}});
    out.possible =
        IdentitySet::from_iter({Identity{0, 3}, Identity{4, 3}, Identity{0, 1}});
    return out;
  });
  g.players[static_cast<int>(TestPlayer::BOB)] = g.common;
  g.players[static_cast<int>(TestPlayer::ALICE)] = g.common;
  if (clued) g.with_card(o, [](Card& c) { c.clued = true; });
  return g;
}

}  // namespace

TEST(Reactor0SparePitch, ACluedSlotIsPitchedOnItsSpareOrange) {
  Game g = seed_pitch_slot(setup(pitch_pair_opts()), /*clued=*/true);
  const int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_TRUE(g.state.deck[bob_s2].clued) << "guard: this is the clued arm";

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  EXPECT_EQ(g.meta[bob_s2].status, CardStatus::CALLED_TO_PLAY)
      << "no reading can play, but the o3 can be spared -- so the Play button "
         "is a pitch and the pairing stands";
}

TEST(Reactor0SparePitch, AnUncluedSlotIsNotPitchedOnTheSameReading) {
  Game g = seed_pitch_slot(setup(pitch_pair_opts()), /*clued=*/false);
  const int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  ASSERT_FALSE(g.state.deck[bob_s2].clued) << "guard: the only difference";
  ASSERT_EQ(g.meta[bob_s2].status, CardStatus::NONE);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  EXPECT_NE(g.meta[bob_s2].status, CardStatus::CALLED_TO_PLAY)
      << "the team spent nothing on this card, so a spare orange somewhere in "
         "its empathy must not license a blind Play -- v10.3.0's boundary";
}
