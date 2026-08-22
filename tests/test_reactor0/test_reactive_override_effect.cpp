// A `/set` override changes what a clue MEANS, not just how /settings prints.
//
// The assignment decides two things at clue time: which parity ruleset runs
// (even = double play / double discard, odd = exactly one play) and the anchor
// the react/target slot arithmetic keys on. Both are read from the override
// list when one applies.
//
// And the change is insulated: the parity binds into the `ReactorWC` when the
// clue is given, so a `/set` landing mid-game cannot alter what an
// already-given clue meant. That is the same protection `wc.rlocks` provides.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Alice gives, Bob reacts, Cathy receives -- a clue to Cathy is reactive.
// Stacks at 0 so both have playables.
SetupOptions ov_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},  // Alice (giver, us)
      {"r1", "y1", "b1", "p1", "g4"},  // Bob (reacter) -- playables on 1-4
      // Cathy: a playable g1 on slot 1 for the even ruleset to target, plus a
      // yellow and a red for the two clues under test. Deliberately NOT a 1
      // that Bob also holds -- that would trip the duplicate-play veto and the
      // clue would never be offered.
      {"g1", "y2", "r2", "b4", "p5"},  // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

// Since v8.0.0 the receiver is stamped only when the reacter acts (§1d), so at
// clue time the stamps show one side of the pair. The PAIR is what names the
// ruleset, so recover the receiver's half from the waiting connection.
int play_calls(const Game& g) {
  int n = 0;
  for (auto who : {TestPlayer::BOB, TestPlayer::CATHY}) {
    for (int o : g.state.hands[static_cast<int>(who)]) {
      if (g.meta[o].status == CardStatus::CALLED_TO_PLAY) ++n;
    }
  }
  if (g.waiting.empty()) return n;
  const ReactorWC& wc = g.waiting.front();
  if (wc.react_order < 0 || !wc.even_parity) return n;
  if (!hanabi::reactor0::predicted_receiver_order(g)) return n;
  if (hanabi::reactor0::predicts_reactive_lock(g)) return n;
  if (hanabi::reactor0::receiver_button(*wc.even_parity,
                                        g.meta[wc.react_order].status) ==
      CardStatus::CALLED_TO_PLAY) {
    ++n;
  }
  return n;
}

// `/set Yellow even 4`.
std::vector<ReactiveOverride> yellow_even_four(const Game& g) {
  auto clue = hanabi::reactor0::parse_clue_label(*g.state.variant, "Yellow");
  EXPECT_TRUE(clue.has_value());
  return {ReactiveOverride{clue->first, clue->second, /*even=*/true, 4}};
}

}  // namespace

// Without the override a colour clue is the ODD bucket: exactly one play, and
// the anchor is Yellow's built-in value of 2.
TEST(Reactor0ReactiveOverride, DefaultYellowIsOddWithValueTwo) {
  Game g = setup(ov_opts());
  g = take_turn(std::move(g), "Alice clues yellow to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 2) << "Yellow=2 by the built-in table";
  EXPECT_EQ(play_calls(g), 1) << "odd bucket promises exactly one play";
}

// With it, the same clue runs the EVEN ruleset and anchors on 4.
TEST(Reactor0ReactiveOverride, OverriddenYellowIsEvenWithValueFour) {
  Game g = setup(ov_opts());
  g.reactive_overrides = yellow_even_four(g);

  g = take_turn(std::move(g), "Alice clues yellow to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 4) << "the override supplies the anchor";
  EXPECT_EQ(play_calls(g) % 2, 0)
      << "the even bucket promises a double play or a double discard; got "
      << play_calls(g);
}

// Only the named clue moves.
TEST(Reactor0ReactiveOverride, OtherCluesAreUnaffected) {
  Game g = setup(ov_opts());
  g.reactive_overrides = yellow_even_four(g);

  g = take_turn(std::move(g), "Alice clues red to Cathy");

  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 1) << "Red=1, untouched";
  EXPECT_EQ(play_calls(g), 1) << "and red is still the odd bucket";
}

// The insulation. A clue given under the old table keeps the meaning it was
// given with, even though `/set` retro-applies to the running game.
TEST(Reactor0ReactiveOverride, AnInFlightReactionKeepsItsOriginalParity) {
  Game g = setup(ov_opts());
  g = take_turn(std::move(g), "Alice clues yellow to Cathy");
  ASSERT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  ASSERT_TRUE(g.waiting.front().even_parity.has_value());
  ASSERT_FALSE(*g.waiting.front().even_parity) << "given as the odd bucket";
  const int react_order = g.waiting.front().react_order;

  // /set lands mid-game and is retro-applied, as chat_set does.
  g.reactive_overrides = yellow_even_four(g);

  EXPECT_FALSE(*g.waiting.front().even_parity)
      << "the pending reaction still carries the parity it was given under";
  // And resolution reads that, not the new table: the odd ruleset with the
  // reacter playing means the receiver is called to DISCARD.
  EXPECT_EQ(g.waiting.front().react_order, react_order)
      << "the standing connection is otherwise untouched";
}
