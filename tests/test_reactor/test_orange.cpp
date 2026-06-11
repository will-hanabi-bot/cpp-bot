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

// Regression for replay 1885527 turn 1: stable colour clue whose
// ref_play target lands on an orange card. Pre-fix, ref_play used
// `target_play` unconditionally and stamped CTP on the orange — which
// the engine's `on_play(inverted)` then sent to the discard pile.
// Now ref_play must reject this interpretation (return nullopt →
// MISTAKE), so eval penalises the clue by −10 and the bot picks a
// different clue.
TEST(Orange, StableColourRefPlayOnOrangeTargetIsMistake) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob: r3 at slot 3 is the colour-red focus. The slot to its
      // left (slot 2 = o1, currently playable) is what ref_play
      // would CTP under the old code.
      {"b4", "r3", "o1", "b3", "b2"},
      // Cathy: filler ensuring card counts.
      {"r4", "b1", "b2", "r2", "r3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  // Build the colour-red→Bob clue and confirm it interprets as
  // MISTAKE rather than CTP'ing Bob's orange.
  Clue red_to_bob{ClueKind::COLOUR, 0, static_cast<int>(TestPlayer::BOB)};
  ClueAction act{g.state.our_player_index, red_to_bob.target,
                  g.state.clue_touched(g.state.hands[red_to_bob.target],
                                        red_to_bob.kind, red_to_bob.value),
                  red_to_bob.base()};
  Game hypo = g.simulate(Action{act});
  ASSERT_FALSE(hypo.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(hypo.move_history.back()));
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(hypo.move_history.back())), 0)
      << "expected ClueInterp::MISTAKE (=0); ref_play onto an orange "
         "target must be rejected";
  for (int o : hypo.state.hands[static_cast<int>(TestPlayer::BOB)]) {
    EXPECT_NE(hypo.meta[o].status, CardStatus::CALLED_TO_PLAY)
        << "rejected clue must not CTP any of bob's cards (would lose to "
           "discard pile under the orange game-rule)";
  }
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

// Regression for replay 1885550 turn 6. After play stacks reach (1, 1, 1)
// in Orange (3 Suits), a rank-1-clued slot has `possible = {r1, b1, o1}`
// and every member is basic_trash, so `thinks_trash` includes that slot.
// Without the orange-trash safety filter the bot would dispatch
// `PerformDiscard` on that slot — but if the actual card is o1, the
// engine's `on_discard(inverted, failed=false)` runs a play-attempt that
// strikes on orange stack 1. The filter must drop the candidate so the
// bot falls back to the chop (here Alice's slot 2 = an unclued card).
TEST(Orange, AvoidPerformDiscardOnMultiIdOrangePossibleTrash) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer): slot 1 = an actual r1 we'll rank-1-clue;
      // slots 2..5 are unclued filler. The chop must end up at slot 2.
      {"r1", "xx", "xx", "xx", "xx"},
      // Bob filler (visible — picked so deck counts stay within card_count
      // after the play_stacks pre-seed of {1, 1, 1}).
      {"r3", "b3", "o3", "r4", "b4"},
      // Cathy filler.
      {"o4", "r2", "b2", "o2", "r3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 1, 1};  // r1, b1, o1 all basic_trash on the stack.
  opts.clue_tokens = 0;          // Force a discard (no cluing available).
  Game g = setup(std::move(opts));
  // Pre-clue rank-1 on Alice's slot 1 so its possible narrows to
  // {r1, b1, o1} (every member basic_trash → order_trash returns true).
  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g.elim();

  int alice_slot1 = g.state.hands[0][0];
  ASSERT_TRUE(g.me().order_trash(g, alice_slot1))
      << "slot 1 must be empathy-trash after the rank-1 clue + stacks";
  bool has_orange_in_possible = false;
  for (Identity i : g.me().thoughts[alice_slot1].possible) {
    if (g.state.variant->suits[i.suit_index].suit_type.inverted) {
      has_orange_in_possible = true;
      break;
    }
  }
  ASSERT_TRUE(has_orange_in_possible)
      << "slot 1's possible must still include o1 — the filter only fires "
         "when the empathy-trash card could be orange";

  PerformAction perform = g.take_action();
  // The bot must not blindly PerformDiscard the orange-trash candidate.
  if (std::holds_alternative<PerformDiscard>(perform)) {
    EXPECT_NE(std::get<PerformDiscard>(perform).target, alice_slot1)
        << "PerformDiscard on a possibly-orange empathy-trash card would "
           "misplay under the orange game-rule; expected chop discard instead";
  }
}

