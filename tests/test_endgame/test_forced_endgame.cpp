// Unit tests for the forced-endgame hardcoded rules
// (src/endgame/forced_endgame.cpp). Currently pinned: the 5-lockout
// rule, which fires when `cards_left == 1` and the suit's 5 is held by
// a player whose cycle position is at-or-before every 4-holder for the
// same suit (so playing now would empty the deck and lock the 5-holder
// out of their last-turn-after-4-played slot).
//
// Setup notes: we pre-seed `play_stacks` to {0, 0, 3} so the only
// suit that can trigger 5-lockout in the rule's iteration is Orange.
// Red and Blue stay at 0 with their 5s in the deck (not in any hand),
// so `five_holder = nullopt` for those suits and the rule short-
// circuits. We also force `cards_left = 1` directly (the rule only
// inspects the field; we don't need to drive 14 plays/discards to
// exhaust the deck).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/endgame/forced_endgame.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

Game with_cards_left(Game g, int n) {
  g.state.cards_left = n;
  return g;
}

}  // namespace

// 3-player Orange (3 Suits). Stacks (0, 0, 3). CP=Alice (observer).
// Bob (offset 1) holds o5; Cathy (offset 2) holds o4. The 5-holder
// (offset 1) is BEFORE the 4-holder (offset 2): playing now empties
// the deck, Bob's last turn sees o=3 (can't play o5), Cathy's plays
// o4, Alice's closes — Bob never gets another shot at o5. Rule must
// fire.
TEST(ForcedEndgame, FiveLockoutFiresWhenFiveHolderPrecedesFourHolder) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer + CP): explicit non-rank-4/5-orange filler.
      {"r1", "b1", "o1", "r2", "b2"},
      // Bob (offset 1): slot 1 = o5.
      {"o5", "r3", "b3", "r2", "b2"},
      // Cathy (offset 2): slot 1 = o4.
      {"o4", "r3", "b3", "r4", "b4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 3};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));
  g = with_cards_left(std::move(g), 1);

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value())
      << "5-lockout must fire when 5-holder (Bob, offset 1) is before "
         "4-holder (Cathy, offset 2)";
  EXPECT_TRUE(std::holds_alternative<PerformColour>(*forced) ||
              std::holds_alternative<PerformRank>(*forced))
      << "forced action must be a clue, not a play/discard";
}

// Same shape but swap holders: Bob (offset 1) has o4; Cathy (offset 2)
// has o5. Now the 4-holder precedes the 5-holder, so if Alice plays
// now, Bob plays o4 on his last turn (o=4) and Cathy plays o5 on hers.
// Rule must NOT fire.
TEST(ForcedEndgame, FiveLockoutDoesNotFireWhenFourHolderPrecedesFiveHolder) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "b1", "o1", "r2", "b2"},
      // Bob (offset 1): slot 1 = o4.
      {"o4", "r3", "b3", "r2", "b2"},
      // Cathy (offset 2): slot 1 = o5.
      {"o5", "r3", "b3", "r4", "b4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 3};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));
  g = with_cards_left(std::move(g), 1);

  auto forced = hanabi::endgame::forced_endgame_action(g);
  EXPECT_FALSE(forced.has_value())
      << "rule must not fire when some 4-holder strictly precedes the "
         "5-holder (Bob offset 1 < Cathy offset 2)";
}

// Same player holds BOTH o4 and o5. They only get one last turn after
// the deck empties → only one of the two cards can play, losing the
// other. Rule must fire (cluing delays the deck-empty event so the
// holder gets two pre-empty turns to play 4 then 5).
TEST(ForcedEndgame, FiveLockoutSameHandFires) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "b1", "o1", "r2", "b2"},
      // Bob (offset 1) holds both o4 and o5.
      {"o5", "o4", "b3", "r3", "b2"},
      // Cathy filler, no rank-4/5 of orange.
      {"r2", "b3", "r3", "b4", "r4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 3};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));
  g = with_cards_left(std::move(g), 1);

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value())
      << "5-lockout must fire when 5- and 4-holder are the same player "
         "(offsets equal → `>=` predicate holds)";
  EXPECT_TRUE(std::holds_alternative<PerformColour>(*forced) ||
              std::holds_alternative<PerformRank>(*forced));
}

