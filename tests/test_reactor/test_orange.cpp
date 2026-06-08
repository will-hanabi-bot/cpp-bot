// Behavioral tests for the Orange / Dark Orange suit inversion. Cards on an
// inverted suit flip play/discard at the game-rule layer (discard advances
// the play stack, play sends to the discard pile + regains a clue), and
// the reactor convention compensates by inverting the reacter's physical
// action whenever the reactive target is on the inverted suit — so the
// receiver's standard reading of (clue kind + reacter action) leaves them
// pointed at the *physical* action that the orange game rule needs.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

bool any_status(const Game& g, TestPlayer player, CardStatus s) {
  int pi = static_cast<int>(player);
  for (int o : g.state.hands[pi]) {
    if (g.meta[o].status == s) return true;
  }
  return false;
}

}  // namespace

// Example 1: Cathy has [r3, o1, r1, ?, ?] with stacks empty. Alice clues red
// touching Cathy's slots 1 (r3) and 3 (r1) — focus_slot = 3, the leftmost
// playable target is o1 at slot 2. Without the inversion the reactor would
// mark Bob CALLED_TO_DISCARD (color + play_target → reacter discards). For
// an orange target the giver-side swap puts CALLED_TO_PLAY on Bob's slot 1
// instead: the receiver's standard reading then becomes target_i_discard
// on Cathy's o1, whose physical discard advances the orange stack.
TEST(Orange, ColourClueOrangePlayTargetSwapsReacterToPlay) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer + giver): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob (reacter): slot 1 = r1 (playable on red stack), so the
      // inverted-target swap to target_play succeeds against possible-playable
      // inference. The remaining slots are non-playable trash fillers that
      // don't collide with Cathy's deck copies.
      {"r1", "r4", "o3", "b3", "b2"},
      // Cathy (receiver): r3 / o1 / r1 / fillers per the user's example.
      {"r3", "o1", "r1", "b4", "b4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues red to Cathy");

  // Bob's react_slot is calc_slot(focus_slot=3, target_slot=2, hand_size=5)
  // = 1, i.e., Bob's slot 1. With the orange-target swap it should be
  // CALLED_TO_PLAY (not CALLED_TO_DISCARD as it would be on a non-inverted
  // target).
  int bob_slot_1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  EXPECT_EQ(g.meta[bob_slot_1].status, CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_DISCARD));
}

// Example 2: Cathy has [o2, r3, y4, g5, b2] with stacks empty. Alice clues
// rank-3 to Cathy, touching only slot 2 — focus_slot = 2. No direct
// play_target exists (no slot is currently playable), so the convention
// falls into the finesse_targets path. The first iteration (react_slot=1
// → target_slot=1) lines up Bob's slot 1 = o1 (the prerequisite) with
// Cathy's slot 1 = o2 (the finessed target). Standard rank-finesse would
// call target_play on Bob; with the orange swap (target is o2) it calls
// target_discard instead, so Bob's slot 1 is CALLED_TO_DISCARD. take_action
// on Bob's turn then issues PerformDiscard, which the game-rule inversion
// turns into "play o1 onto the orange stack" — the user-described chain.
TEST(Orange, RankFinesseOrangeChainMarksBobDiscard) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer + giver): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob (reacter): slot 1 = o1 (the finessed prerequisite). Other slots
      // are non-playable fillers.
      {"o1", "b4", "y4", "g4", "r4"},
      // Cathy (receiver): exact user-example hand.
      {"o2", "r3", "y4", "g5", "b2"},
  };
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  int bob_slot_1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  EXPECT_EQ(g.meta[bob_slot_1].status, CardStatus::CALLED_TO_DISCARD)
      << "Orange finesse target should flip Bob's slot 1 to CTD so his "
         "physical discard advances the orange stack via the inversion";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}

