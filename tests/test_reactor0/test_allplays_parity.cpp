// Defect A (findings #1/#7/#9). reactor0's parity is fixed by clue kind:
// colour = exactly one play, rank = an even number. `/allplays` is a reactor
// concept -- it promotes colour reactives to play+play -- and reactor0 has no
// use for it. But reaction resolution inherited reactor's `|| wc.all_plays` /
// `&& !wc.all_plays` parity terms while clue-time selection branches on
// clue.kind alone, so with the flag set the two layers read the SAME colour
// clue as exact opposites: the giver and reacter agree the receiver should
// play, and then resolution calls her to discard the very card she was told
// to play.
//
// The agreement is now two-layered:
//   1. reactor0 never sets `all_plays` -- not on the game (commands.cpp), and
//      not in the WC. So the parity terms have nothing to read.
//   2. If a WC does carry the flag anyway (a replayed snapshot, a reactor WC
//      resolved under reactor0), then under /allplays the reacter has no
//      discard available to them at all -- so a reacter discard is a KNOWN
//      MISTAKE, not the other half of a parity, and teaches the receiver
//      nothing.
#include <gtest/gtest.h>

#include "hanabi/basics/convention.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Same position as Reactor0ReactiveColour.ReacterDiscardsReceiverPlays:
// red = anchor 1, Cathy's playable r1 at slot 1 -> react_slot
// calc_slot(1,1,5) = 5, so Bob discards slot 5 and Cathy plays slot 1.
SetupOptions allplays_opts(bool all_plays) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "y3", "b3", "g4", "y4"},   // Bob (reacter)
      {"r1", "y2", "g3", "b4", "p2"},   // Cathy: r1 playable at slot 1
  };
  opts.init = [all_plays](Game& g) {
    g.convention = Convention::REACTOR0;
    g.allow_reactive_locks = true;
    g.all_plays = all_plays;
  };
  return opts;
}

std::optional<DiscardInterp> last_discard_interp(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* di = std::get_if<DiscardInterp>(&g.move_history.back())) return *di;
  return std::nullopt;
}

}  // namespace

// Layer 1: the flag never reaches a reactor0 WC, so parity is untouched.
TEST(Reactor0AllPlaysParity, WaitingConnectionNeverCarriesAllPlays) {
  Game g = setup(allplays_opts(/*all_plays=*/true));

  g = take_turn(std::move(g), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty()) << "the reactive must leave a WC";
  for (const auto& wc : g.waiting) {
    EXPECT_FALSE(wc.all_plays)
        << "reactor0 must not propagate all_plays into its waiting "
           "connections; the flag is a reactor concept";
  }
}

TEST(Reactor0AllPlaysParity, ColourReactiveResolvesToPlayWithAllPlaysOn) {
  Game g = setup(allplays_opts(/*all_plays=*/true));

  g = take_turn(std::move(g), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD)
      << "clue-time selection must be unaffected by all_plays";
  // Since v8.0.0 the receiver's call is made when the reacter acts (§1d).
  ASSERT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::NONE);

  // Bob performs the discard the clue called for.
  g = take_turn(std::move(g), "Bob discards y4 (slot 5)", "r3");

  // Resolution must agree with the clue-time reading: colour + reacter
  // discard = the receiver PLAYS. Under the bug it takes the even-parity
  // branch and re-stamps the target CALLED_TO_DISCARD.
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), CardStatus::CALLED_TO_PLAY)
      << "all_plays must not flip reactor0's colour parity at reaction time";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED))
      << "and must not let is_lock_target fire on a play target";
}

TEST(Reactor0AllPlaysParity, AllPlaysOffAndOnAgreeEndToEnd) {
  // The two runs must be indistinguishable: all_plays has no meaning in
  // reactor0, so the same clue must resolve identically either way.
  Game off = setup(allplays_opts(/*all_plays=*/false));
  off = take_turn(std::move(off), "Alice clues red to Cathy");
  off = take_turn(std::move(off), "Bob discards y4 (slot 5)", "r3");

  Game on = setup(allplays_opts(/*all_plays=*/true));
  on = take_turn(std::move(on), "Alice clues red to Cathy");
  on = take_turn(std::move(on), "Bob discards y4 (slot 5)", "r3");

  EXPECT_EQ(status_at(off, TestPlayer::CATHY, 1),
            status_at(on, TestPlayer::CATHY, 1))
      << "reactor0 resolution must not consult all_plays";
}

// Layer 2: if a WC carries the flag anyway, the reacter has no discard by
// agreement -- so a discard is a known mistake and marks nothing.
TEST(Reactor0AllPlaysParity, ReacterDiscardUnderAllPlaysIsAKnownMistake) {
  Game g = setup(allplays_opts(/*all_plays=*/false));

  g = take_turn(std::move(g), "Alice clues red to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  // Nothing is on the receiver yet -- since v8.0.0 the call is made when the
  // reacter acts, and here the reacter never legitimately acts at all.
  CardStatus target_before = status_at(g, TestPlayer::CATHY, 1);
  ASSERT_EQ(target_before, CardStatus::NONE);

  // Force the inherited-WC case: a live reactor0 reactive whose WC says
  // play+play. Bob discarding now contradicts the agreement outright.
  for (auto& wc : g.waiting) wc.all_plays = true;

  g = take_turn(std::move(g), "Bob discards y4 (slot 5)", "r3");

  EXPECT_EQ(last_discard_interp(g), DiscardInterp::MISTAKE)
      << "under /allplays the reacter has no discard available by agreement, "
         "so a discard is a known mistake rather than a parity signal";
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 1), target_before)
      << "a known mistake teaches the receiver nothing -- her target is never "
         "stamped at all";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CHOP_MOVED))
      << "and must not fall through to the lock reading";
}