// Sanity guard: rule must not fire when `cards_left > 1`.
TEST(ForcedEndgame, FiveLockoutDoesNotFireWhenCardsLeftAboveOne) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "b1", "o1", "r2", "b2"},
      {"o5", "r3", "b3", "r2", "b2"},
      {"o4", "r3", "b3", "r4", "b4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 3};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));
  g = with_cards_left(std::move(g), 2);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// --- Two-critical play rule ---------------------------------------------
//
// Setup pattern: Alice (CP) holds r4 in slot 1 and b4 in slot 2 with
// `discarded = {"r4", "b4"}` so each remaining 4 is the only copy left
// in the game (`is_critical` true). `fully_known` pre-clues both slots
// so `common.thoughts[order].inferred` is a singleton. All 5s stay in
// the deck so the 5-lockout rule never triggers for these positions
// (its `five_holder` lookup returns nullopt). `play_stacks` controls
// playability: r=3 makes r4 playable; r=0 leaves it unplayable.

namespace {

SetupOptions two_crit_base() {
  SetupOptions opts;
  opts.hands = {
      {"r4", "b4", "o1", "o2", "o3"},
      {"r1", "r2", "b1", "b2", "o4"},
      {"r1", "r3", "b1", "b3", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.discarded = {"r4", "b4"};
  return opts;
}

}  // namespace

// Canonical fire case: two known crits (r4, b4), r4 playable (r stack=3),
// cards_left=1, clue_tokens=2 < num_players=3 → forced PerformPlay on r4.
TEST(ForcedEndgame, TwoCriticalPlayFiresWhenTwoCritsOnePlayable) {
  SetupOptions opts = two_crit_base();
  opts.play_stacks = {3, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r4");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "b4");
  g = with_cards_left(std::move(g), 1);

  int r4_order = g.state.hands[0][0];
  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value())
      << "two-critical play must fire: Alice knows r4 (playable) and b4 "
         "(critical, non-playable); cards_left=1, ct=2 < n=3";
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced))
      << "forced action must be a PerformPlay, not a clue/discard";
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, r4_order)
      << "must target the playable critical card (r4)";
}

// Negative: only one critical known. Alice has r4 fully_known (crit
// playable); b4 in hand but not pre-clued so common.thoughts.inferred is
// not a singleton and the rule doesn't count it. Rule must not fire.
TEST(ForcedEndgame, TwoCriticalPlayDoesNotFireWhenOnlyOneCrit) {
  SetupOptions opts = two_crit_base();
  opts.play_stacks = {3, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r4");
  g = with_cards_left(std::move(g), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "only one known critical in hand — defer to the solver";
}

// Negative: two known critical 4s but stacks are too low for either to
// be playable. The rule needs at least one playable critical.
TEST(ForcedEndgame, TwoCriticalPlayDoesNotFireWhenNoPlayable) {
  SetupOptions opts = two_crit_base();
  opts.play_stacks = {0, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r4");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "b4");
  g = with_cards_left(std::move(g), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "neither known critical is currently playable — rule must not "
         "fire (no productive play exists for the rule to force)";
}

// Negative: clue_tokens == num_players. The team can in principle stall
// by all cluing once each, giving Alice another play opportunity later.
TEST(ForcedEndgame, TwoCriticalPlayDoesNotFireWhenClueTokensAtLeastPlayers) {
  SetupOptions opts = two_crit_base();
  opts.play_stacks = {3, 0, 0};
  opts.clue_tokens = 3;  // == num_players, stalling possible.
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r4");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "b4");
  g = with_cards_left(std::move(g), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "ct >= num_players — team could cycle clues, defer to the solver";
}

// Sanity guard: rule must not fire when `cards_left > 1`.
TEST(ForcedEndgame, TwoCriticalPlayDoesNotFireWhenCardsLeftAboveOne) {
  SetupOptions opts = two_crit_base();
  opts.play_stacks = {3, 0, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "r4");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "b4");
  g = with_cards_left(std::move(g), 2);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// Tiebreaker: when two playable criticals exist (e.g. r4 and b5 both
// playable from CP), play the one whose successor (rank+1) is held by
// another player. Without the tiebreaker, the rule picks the first slot
// in iteration order — b5 in this setup — which would lose the
// successor's play opportunity (here r5, held by Bob, becomes playable
// only after r4 lands; if CP plays b5 first then r5 has no chance).
//
// Setup: r stack=3 (so r4 playable), b stack=4 (so b5 playable). r4 and
// b5 both fully_known in Alice's hand. r5 in Bob's hand (he'll play it
// in his endgame turn iff Alice unblocks it first). Replay 1899527 T47
// reproducer.
TEST(ForcedEndgame, TwoCriticalPlayPrefersRank4WhenSuccessorHeldElsewhere) {
  SetupOptions opts;
  opts.hands = {
      // Alice: slot 1 = b5 (critical, playable on b=4), slot 2 = r4
      // (critical, playable on r=3). Filler avoids accidental clue
      // targets touching these slots.
      {"b5", "r4", "o1", "o2", "o3"},
      // Bob: slot 1 = r5 (the successor of r4; would play in his
      // endgame turn only if r4 lands first).
      {"r5", "r1", "r2", "b1", "b2"},
      // Cathy: filler.
      {"r1", "r3", "b1", "b3", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  // r4 has 2 copies — discard 1 to make Alice's copy critical. b5 has
  // 1 copy (rank-5), so Alice's b5 is naturally critical.
  opts.discarded = {"r4"};
  opts.play_stacks = {3, 4, 0};
  opts.clue_tokens = 2;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, 1, "b5");
  g = fully_known(std::move(g), TestPlayer::ALICE, 2, "r4");
  g = with_cards_left(std::move(g), 1);

  int b5_order = g.state.hands[0][0];
  int r4_order = g.state.hands[0][1];

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value()) << "two-critical play must fire";
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced));
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, r4_order)
      << "must play r4 (the rank-4 critical with a successor r5 held by "
         "Bob), not b5 (rank-5, no successor to unblock). Playing b5 "
         "first loses r5 permanently — Bob can't play r5 on his endgame "
         "turn because r stack is still 3.";
  EXPECT_NE(std::get<PerformPlay>(*forced).target, b5_order);
}
