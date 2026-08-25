// Reactor0 under Odds and Evens: the two clue kinds swap reactive roles.
//
//   normally   RANK   = even parity -- double play, or double discard
//              COLOUR = odd parity  -- exactly one play
//   O&E        COLOUR = even parity
//              RANK   = odd parity
//
// and a rank clue's value names a parity rather than a rank, so it maps
// odd (1) -> anchor 3, even (2) -> anchor 4. Colour anchors are unchanged.
//
// Each test here has a mirror on the same fixture in a plain variant, so what
// is pinned is the SWAP rather than any particular reading.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/colour_value.h"
#include "test_harness.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Alice gives, Bob reacts, Cathy receives -- reactor0's positional dispatch,
// so a clue to Cathy is reactive. Stacks are all 0, so Cathy's leftmost
// playable is her slot 1 and Bob has playables to be called onto.
SetupOptions oe_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},  // Alice (giver, us)
      {"r1", "y1", "g1", "b1", "p1"},  // Bob   (reacter) -- all playable
      {"r1", "y2", "g3", "b4", "p5"},  // Cathy (receiver) -- red for the
                                       // colour clue, playable r1 on slot 1
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

// --- the anchor -----------------------------------------------------------

TEST(Reactor0OddsAndEvens, RankAnchorIsThreeForOddAndFourForEven) {
  Game g = setup(oe_opts("Odds and Evens (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 1 to Cathy");
  ASSERT_FALSE(g.waiting.empty()) << "an odd clue to Cathy must read reactive";
  EXPECT_EQ(g.waiting.front().focus_slot, 3) << "odd -> 3";

  Game g2 = setup(oe_opts("Odds and Evens (5 Suits)"));
  g2 = take_turn(std::move(g2), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g2.waiting.empty());
  EXPECT_EQ(g2.waiting.front().focus_slot, 4) << "even -> 4";
}

// The mirror: in a plain variant the anchor is still the rank itself.
TEST(Reactor0OddsAndEvens, PlainVariantRankAnchorIsStillTheRank) {
  Game g = setup(oe_opts("No Variant"));
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 2) << "the clue value IS the rank";
}

// Colour anchors are untouched by the variant: same colour, same value.
TEST(Reactor0OddsAndEvens, ColourAnchorsAreUnchanged) {
  const Variant& oe = get_variant("Odds and Evens (5 Suits)");
  const Variant& plain = get_variant("No Variant");
  for (int c = 0; c < 5; ++c) {
    EXPECT_EQ(hanabi::reactor0::colour_clue_value(oe, c),
              hanabi::reactor0::colour_clue_value(plain, c))
        << "colour " << c << " must anchor identically in both variants";
  }
}

// --- the parity swap ------------------------------------------------------
//
// The observable difference between the two rulesets is what gets stamped.
// The even-parity family calls a play on BOTH seats (double play) or a discard
// on both; the odd-parity family calls exactly one play.

namespace {

// How many cards in one hand carry a given call.
int count_status(const Game& g, TestPlayer who, CardStatus st) {
  int n = 0;
  for (int o : g.state.hands[static_cast<int>(who)]) {
    if (g.meta[o].status == st) ++n;
  }
  return n;
}

// Total PLAY calls the clue made across the reacter and the receiver. This is
// the convention's own way of naming the two rulesets -- /settings calls them
// "odd plays" and "even plays" -- and unlike any single stamp it does not
// depend on WHICH seat the odd-parity family decided to give the play to.
//
// Since v8.0.0 the receiver is stamped only when the reacter acts (§1d), so at
// clue time the stamps show one side of the pair. The PAIR is what names the
// ruleset, so recover the receiver's half from the waiting connection the way
// the decision layer does -- the information is settled, it just is not written
// onto the card yet.
int play_calls(const Game& g) {
  int n = count_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY) +
          count_status(g, TestPlayer::CATHY, CardStatus::CALLED_TO_PLAY);
  if (g.waiting.empty()) return n;
  const ReactorWC& wc = g.waiting.front();
  if (wc.react_order < 0 || !wc.even_parity) return n;
  if (!hanabi::reactor0::predicted_receiver_order(g)) return n;
  if (hanabi::reactor0::predicts_reactive_lock(g)) return n;
  const CardStatus rb = hanabi::reactor0::receiver_button(
      *wc.even_parity, g.meta[wc.react_order].status);
  if (rb == CardStatus::CALLED_TO_PLAY) ++n;
  return n;
}

}  // namespace