// Layer-1 isolation: with Orange in the suit list, on_play physically
// discards the orange card (and regains a clue), while on_discard with a
// playable orange id physically advances the orange play stack (no clue
// regain). Misplaying (discarding an unplayable orange) strikes.
TEST(Orange, PlayOrangeSendsToDiscardPileAndRegainsClue) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"o1", "b4", "y4", "g4", "r4"},
      {"o2", "r3", "y4", "g5", "b2"},
  };
  opts.variant_name = "Orange (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 4;  // leave headroom to observe a clue-regain.
  Game g = setup(std::move(opts));

  int bob_o1 = g.state.hands[1][0];  // Bob slot 1 = o1.
  int orange_suit = 4;  // 5-suit variant: suits[4] = Orange.
  ASSERT_TRUE(g.state.variant->suits[orange_suit].suit_type.inverted);

  // Physical PLAY of an orange card → goes to discard pile, regains clue,
  // play stack untouched.
  Game g_play = g;
  g_play.handle_action(PlayAction{1, bob_o1, orange_suit, 1});
  EXPECT_EQ(g_play.state.play_stacks[orange_suit], 0);
  EXPECT_EQ(g_play.state.clue_tokens, 5);
  EXPECT_EQ(g_play.state.strikes, 0);
  EXPECT_FALSE(g_play.state.discard_stacks[orange_suit][0].empty());

  // Physical DISCARD of a playable orange card → advances orange stack
  // by one, no clue regain, no strike.
  Game g_discard = g;
  g_discard.handle_action(
      DiscardAction{1, bob_o1, orange_suit, 1, /*failed=*/false});
  EXPECT_EQ(g_discard.state.play_stacks[orange_suit], 1);
  EXPECT_EQ(g_discard.state.clue_tokens, 4);
  EXPECT_EQ(g_discard.state.strikes, 0);

  // Physical DISCARD of an unplayable orange card (orange stack still 0,
  // trying to advance with o2) → strike, no advance, no clue regain.
  int cathy_o2 = g.state.hands[2][0];  // Cathy slot 1 = o2.
  Game g_misplay = g;
  g_misplay.handle_action(
      DiscardAction{2, cathy_o2, orange_suit, 2, /*failed=*/true});
  EXPECT_EQ(g_misplay.state.play_stacks[orange_suit], 0);
  EXPECT_EQ(g_misplay.state.clue_tokens, 4);
  EXPECT_EQ(g_misplay.state.strikes, 1);
  EXPECT_FALSE(g_misplay.state.discard_stacks[orange_suit][1].empty());
}

// Case 9: empathy-fill of a playable orange (no convention mark) → bot
// dispatches PerformDiscard so the orange game-rule turns the physical
// discard into a play onto the orange stack. The bot detects this case
// through the empathy-aware `all_plays` construction (orange known-id →
// DiscardAction + PerformDiscard) rather than through any urgent CTP
// path — CTP/CTD on a card are PHYSICAL labels, so an urgent CTP on an
// orange card would actually mean "PerformPlay, dump to pile" which is
// case 10's behaviour, not case 9.
TEST(Orange, EmpathyFilledPlayableOrangeDispatchesPerformDiscard) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"o3", "r4", "r2", "b3", "b2"},  // Bob slot 1 = o3 (playable on o-stack=2).
      {"r3", "b4", "r2", "r3", "b3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 2};  // orange stack at 2 so o3 is playable.
  Game g = setup(std::move(opts));

  int bob_o3 = g.state.hands[1][0];

  // Pin Bob's empathy-view of his slot 1 to {(O,3)} — both his player
  // view and common, since the convention layer would set both via
  // elim. No CTP / urgent mark here: we're testing the empathy-fill
  // path through `all_plays`, not the convention-mark path.
  IdentitySet locked = IdentitySet::single(Identity{2, 3});
  auto pin = [&](Player& p) {
    p = p.with_thought(bob_o3, [&](const Thought& t) {
      Thought out = t;
      out.inferred = locked;
      out.possible = locked;
      out.info_lock = locked;
      return out;
    });
  };
  pin(g.common);
  pin(g.players[1]);
  g.state.deck[bob_o3].clued = true;

  // Switch POV to Bob and run take_action.
  g.state.our_player_index = 1;
  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformDiscard>(perform))
      << "Empathy-filled playable orange must dispatch as PerformDiscard "
         "so the orange game-rule plays it onto the stack.";
  EXPECT_EQ(std::get<PerformDiscard>(perform).target, bob_o3);
}

