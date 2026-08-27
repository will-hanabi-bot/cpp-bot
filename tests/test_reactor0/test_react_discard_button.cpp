// The reacter's DISCARD button is a ladder, not a choice.
//
// `stamp_react_discard_button` (interpret_reactive.cpp) is the mirror of
// v10.8.0's `stamp_react_play_button`: try the CHUCK -- an inverted reading the
// stack is waiting for, which the Discard button stacks -- and fall back to the
// ordinary throw-away. Between them those are `slot_is_chuckable` asked of the
// slot's own inferences, so the call is refused exactly when NEITHER reading
// exists, never because the wrong arm was picked.
//
// Before v10.9.0 the two colour sites picked an arm with a `react_could_chuck`
// gate and had no fallback, and the gate asked its question of `possible` while
// the chuck stamp asks it of `possibilities()` -- `inferred` once non-empty.
// reactor0's own deferred negatives (`slot_elims`) strip the PLAYABLE identities
// from a reacter's unpaired slots, which is that exact axis, so the two sets
// come apart on any reacter that has already been through one reactive. When
// they did, the gate said "chuck", the chuck found nothing to name, and the
// whole clue read as a MISTAKE. Replay 1974342 T13.
//
// Four cells here, plus the giver. The giver test is the one that matters most
// for the shape of the bug: `Game::find_all_clues` drops every candidate whose
// simulated interpretation is MISTAKE, so for as long as a legal reactive reads
// MISTAKE it is a clue NO bot can offer -- which is why bot-vs-bot play could
// never surface this and it took a human at the table to give it.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// "Orange (5 Suits)" -- r/y/g/b/o, orange inverted. Colour values are
// Red=1, Yellow=2, Green=3, Blue=4, Orange=5 (colour_value.cpp: the fixed
// colours take 1-4, then Orange takes the first of {2,5,4,3,1} still free).
//
// Orange sits on 0, so o1 -- and only o1 -- is the orange a chuck would stack.
// That is the replay's position: the react slot's `possible` admits o1 while
// its `inferred` does not.
//
// Cathy's leftmost playable is her slot 3 (b2, blue on 1). GREEN has anchor 3,
// and calc_slot(3, 3, 5) = 5, so the pairing is Cathy slot 3 <- Bob slot 5.
// Green touches her g4 on slot 4, which is only there to make the clue legal --
// reactor0's reactive branches never read the touched list.
SetupOptions ladder_opts(std::string bob_react_slot) {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 1, 0};  // blue on 1, orange on 0
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},                      // Alice (us, giver)
      {"y4", "g4", "b4", "r4", std::move(bob_react_slot)}, // Bob (reacter)
      {"r4", "y4", "b2", "g4", "y3"},                      // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

// Narrow the react slot's INFERRED without touching its `possible`, which is
// what `slot_elims` does in a real game: it removes the identities a previous
// reaction ruled out, playables first, and leaves the raw empathy alone.
// Doing it directly keeps this test about the ladder rather than about the
// two-reactive sequence that produces the state (the replay regression
// test_replay_1974342_reactive_discard_survives_a_dead_chuck.cpp covers that).
Game narrow_inferred(Game g, int order, const std::vector<std::string>& ids) {
  IdentitySet keep = IdentitySet::empty();
  for (const std::string& s : ids) {
    keep = keep.add(g.state.expand_short(s));
  }
  g.with_thought(order, [keep](const Thought& t) {
    Thought out = t;
    out.inferred = keep;
    return out;
  });
  return g;
}

}  // namespace

// --- the two arms ---------------------------------------------------------

// Arm 1. The react slot's inferences include an orange the stack is waiting
// for, so pressing Discard stacks it: a CHUCK, and the inference narrows to
// exactly what that button advances.
TEST(Reactor0ReactDiscardButton, ChuckArmFiresOnAPlayableInvertedReading) {
  Game g = setup(ladder_opts("r2"));
  const int react = order_at(g, TestPlayer::BOB, 5);
  // {o1, o3}: with orange on 0 the o1 is the one a chuck would stack.
  g = narrow_inferred(std::move(g), react, {"o1", "o3"});

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 5));
  // The chuck stamp names what the Discard button actually stacks.
  expect_infs(g, std::nullopt, TestPlayer::BOB, 5, {"o1"});
}

