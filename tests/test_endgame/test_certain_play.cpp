// A card can be CERTAIN to play without anyone knowing what it is.
//
// The endgame had two notions of "known playable" and both miss that case:
//
//   * `Player::obvious_playables` is clue-derived, so a card known playable
//     only by inference is invisible to it. `trivially_winnable` walks it.
//   * `Thought::id(infer=true)` needs a PINNED singleton. `solve`'s "one play
//     wins" shortcut uses it.
//
// A card read as {a5, d5} with both stacks on 4 scores whichever it is, and
// neither notion can see that. Replay 1969779 T68 is what it cost: on the final
// turn at 28/30 will-bot67 held exactly that card and gambled a different one
// instead, which was trash.
//
// `certainly_advances` asks the question directly, and asks it about the
// BUTTON: Play advances a plain card and PITCHES an inverted one, Discard the
// reverse.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/forced_endgame.h"
#include "hanabi/endgame/helper.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using hanabi::endgame::certain_plays;
using hanabi::endgame::certainly_advances;

namespace {

// Red and blue both on 4, so r5 and b5 are each playable. Our own hand is face
// down -- `possibilities()` is what the holder knows, and the harness would
// otherwise hand us our own identities.
SetupOptions two_fives_position() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 0, 0, 4, 0};
  opts.clue_tokens = 4;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (us)
      {"y1", "g1", "p1", "y2", "g2"},
      {"y3", "g3", "p2", "y4", "g4"},
  };
  return opts;
}

int mine(const Game& g, int slot) { return g.state.hands[0][slot - 1]; }

}  // namespace

// --- the case both old notions miss ---------------------------------------

TEST(EndgameCertainPlay, EveryReadingPlayableWithoutAPinnedIdentity) {
  Game g = setup(two_fives_position());
  // Our slot 1 is clued as a 5. Red and blue are both on 4, so the reading is
  // {r5, y5, g5, b5, p5} narrowed by the stacks... which is not enough on its
  // own -- pin it to the two fives that are actually playable.
  const int o = mine(g, 1);
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{0, 5}).add(Identity{3, 5});
    return out;
  });
  g.players[0] = g.common;

  ASSERT_FALSE(g.me().thoughts[o].id(/*infer=*/true).has_value())
      << "guard: not pinned to one identity, so `id(infer=true)` cannot see it";

  EXPECT_TRUE(certainly_advances(g, o, PerformAction{PerformPlay{o}}))
      << "both readings are playable, so pressing Play scores either way";
  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformDiscard{o}}))
      << "these are plain suits; Discard throws them away";
}

TEST(EndgameCertainPlay, OneTrashReadingIsEnoughToMakeItAGamble) {
  Game g = setup(two_fives_position());
  const int o = mine(g, 1);
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    // r5 and b5 play; r1 is long gone.
    out.inferred =
        IdentitySet{}.add(Identity{0, 5}).add(Identity{3, 5}).add(Identity{0, 1});
    return out;
  });
  g.players[0] = g.common;

  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformPlay{o}}))
      << "one reading that would strike is enough";
}

// --- the button is part of the question -----------------------------------

namespace {

// "Orange (3 Suits)" -- r / b / o, with orange inverted. Orange on 2, so o3 is
// the card its stack is waiting for; on an inverted suit the button that
// advances it is DISCARD.
SetupOptions orange_position() {
  SetupOptions opts;
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 2};
  opts.clue_tokens = 4;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "b1", "r2", "b2", "r3"},
      {"b3", "r4", "b4", "o1", "o2"},
  };
  return opts;
}

}  // namespace

TEST(EndgameCertainPlay, AnInvertedCardIsCertainViaDiscardNotPlay) {
  Game g = setup(orange_position());
  const int o = mine(g, 1);
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{2, 3});  // o3, the next orange
    return out;
  });
  g.players[0] = g.common;

  EXPECT_TRUE(certainly_advances(g, o, PerformAction{PerformDiscard{o}}))
      << "Discard is the button that advances an inverted stack";
  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformPlay{o}}))
      << "pressing Play on an orange card PITCHES it into the discard pile";
}

