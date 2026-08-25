// Rule 0b of `forced_endgame_action`: the play that HAS to happen, when nothing
// in hand is certain.
//
// With the deck empty every other hand is visible, so "would playing this
// actually gain anything" is answerable without a search:
// `best_reachable_plays` prices the rest of the final round twice, once as-is
// and once per currently-playable identity. An identity that lifts the ceiling
// is REQUIRED -- nobody else is going to cash it in time.
//
// Selection is the convention: among our cards that could be a required
// identity, the leftmost CLUED one, else the leftmost of any. Replay 1970943
// T24 is why clued-first is load-bearing -- the leftmost candidate overall was
// an omni 1 and only the clued one was the r4.
#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/forced_endgame.h"
#include "hanabi/endgame/helper.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using hanabi::endgame::best_reachable_plays;
using hanabi::endgame::forced_endgame_action;

namespace {

// The 1970943 T24 shape, rebuilt small. Red on 3, everything else finished, so
// r4 is the only playable identity.
//
//   * Bob's hand is entirely trash -- they cannot contribute.
//   * Cathy holds r4 AND r5 but will get one turn, so whichever they lay, the
//     other dies. Our r4 is the only way both score.
SetupOptions blocked_red_position() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {3, 5, 5, 5, 5};
  opts.clue_tokens = 4;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us)
      {"y1", "g1", "b1", "p1", "y2"},  // Bob -- all below their stacks, trash
      {"r4", "r5", "g2", "b2", "p2"},  // Cathy -- holds both red cards
  };
  return opts;
}

int mine(const Game& g, int slot) { return g.state.hands[0][slot - 1]; }

// Give `order` a reading and optionally mark it clued.
void read_as(Game& g, int order, IdentitySet reading, bool clued) {
  g.with_thought(order, [&](const Thought& t) {
    Thought out = t;
    out.inferred = reading;
    return out;
  });
  if (clued) g.with_state([order](State& s) { s.deck[order].clued = true; });
  g.players[0] = g.common;
}

// Narrow EVERY slot to plain trash. Without this the harness leaves our unset
// slots reading the full identity space, which includes the required card --
// so every slot is a candidate and the leftmost is whichever we did not touch.
// r1 is trash in both positions below (red is past 1 in each).
void blank_hand(Game& g) {
  for (int o : g.state.our_hand()) {
    g.with_thought(o, [](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet{}.add(Identity{0, 1});
      return out;
    });
  }
  g.players[0] = g.common;
}

// The deck is empty and the final round is counting down.
void empty_deck(Game& g, int turns) {
  g.with_state([turns](State& s) {
    s.cards_left = 0;
    s.endgame_turns = turns;
  });
}

const Identity kR4{0, 4};
const Identity kR5{0, 5};
const Identity kY1{1, 1};

}  // namespace

// --- the ceiling helper ---------------------------------------------------

TEST(EndgameRequiredPlay, ReachableCountsWhatTheSeatsCanStillLay) {
  Game g = setup(blocked_red_position());
  const State& s = g.state;

  // Cathy alone, red on 3: they can lay the r4, and that is all -- one seat,
  // one turn, one play.
  EXPECT_EQ(best_reachable_plays(s, s.play_stacks, {2}), 1);

  // With r4 already on the stack, Cathy lays the r5 instead. Still one.
  std::vector<int> after = s.play_stacks;
  after[0] = 4;
  EXPECT_EQ(best_reachable_plays(s, after, {2}), 1);

  // Bob is void, so adding their turn changes nothing.
  EXPECT_EQ(best_reachable_plays(s, s.play_stacks, {1, 2}), 1);

  // Two Cathy turns would cash both.
  EXPECT_EQ(best_reachable_plays(s, s.play_stacks, {2, 2}), 2);
}

TEST(EndgameRequiredPlay, OurOwnHiddenCardsAreNeverCredited) {
  Game g = setup(blocked_red_position());
  const State& s = g.state;
  // Our hand is face down, so the helper cannot price it -- that is the whole
  // reason the rule has to gamble rather than compute.
  EXPECT_EQ(best_reachable_plays(s, s.play_stacks, {0}), 0);
}