// A COLOUR clue under O&E runs the ruleset a RANK clue normally would: an EVEN
// number of plays (two, or none).
TEST(Reactor0OddsAndEvens, ColourClueTakesTheEvenParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe) % 2, 0)
      << "O&E colour is the even-parity family; got " << play_calls(oe)
      << " play calls";

  // The mirror on the identical fixture: in a plain variant the same clue is
  // the ODD-parity family, so it calls exactly one play.
  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain), 1)
      << "plain colour is the odd-parity family; got " << play_calls(plain);
}

// ...and a RANK clue under O&E runs the ruleset a COLOUR clue normally would:
// exactly one play.
TEST(Reactor0OddsAndEvens, RankClueTakesTheOddParityRuleset) {
  Game oe = setup(oe_opts("Odds and Evens (5 Suits)"));
  oe = take_turn(std::move(oe), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_clue_interp(oe), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(oe), 1)
      << "O&E rank is the odd-parity family; got " << play_calls(oe);

  Game plain = setup(oe_opts("No Variant"));
  plain = take_turn(std::move(plain), "Alice clues 1 to Cathy");
  ASSERT_EQ(last_clue_interp(plain), ClueInterp::REACTIVE);
  EXPECT_EQ(play_calls(plain) % 2, 0)
      << "plain rank is the even-parity family; got " << play_calls(plain);
}

// --- the lock-slot rank promise is a PARITY promise ------------------------

// Replay 1971788 T13. yagami gave an odd rank clue to will-bot69; it touched
// the lock slot, so the hand was correctly stamped CHOP_MOVED -- and then
// `apply_rank_promise` narrowed the lock slot to *rank 1*, because it filtered
// on `i.rank == clue.value`. Under Odds and Evens the value names a PARITY: an
// odd clue promises 1, 3 OR 5.
//
// The card was a Dark Omni 5. Every rank-1 identity was already trash, so from
// that turn on it read as known trash, and sixteen turns later it was thrown
// and the max score fell 30 to 29.
//
// The promise only exists at all when the variant has a pinkish suit --
// `ref_discard` gates `apply_rank_promise` on `includes_pinkish`
// (src/conventions/reactor/interpret_clue.cpp:354) -- which is why these
// fixtures use an Omni variant rather than plain Odds and Evens.
namespace {

// Bob's oldest card is the lock slot. Nothing is playable beyond the 1s, so
// the direct-play arm cannot fire and the clue falls through to the lock.
SetupOptions lock_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},   // Alice (giver, us)
      {"r1", "y1", "g1", "b1", "p3"},   // Bob -- slot 5 (p3) is the lock slot
      {"r2", "y2", "g2", "b2", "p2"},   // Cathy
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0OddsAndEvens, AnOddLockPromisesOneThreeOrFive) {
  Game g = setup(lock_opts("Odds and Evens & Omni (6 Suits)"));
  const int lock_slot = order_at(g, TestPlayer::BOB, 5);
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  ASSERT_EQ(g.meta[lock_slot].status, CardStatus::CHOP_MOVED)
      << "guard: the clue touched the lock slot, so this is a LOCK";

  const IdentitySet inf = g.common.thoughts[lock_slot].inferred;
  ASSERT_TRUE(inf.non_empty());
  EXPECT_TRUE(inf.forall([](Identity i) { return i.rank % 2 == 1; }))
      << "an odd clue promises an ODD rank";
  EXPECT_TRUE(inf.exists([](Identity i) { return i.rank == 3; }))
      << "3 is odd, so the promise must still admit it -- narrowing to rank 1 "
         "is what condemned replay 1971788's Dark Omni 5 as trash";
  EXPECT_TRUE(inf.exists([](Identity i) { return i.rank == 5; }));
}

TEST(Reactor0OddsAndEvens, AnEvenLockPromisesTwoOrFour) {
  SetupOptions opts = lock_opts("Odds and Evens & Omni (6 Suits)");
  opts.hands[1] = {"r1", "y1", "g1", "b1", "p2"};  // lock slot is even now
  Game g = setup(std::move(opts));
  const int lock_slot = order_at(g, TestPlayer::BOB, 5);
  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  ASSERT_EQ(g.meta[lock_slot].status, CardStatus::CHOP_MOVED);
  const IdentitySet inf = g.common.thoughts[lock_slot].inferred;
  ASSERT_TRUE(inf.non_empty());
  EXPECT_TRUE(inf.forall([](Identity i) { return i.rank % 2 == 0; }));
  EXPECT_TRUE(inf.exists([](Identity i) { return i.rank == 4; }));
}