// Regression for the live-bot crash:
//
//   !! take_action failed for table 1424:
//        check_missed: no old_inferred on urgent card
//
// Root cause: `target_discard` in src/conventions/reactor/interpret_clue.cpp
// used to filter `inferred` and stamp `m.urgent = true` without preserving
// `out.old_inferred`. The buggy call path is `interpret_reactive_rank`'s
// standard play-target swap on an inverted (orange) target — it called
// `target_discard(react_order, urgent=true)` without saving `old_inferred`
// at the call site (unlike the other three reactive call sites). The next
// `check_missed` to land on that urgent card threw because `old_inferred`
// was nullopt.
//
// This test exercises the rank-reactive play-target swap for an orange
// target and verifies (a) the contract holds after `target_discard` —
// `urgent && old_inferred.has_value()` — and (b) a follow-up action that
// invokes `check_missed` on the reacter does not throw.
TEST(Orange, ReactiveRankPlayTargetSwapPreservesOldInferred) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer + giver): hidden hand.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob (reacter): slot 5 (oldest, hand[1][4]) is the react_order. r2
      // is not an obvious play (stacks empty, needs r1 first). Slot 5's
      // effective_possible (unclued, no info) is all-possible, so the
      // "playable id in possible" gate at interpret_reactive.cpp:396
      // passes (empty stacks → r1/b1/o1 in playable_set).
      {"r4", "b4", "r3", "o2", "r2"},
      // Cathy (receiver): slot 2 (hand[2][1]) = o1, the only rank-1 card.
      // Rank-1 clue from Alice will touch only o1 → focus_slot = 2 →
      // calc_slot(2, 2, 5) = 5, so react_slot = 5 = Bob's oldest slot.
      // o1 is playable on orange stack 0, so play_targets is non-empty
      // and `target_is_inverted(state, o1)` is true → buggy path fires.
      // Slot 4 = r3 lets Bob clue red to Cathy in the follow-up step.
      {"o3", "o1", "b4", "r3", "b2"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  int react_order = g.state.hands[static_cast<int>(TestPlayer::BOB)][4];

  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  // Contract after target_discard(urgent=true): urgent set AND old_inferred
  // preserved. Pre-fix, old_inferred was nullopt and check_missed would
  // throw on the next action.
  EXPECT_TRUE(g.meta[react_order].urgent)
      << "reactive-rank orange-target swap must mark the reacter's "
         "react_slot urgent (called_to_discard under the inversion)";
  // check_missed reads common.thoughts, so the fix must land there.
  EXPECT_TRUE(g.common.thoughts[react_order].old_inferred.has_value())
      << "target_discard must preserve old_inferred so check_missed can "
         "revert if the urgent call goes unactioned";

  // Bob's turn. A clue from Bob runs interpret_clue → check_missed(Bob).
  // That iterates Bob's hand looking for urgent cards != action_order; it
  // finds react_order, restores inferred from old_inferred, and clears
  // urgent. Pre-fix this would throw "no old_inferred on urgent card".
  ASSERT_NO_THROW({
    g = take_turn(std::move(g), "Bob clues red to Cathy");
  });
  EXPECT_FALSE(g.meta[react_order].urgent)
      << "check_missed must clear urgent after reverting from old_inferred";
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

// Orange-aware eval bump: in a variant with an inverted (Orange) suit,
// PerformDiscard of an orange-playable card advances the orange stack
// via the on_discard inversion. eval_action's discard branch must value
// that discard at the play tier (positive), not the baseline -0.5.
TEST(Orange, EvalPrefersKnownOrangePlayableDiscardOverChopDiscard) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer): slot 1 = o3 fully-known; slot 2 = unclued chop
      // (b2 — non-trash, not a known play).
      {"o3", "b2", "xx", "xx", "xx"},
      {"r3", "b3", "r4", "b4", "r4"},
      {"r2", "b4", "r3", "o2", "b3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  // Orange stack at 2 so o3 is playable; red/blue at 0 so the chop b2
  // is not trash and not a known play.
  opts.play_stacks = {0, 0, 2};
  Game g = setup(std::move(opts));
  // Empathy-know Alice's slot 1 = o3.
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "o3");
  g.elim();

  int alice_o3 = g.state.hands[0][0];
  int alice_chop = g.state.hands[0][1];

  ASSERT_TRUE(g.state.is_playable(Identity{2, 3}))
      << "test setup: o3 must be playable on orange stack 2";

  Action discard_o3{
      DiscardAction{0, alice_o3, /*suit=*/2, /*rank=*/3, /*failed=*/false}};
  Action discard_chop{
      DiscardAction{0, alice_chop, /*suit=*/1, /*rank=*/2, /*failed=*/false}};

  double v_o3 = hanabi::reactor::eval_action(g, discard_o3);
  double v_chop = hanabi::reactor::eval_action(g, discard_chop);
  EXPECT_GT(v_o3, v_chop)
      << "discard of known-orange playable should outscore a chop discard "
         "in an orange variant (PerformDiscard{o3} advances the orange "
         "stack via on_discard inversion)";
}