// --- the rule -------------------------------------------------------------

TEST(EndgameRequiredPlay, PrefersTheLeftmostCluedCandidate) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  // Slot 1 is unclued and could be the r4; slot 3 is clued and could be too.
  // Leftmost-of-any would take slot 1. The convention takes slot 3.
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kY1), /*clued=*/false);
  read_as(g, mine(g, 3), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  empty_deck(g, 3);

  ASSERT_TRUE(hanabi::endgame::certain_plays(g).empty())
      << "guard: neither reading is certain, so Rule 0 stands down";

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced));
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, mine(g, 3));
}

TEST(EndgameRequiredPlay, FallsBackToTheLeftmostWhenNothingIsClued) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 2), IdentitySet{}.add(kR4).add(kY1), /*clued=*/false);
  read_as(g, mine(g, 4), IdentitySet{}.add(kR4).add(kY1), /*clued=*/false);
  empty_deck(g, 3);

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, mine(g, 2)) << "leftmost";
}

TEST(EndgameRequiredPlay, LeftmostWinsAmongTwoCluedCandidates) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 2), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  read_as(g, mine(g, 5), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  empty_deck(g, 3);

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, mine(g, 2));
}

// --- the stand-downs ------------------------------------------------------

TEST(EndgameRequiredPlay, ACertainPlayOutranksAGamble) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4), /*clued=*/true);   // certain
  read_as(g, mine(g, 3), IdentitySet{}.add(kR4).add(kY1), true);    // a gamble
  empty_deck(g, 3);

  ASSERT_FALSE(hanabi::endgame::certain_plays(g).empty())
      << "guard: slot 1 scores on every reading";

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, mine(g, 1))
      << "Rule 0 answers first; the gamble never gets asked";
}

// If someone else will cash the card in time, our playing it gains nothing --
// the ceiling is the same either way, so there is nothing to be forced about.
TEST(EndgameRequiredPlay, StandsDownWhenTheCeilingDoesNotImprove) {
  SetupOptions opts = blocked_red_position();
  // Cathy holds the r4 but NOT the r5 -- so they lay the r4 themselves and the
  // suit is finished either way.
  opts.hands[2] = {"r4", "g2", "b2", "p2", "y3"};
  Game g = setup(std::move(opts));
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  empty_deck(g, 3);

  EXPECT_FALSE(forced_endgame_action(g).has_value())
      << "the r4 gets laid whether or not we gamble on it";
}

TEST(EndgameRequiredPlay, StandsDownWhenNoCardCouldBeTheRequiredOne) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kY1), /*clued=*/true);
  empty_deck(g, 3);
  EXPECT_FALSE(forced_endgame_action(g).has_value());
}

// The `cards_left == 0` gate, which v8.5.0 established as load-bearing for
// Rule 0 and which this rule shares: with a card still in the deck there is a
// future turn the gamble could be traded for, and the solver is trusted to see
// it.
// v8.7.0 asserted the opposite here -- that the gamble stands down with a card
// still in the deck. v9.2.0's rule 0c deliberately reverses it for the ONE-card
// case: replay 1972670 T25 showed a position where the other copy of the needed
// card was held by a seat who could not cash it, so ours was the only one that
// could be played, and waiting scored nothing.
//
// What makes it safe to assert one card early is the narrower candidate test:
// the card must be CLUED and everything it could be that is not already trash
// must be the single required identity. Here {r4, y1} loses the y1 to the
// stacks, leaving exactly {r4}.
TEST(EndgameRequiredPlay, FiresWithOneCardLeftOnACardWeCanNearlyName) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  g.with_state([](State& s) {
    s.cards_left = 1;
    s.endgame_turns = std::nullopt;
  });

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced));
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, mine(g, 1));
}