// The carve-out: outside Odds and Evens a rank clue still promises the literal
// rank, so the pinkish path is unchanged.
TEST(Reactor0OddsAndEvens, APlainOmniLockStillPromisesTheLiteralRank) {
  Game g = setup(lock_opts("Omni (6 Suits)"));
  const int lock_slot = order_at(g, TestPlayer::BOB, 5);
  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  ASSERT_EQ(g.meta[lock_slot].status, CardStatus::CHOP_MOVED);
  const IdentitySet inf = g.common.thoughts[lock_slot].inferred;
  ASSERT_TRUE(inf.non_empty());
  EXPECT_TRUE(inf.forall([](Identity i) { return i.rank == 3; }))
      << "a rank-3 clue outside Odds and Evens promises exactly rank 3";
}

// --- a direct rank play clue focuses from the RIGHT ------------------------

// One rank clue in Odds and Evens names a whole parity class, so an odd clue
// sweeps up 1s, 3s and 5s at once. When every good remaining card of that
// parity is playable the clue is a direct rank play clue, and it promises the
// RIGHTMOST touched card rather than the leftmost.
namespace {

// Every stack on 4, so the only useful odd identities left are the 5s -- and
// they are all playable, which is exactly the direct-play condition. An odd
// clue then touches Bob's whole hand.
SetupOptions right_focus_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 4, 4, 4, 4};
  opts.hands = {
      {"r2", "y2", "g2", "b2", "p2"},   // Alice (giver, us)
      {"r5", "y1", "g3", "b1", "p5"},   // Bob -- all odd, so all touched
      {"r1", "y3", "g1", "b3", "p1"},   // Cathy
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0OddsAndEvens, DirectRankPlayFocusesTheRightmostTouched) {
  Game g = setup(right_focus_opts("Odds and Evens & Omni (6 Suits)"));
  const int leftmost = order_at(g, TestPlayer::BOB, 1);
  const int rightmost = order_at(g, TestPlayer::BOB, 5);
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(g.meta[rightmost].status, CardStatus::CALLED_TO_PLAY)
      << "an odd clue names the rightmost touched card in Odds and Evens";
  EXPECT_NE(g.meta[leftmost].status, CardStatus::CALLED_TO_PLAY)
      << "the leftmost is what the rule used to name";
}

// The mirror: outside Odds and Evens a rank clue names one rank, and the focus
// stays leftmost. Same fixture, same clue shape.
TEST(Reactor0OddsAndEvens, PlainOmniDirectRankPlayStillFocusesLeftmost) {
  SetupOptions opts = right_focus_opts("Omni (6 Suits)");
  opts.hands[1] = {"r5", "y5", "g5", "b5", "p5"};  // a rank-5 clue touches all
  Game g = setup(std::move(opts));
  const int leftmost = order_at(g, TestPlayer::BOB, 1);
  const int rightmost = order_at(g, TestPlayer::BOB, 5);
  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  EXPECT_EQ(g.meta[leftmost].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_NE(g.meta[rightmost].status, CardStatus::CALLED_TO_PLAY);
}

// "Rightmost touched card whose empathy does not completely consist of
// unplayable cards" -- a candidate that cannot be playable is skipped, not
// taken and wasted.
TEST(Reactor0OddsAndEvens, ADeadRightmostCandidateIsSkipped) {
  using hanabi::reactor0::rightmost_could_be_playable;
  Game g = setup(right_focus_opts("Odds and Evens & Omni (6 Suits)"));
  const int rightmost = order_at(g, TestPlayer::BOB, 5);
  const int fourth = order_at(g, TestPlayer::BOB, 4);

  // Pin the rightmost to a rank-1: trash with every stack on 4, so no reading
  // of it can be playable.
  g.with_thought(rightmost, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(Identity{0, 1});
    return out;
  });

  ClueAction action(/*giver=*/0, /*target=*/1, g.state.hands[1],
                    BaseClue{ClueKind::RANK, 1});

  auto got = rightmost_could_be_playable(g, action, action.list_);
  ASSERT_TRUE(got.has_value());
  EXPECT_NE(*got, rightmost) << "every reading of it is trash";
  EXPECT_EQ(*got, fourth) << "so the promise moves one card left";
}