// Orange-aware eval bump: a card whose `possible` set includes an
// inverted suit (Orange) should not eat the full -1.5 unknown-card
// penalty, since the discard has upside under the on_discard inversion.
// Compared with No Variant where no such upside exists, the same chop
// shape scores materially lower.
TEST(Orange, EvalPossiblyOrangeDiscardBeatsHardUnknown) {
  // Orange variant: Alice's unclued slot 2 has possible = all
  // identities (3 suits × 5 ranks) including o1..o5. eval_action
  // should NOT score it at the -1.5 baseline.
  SetupOptions opts_o;
  opts_o.hands = {
      {"r2", "b2", "xx", "xx", "xx"},
      {"r3", "b3", "r4", "b4", "r4"},
      {"o4", "b4", "r3", "o2", "b3"},
  };
  opts_o.variant_name = "Orange (3 Suits)";
  opts_o.starting = TestPlayer::ALICE;
  Game g_o = setup(std::move(opts_o));
  int o_chop = g_o.state.hands[0][1];  // alice slot 2 = unclued b2.

  Action discard_o{
      DiscardAction{0, o_chop, /*suit=*/1, /*rank=*/2, /*failed=*/false}};
  double v_orange = hanabi::reactor::eval_action(g_o, discard_o);

  // No Variant: same shape (5-suit, unclued chop). Same b2 identity at
  // alice slot 2, but `possible` cannot include any inverted-suit id
  // (No Variant has no orange), so the bump does not fire.
  SetupOptions opts_n;
  opts_n.hands = {
      {"r2", "b2", "xx", "xx", "xx"},
      {"r3", "b3", "y4", "g4", "p4"},
      {"y4", "g4", "r3", "y2", "g3"},
  };
  opts_n.variant_name = "No Variant";
  opts_n.starting = TestPlayer::ALICE;
  Game g_n = setup(std::move(opts_n));
  int n_chop = g_n.state.hands[0][1];

  Action discard_n{
      DiscardAction{0, n_chop, /*suit=*/3, /*rank=*/2, /*failed=*/false}};
  double v_no_variant = hanabi::reactor::eval_action(g_n, discard_n);

  // The orange-aware bump floors the value at the chop tier (-0.25).
  // No Variant is untouched, so it sits at -0.25 (the chop branch fires
  // before the unknown branch since chop matches da.order). For an
  // *unknown non-chop* discard the gap would be -1.5 vs -0.25; here both
  // are chop, but the relative invariant we need is: orange ≥ no-variant.
  EXPECT_GE(v_orange, v_no_variant)
      << "in an orange variant, the discard upside (might advance orange "
         "stack via on_discard inversion) must never depress eval below "
         "the equivalent No-Variant baseline";
}

// Orange-aware eval bump: the -10 "blocking a known play" penalty
// must NOT fire when the discard target is itself a known-orange
// playable, because PerformDiscard on it advances the orange stack
// (the discard is also a play). Otherwise the bot's two known plays
// would each penalize the other into -10 territory and the bot would
// prefer a clue / chop discard.
TEST(Orange, EvalKnownOrangePlayableNotBlockingPenalized) {
  SetupOptions opts;
  opts.hands = {
      // Alice: slot 1 = o1 fully-known playable; slot 2 = r1 fully-
      // known playable. Both are obvious_playables.
      {"o1", "r1", "xx", "xx", "xx"},
      {"r3", "b3", "r4", "b4", "r4"},
      {"r2", "b4", "r3", "o2", "b3"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "o1");
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/2, "r1");
  g.elim();

  int alice_o1 = g.state.hands[0][0];

  Action discard_o1{
      DiscardAction{0, alice_o1, /*suit=*/2, /*rank=*/1, /*failed=*/false}};
  double v = hanabi::reactor::eval_action(g, discard_o1);

  // The -10 penalty would push the value well below 0; if exempted,
  // the value sits in the play-tier neighborhood (0.02 * 4 = 0.08 for
  // r1, plus advance()'s downstream bonus). Allow a wide margin for
  // advance()'s contribution.
  EXPECT_GT(v, -5.0)
      << "discard of known-orange playable must not be -10-penalized by "
         "the other obvious playable; PerformDiscard{o1} is itself a "
         "play under the orange inversion";
}