// Case 3: stable rank clue where the only non-trash matching id is orange.
// playable_rank fires; with the orange-focus swap the convention marks the
// orange focus CTD (PHYSICAL), so urgent_action dispatches PerformDiscard,
// which the orange game-rule turns into "advance the orange stack".
TEST(Orange, Case3_StableRankOnlyPlayableIsOrangeMarksFocusCTD) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob slot 1 = o3. The other slots are non-playable, non-rank-3 cards
      // so the rank-3 clue only touches o3.
      {"o3", "r1", "b2", "r2", "b1"},
      // Cathy filler — chosen so deck counts stay within card_count after
      // the play_stacks pre-seed of {3, 3, 2}.
      {"b3", "r3", "r1", "b1", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {3, 3, 2};  // R3 / B3 trash; O3 is the unique playable.
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 3 to Bob");

  int bob_o3 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  EXPECT_EQ(g.meta[bob_o3].status, CardStatus::CALLED_TO_DISCARD)
      << "Orange focus in playable_rank should be marked CTD physical so "
         "PerformDiscard advances the orange stack via inversion";
}

// Case 4: COLOR + non-orange playable receiver target + orange playable
// reacter card. Standard COLOR play_target is target_discard(reacter);
// the would_lose_inverted_reacter gate lets the orange playable through
// (target_discard on a *playable* orange = PerformDiscard = advance). Bob's
// orange card ends up CTD; on his turn PerformDiscard advances orange and
// the receiver's r1 gets CTP'd via react_discard + COLOR → target_i_play.
TEST(Orange, Case4_ColourPlayTargetWithOrangePlayableReacterMarksCTD) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob hand: only non-red fillers + o1 at slot 5 so the red clue
      // doesn't touch Bob's hand at all. Bob's o1 is playable on orange
      // stack = 0; we want it CTD via the reactive's reacter mapping.
      {"b4", "b3", "b2", "b1", "o1"},
      // Cathy: only one red card (r1 at slot 1) so the focus lands on
      // slot 1, focus_slot = 1, and calc_slot(1, target=1, 5) = 5 →
      // Bob's slot 5 = o1.
      {"r1", "b4", "b3", "b2", "b1"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues red to Cathy");

  int bob_o1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][4];
  EXPECT_EQ(g.meta[bob_o1].status, CardStatus::CALLED_TO_DISCARD)
      << "Standard COLOR + non-orange play_target marks reacter CTD; with "
         "orange playable reacter card the gate keeps it, and PerformDiscard "
         "advances the orange stack via inversion.";
}

// Case 10: empathy-fill of a trash orange. No convention mark — the bot's
// own heuristic ("this card is trash") routes through all_discards, and
// the suit-aware construction emits PerformPlay so the orange game-rule
// sends it to the discard pile (regaining a clue).
TEST(Orange, Case10_EmpathyFilledTrashOrangeDispatchesPerformPlay) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob slot 1 = o1, which is *trash* once the orange stack reaches 1.
      {"o1", "r3", "b3", "r4", "b4"},
      {"r1", "b1", "r2", "b2", "o4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  // Pre-advance the orange stack to 1 so o1 is basic trash.
  opts.play_stacks = {0, 0, 1};
  // Force the bot to choose between play and discard candidates — at 8
  // clue tokens a clue would be preferred over either, drowning out the
  // case-10 signal we're trying to verify.
  opts.clue_tokens = 0;
  Game g = setup(std::move(opts));

  int bob_o1 = g.state.hands[1][0];

  // Pin Bob's empathy of slot 1 to {(O,1)}.
  IdentitySet locked = IdentitySet::single(Identity{2, 1});
  auto pin = [&](Player& p) {
    p = p.with_thought(bob_o1, [&](const Thought& t) {
      Thought out = t;
      out.inferred = locked;
      out.possible = locked;
      out.info_lock = locked;
      return out;
    });
  };
  pin(g.common);
  pin(g.players[1]);
  g.state.deck[bob_o1].clued = true;
  // Pre-fill any unrelated hand slot as a known non-trash card so Bob
  // actually has a "play vs discard" choice (otherwise lock-discard
  // logic activates and the test loses meaning).
  // We can leave it — Bob's other slots are unclued, so the bot's
  // discard heuristic picks the known trash orange and dispatches it
  // through all_discards.

  g.state.our_player_index = 1;
  PerformAction perform = g.take_action();
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(perform))
      << "Empathy-known trash orange should dispatch through all_discards "
         "as PerformPlay so the orange game-rule sends it to the discard "
         "pile (regaining a clue), not PerformDiscard (which would attempt "
         "to advance the stack and misplay since o1 is trash now).";
  EXPECT_EQ(std::get<PerformPlay>(perform).target, bob_o1);
}