// Arm 2, and the bug. `possible` still admits the o1 -- nothing has ruled it
// out of the raw empathy -- but `inferred` does not, so there is no chuck to
// stamp. The ordinary throw-away reading is right here and must be reached:
// r2 is non-critical with both copies still out.
//
// The old gate read `possible`, chose the chuck, got nothing, and had nowhere
// to go: the pairing was abandoned, the play pool ran out, and a plain reactive
// discard came out a MISTAKE.
TEST(Reactor0ReactDiscardButton, PlainArmFiresWhenTheChuckReadingIsDead) {
  Game g = setup(ladder_opts("r2"));
  const int react = order_at(g, TestPlayer::BOB, 5);

  // The precondition, stated as an assertion so the test cannot quietly stop
  // exercising the bug: the raw empathy admits the playable orange.
  ASSERT_TRUE(g.common.thoughts[react].possible.contains(
      g.state.expand_short("o1")))
      << "an unclued slot's `possible` still holds o1 -- this is the set the "
         "old gate read";

  // ...and the inference does not. No inverted reading can be stacked; the
  // plain readings are ordinary non-criticals.
  g = narrow_inferred(std::move(g), react, {"r2", "r3", "y3", "g3"});

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::REACTIVE)
      << "a plain reactive discard, unreadable before v10.9.0";
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().focus_slot, 3) << "green = 3";
  EXPECT_EQ(g.waiting.front().react_order, react)
      << "calc_slot(3, 3, 5) = 5 pairs Cathy's b2 with Bob's slot 5";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::CALLED_TO_DISCARD);
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 5));
}

// The floor. The ladder adds a fallback; it does not make every react slot
// discardable. With every reading a critical plain card and no inverted one,
// both arms refuse and the clue is correctly unread -- `slot_is_chuckable` is
// false, and that is the only thing that should produce a MISTAKE here.
TEST(Reactor0ReactDiscardButton, NeitherArmLeavesTheClueUnread) {
  Game g = setup(ladder_opts("r2"));
  const int react = order_at(g, TestPlayer::BOB, 5);
  g = narrow_inferred(std::move(g), react, {"r5", "y5"});

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "no reading can be chucked and none can be spared";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 5), CardStatus::NONE);
}

// --- the giver ------------------------------------------------------------

// The point of the fix, from the seat that has to offer the clue.
//
// `find_all_clues` simulates each candidate and drops it when the
// interpretation is MISTAKE, so the arm-2 position above was not merely
// misread by the reacter -- it was a clue no bot could ever give. It has to
// come back into the candidate set.
TEST(Reactor0ReactDiscardButton, TheGiverOffersTheClueOnceItReads) {
  Game g = setup(ladder_opts("r2"));
  const int react = order_at(g, TestPlayer::BOB, 5);
  g = narrow_inferred(std::move(g), react, {"r2", "r3", "y3", "g3"});

  // Bob's react slot is an r2 with both copies still in play, so the
  // giver-only critical-discard guard (decide.cpp) has no reason to veto:
  // it drops a clue only when the reacter CTD lands on a card the giver can
  // see is critical.
  ASSERT_FALSE(g.state.is_critical(g.state.expand_short("r2")))
      << "two r2s, neither discarded";

  const auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  const int green = str_to_clue(g.state, "green").value;

  bool offered = false;
  for (const auto& c : clues) {
    if (const auto* col = std::get_if<PerformColour>(&c)) {
      if (col->target == cathy && col->value == green) offered = true;
    }
  }
  EXPECT_TRUE(offered)
      << "green to Cathy is a legal reactive discard; while it read MISTAKE it "
         "was filtered out of every candidate list and no bot could give it";
}

// The mirror, so the guard above is pinned as well as the filter: when the
// pairing would call the reacter to throw a card the giver can see is
// critical, the giver must still decline.
TEST(Reactor0ReactDiscardButton, TheGiverStillDeclinesOnACriticalReactSlot) {
  Game g = setup(ladder_opts("r5"));  // the sole r5 -- critical on sight
  const int react = order_at(g, TestPlayer::BOB, 5);
  g = narrow_inferred(std::move(g), react, {"r2", "r3", "y3", "g3"});

  ASSERT_TRUE(g.state.is_critical(g.state.expand_short("r5")));

  const auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  const int green = str_to_clue(g.state, "green").value;

  for (const auto& c : clues) {
    if (const auto* col = std::get_if<PerformColour>(&c)) {
      EXPECT_FALSE(col->target == cathy && col->value == green)
          << "the reacter would discard the only Red 5 -- the critical-discard "
             "guard must keep this clue out of the candidate set";
    }
  }
}