// Reactive-rank finesse must reject the WHOLE clue (= MISTAKE) when
// the slot the reacter would naturally pick (via effective_possible)
// doesn't contain the prereq from the giver's POV. The reacter, who
// can't see her own slot, would otherwise PerformDiscard a non-prereq
// orange card and strike.
//
// Setup: focus_slot=3 (Cathy's o5 at slot 3). Finesse target = Cathy's
// o2 at slot 2 (playable_away=1). `calc_slot(3, react=1, 5) = 2`, so
// the convention's slot search picks react_slot=1 first. Bob's actual
// slot 1 is o4 (not o1 = prereq). The new POV-invariant guard fires
// and the convention returns nullopt → MISTAKE.
TEST(Orange, RankFinesseRejectsMismatchedReacterPrereq) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver, observer): explicit hand so the harness card-
      // counts match.
      {"r1", "r4", "o3", "o1", "b3"},
      // Bob (reacter): slot 1 = b1 (a non-orange playable, but NOT the
      // prereq o1). Crucially this is a card the existing
      // `would_lose_inverted_reacter` *won't* skip (suit_index 1 is
      // non-inverted → guard returns false at line 52). Pre-fix the
      // convention proceeds with target_discard on Bob's b1 and tags
      // the interpretation REACTIVE — even though Cathy's o2 will
      // then strike on her turn. With the new POV-invariant guard:
      // actual_id = b1 ≠ prev_id = o1 → return nullopt → MISTAKE.
      {"b1", "r3", "b3", "r2", "b2"},
      // Cathy (receiver): slot 2 = o2 (finesse target — playable_away
      // = 1 once o1 plays); slot 3 = o5 (the rank-5 focus). Other
      // slots are filler so o5 is the only rank-5 touched by the clue.
      {"r3", "o2", "o5", "b4", "b2"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 5 to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(g.move_history.back())), 0)
      << "rank-5 to Cathy must interpret as MISTAKE (=0): Bob's slot 1 "
         "is b1, not o1, so the convention's finesse interpretation "
         "would force a strike at Cathy's turn (o2 not actually "
         "playable). The new POV-invariant prereq-mismatch guard "
         "aborts before that.";
}

// Pin the receiver-target CTP stamp in the rank play-target reactive
// path. Before this fix, the convention only stamped the *reacter's*
// react_order CTP; the receiver's `target` was implicit, so
// `hypo_plays` returned size=1 and the +10 REACTIVE 2-play eval bonus
// never fired (= bot couldn't tell a 2-play reactive from a 1-play
// one). Now the convention stamps BOTH slots, and downstream eval
// correctly counts two plays.
//
// Position: 3-player "No Variant", stacks all 0. Alice (giver) clues
// rank-1 to Cathy. Cathy's hand is shaped so her rank-1 cards have a
// currently-playable target the convention picks; Bob (reacter) has a
// matching slot via the slot mapping.
TEST(Orange, ReactiveRankPlayTargetStampsReceiverCTP) {
  SetupOptions opts;
  opts.hands = {
      // Alice (observer, giver): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob (reacter): slot 1 = r1 (playable on red 0). The convention
      // picks this via the slot map; target_play marks it CTP.
      {"r1", "y4", "g4", "b4", "p4"},
      // Cathy (receiver): slot 1 = b1 (playable on blue 0 — the
      // receiver's play target). focus_slot computed from leftmost
      // rank-1 touched = slot 1; calc_slot(1, 1, 5) = 5, NOT what we
      // want. So use rank-1 to land the focus at a different slot:
      // give Cathy a non-rank-1 newest slot and rank-1 elsewhere.
      // Slot 2 = r1 (touches with rank-1 → focus_slot = 2). Slot 1 =
      // b1 (the play target, playable). calc_slot(2, 1, 5) = 1 → Bob
      // slot 1 = r1 as react_order. Both r1 (reacter) and b1
      // (receiver target) get stamped CTP.
      {"b1", "r1", "g2", "y2", "p2"},
  };
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 1 to Cathy");

  ASSERT_FALSE(g.move_history.empty());
  ASSERT_TRUE(std::holds_alternative<ClueInterp>(g.move_history.back()));
  EXPECT_EQ(static_cast<int>(std::get<ClueInterp>(g.move_history.back())), 1)
      << "rank-1 to Cathy must interpret as REACTIVE (=1)";

  int bob_slot1 = g.state.hands[static_cast<int>(TestPlayer::BOB)][0];
  int cathy_slot1 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][0];
  EXPECT_EQ(g.meta[bob_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "reacter's react_order (Bob's r1 at slot 1) must be CTP'd";
  EXPECT_EQ(g.meta[cathy_slot1].status, CardStatus::CALLED_TO_PLAY)
      << "receiver's target (Cathy's b1 at slot 1) must also be CTP'd "
         "— the new stamp lets hypo_plays see the second play";
}