// Readings that span a plain and an inverted suit are never certain, because
// the two halves need opposite buttons -- even though every identity in the set
// is playable.
TEST(EndgameCertainPlay, ASetSpanningBothKindsIsNeverCertain) {
  Game g = setup(orange_position());
  const int o = mine(g, 1);
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{0, 5}).add(Identity{2, 3});
    return out;
  });
  g.players[0] = g.common;

  EXPECT_TRUE(g.state.is_playable(Identity{0, 5})) << "guard: r5 plays";
  EXPECT_TRUE(g.state.is_playable(Identity{2, 3})) << "guard: o3 plays";
  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformPlay{o}}));
  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformDiscard{o}}))
      << "no single button covers both halves";
}

// A chuck cannot be performed at 8 tokens, so it is not a certain play there.
TEST(EndgameCertainPlay, AChuckIsNotCertainAtEightTokens) {
  SetupOptions opts = orange_position();
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  const int o = mine(g, 1);
  g.with_thought(o, [&](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{2, 3});
    return out;
  });
  g.players[0] = g.common;

  EXPECT_FALSE(certainly_advances(g, o, PerformAction{PerformDiscard{o}}))
      << "discarding is illegal at 8 tokens, so the chuck is unavailable";
}

// --- the collection -------------------------------------------------------

TEST(EndgameCertainPlay, CertainPlaysListsThemInHandOrder) {
  Game g = setup(two_fives_position());
  const int s1 = mine(g, 1);
  const int s3 = mine(g, 3);
  for (int o : {s1, s3}) {
    g.with_thought(o, [&](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet{}.add(Identity{0, 5}).add(Identity{3, 5});
      return out;
    });
  }
  g.players[0] = g.common;

  auto certain = certain_plays(g);
  ASSERT_EQ(certain.size(), 2u);
  EXPECT_EQ(std::get<PerformPlay>(certain[0]).target, s1) << "hand order";
  EXPECT_EQ(std::get<PerformPlay>(certain[1]).target, s3);
}

// --- forced endgame: a certain play with the deck empty --------------------

// Rule 0 of `forced_endgame_action`. The `cards_left == 0` gate is
// LOAD-BEARING: with one card still in the deck a guaranteed point is sometimes
// worth less than a stall, and the solver is trusted to see it -- replay
// 1874799 must stall rather than play its null-5, 1875304's winning line is a
// stall clue an urgent play must not shortcut, and 1885855 exists so the
// 5-lockout blocks an r5 play. Making the rule unconditional breaks all three.
TEST(EndgameCertainPlay, ForcedOnlyOnceTheDeckIsEmpty) {
  SetupOptions opts = two_fives_position();
  // Four points missing would leave the fork shut; two suits short of max keeps
  // it open, matching a real endgame.
  opts.play_stacks = {4, 5, 5, 4, 5};
  Game g = setup(std::move(opts));
  const int o = mine(g, 1);
  g.with_thought(o, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet{}.add(Identity{0, 5}).add(Identity{3, 5});
    return out;
  });
  g.players[0] = g.common;
  ASSERT_FALSE(certain_plays(g).empty()) << "guard: slot 1 scores either way";

  // With cards still in the deck the rule stands down and the later layers get
  // their turn.
  g.with_state([](State& s) { s.cards_left = 1; });
  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value() &&
               std::holds_alternative<PerformPlay>(
                   *hanabi::endgame::forced_endgame_action(g)) &&
               std::get<PerformPlay>(*hanabi::endgame::forced_endgame_action(g))
                       .target == o)
      << "at cards_left == 1 a stall may still be worth more than the point";

  // Empty deck: nothing to draw, nothing to wait for, so it is forced.
  g.with_state([](State& s) { s.cards_left = 0; });
  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced));
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, o);
}

// And with nothing certain in hand it declines, leaving the later rules alone.
TEST(EndgameCertainPlay, ForcedRuleDeclinesWithNoCertainPlay) {
  SetupOptions opts = two_fives_position();
  opts.play_stacks = {4, 5, 5, 4, 5};
  Game g = setup(std::move(opts));
  g.with_state([](State& s) { s.cards_left = 0; });
  ASSERT_TRUE(certain_plays(g).empty()) << "guard: nothing is pinned";
  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}