// The narrow test is the whole guard, so the unclued form of the same card must
// still stand down -- that is 1972670's slot 1, which had an identical reading
// to the slot that was played and was not a candidate.
TEST(EndgameRequiredPlay, OneCardLeftDeclinesOnAnUncluedCard) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kY1), /*clued=*/false);
  g.with_state([](State& s) {
    s.cards_left = 1;
    s.endgame_turns = std::nullopt;
  });

  auto forced = forced_endgame_action(g);
  const bool gambled = forced && std::holds_alternative<PerformPlay>(*forced) &&
                       std::get<PerformPlay>(*forced).target == mine(g, 1);
  EXPECT_FALSE(gambled) << "rule 0c only bets on a card it can nearly name";
}

// Two live readings is not "nearly named" either. With red on 3 the only
// non-trash identities in this position are r4 and r5, so a card reading
// {r4, r5} keeps both -- and rule 0c declines, because playing it might be the
// r5, which does nothing yet.
TEST(EndgameRequiredPlay, OneCardLeftDeclinesOnTwoLiveReadings) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kR5), /*clued=*/true);
  g.with_state([](State& s) {
    s.cards_left = 1;
    s.endgame_turns = std::nullopt;
  });

  auto forced = forced_endgame_action(g);
  const bool gambled = forced && std::holds_alternative<PerformPlay>(*forced) &&
                       std::get<PerformPlay>(*forced).target == mine(g, 1);
  EXPECT_FALSE(gambled);
}

// And the gate still holds further out: with two cards left the round has not
// been triggered by anything we can do this turn, so there is no window to
// price and the rule says nothing.
TEST(EndgameRequiredPlay, StandsDownWithTwoCardsLeft) {
  Game g = setup(blocked_red_position());
  blank_hand(g);
  read_as(g, mine(g, 1), IdentitySet{}.add(kR4).add(kY1), /*clued=*/true);
  g.with_state([](State& s) {
    s.cards_left = 2;
    s.endgame_turns = std::nullopt;
  });

  auto forced = forced_endgame_action(g);
  const bool gambled = forced && std::holds_alternative<PerformPlay>(*forced) &&
                       std::get<PerformPlay>(*forced).target == mine(g, 1);
  EXPECT_FALSE(gambled);
}

// --- inverted suits -------------------------------------------------------

namespace {

// "Orange (3 Suits)" -- r / b / o with orange INVERTED, so the button that
// advances the orange stack is Discard. Orange on 2, so o3 is what it wants.
SetupOptions blocked_orange_position() {
  SetupOptions opts;
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {5, 5, 2};
  opts.clue_tokens = 4;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "b1", "r2", "b2", "r3"},   // Bob -- trash
      {"o3", "o4", "r4", "b3", "b4"},   // Cathy -- holds o3 and o4
  };
  return opts;
}

}  // namespace

TEST(EndgameRequiredPlay, AnInvertedTargetIsChuckedNotPlayed) {
  Game g = setup(blocked_orange_position());
  blank_hand(g);
  read_as(g, mine(g, 2), IdentitySet{}.add(Identity{2, 3}).add(Identity{0, 1}),
          /*clued=*/true);
  empty_deck(g, 3);

  auto forced = forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  EXPECT_TRUE(std::holds_alternative<PerformDiscard>(*forced))
      << "Discard is the button that advances an inverted stack; pressing Play "
         "would pitch the card into the discard pile";
  EXPECT_EQ(std::get<PerformDiscard>(*forced).target, mine(g, 2));
}

TEST(EndgameRequiredPlay, AnInvertedTargetIsSkippedAtEightTokens) {
  SetupOptions opts = blocked_orange_position();
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  blank_hand(g);
  read_as(g, mine(g, 2), IdentitySet{}.add(Identity{2, 3}).add(Identity{0, 1}),
          /*clued=*/true);
  empty_deck(g, 3);

  EXPECT_FALSE(forced_endgame_action(g).has_value())
      << "discarding is illegal at 8 tokens, so the chuck is unavailable";
}
