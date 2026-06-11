// Hanab.live replay 1885855, variant "Orange (3 Suits)". Orig players:
// P0=will-bot67, P1=yagami_black, P2=will-bot69.
//
// At turn 25 (action[24]) the live will-bot67 (CP=P0=our ALICE) plays
// r5 (orig 18). That draws the last card and starts the endgame timer.
// Each remaining player gets one final turn: P1 (yagami) holds o5
// (rank-5-clued) but orange stack is at 3, so o5 isn't playable;
// P1 discards. P2 (will-bot69) plays o4 → orange=4. P0 (CP) plays
// nothing useful. Game ends — P1 never gets another turn to play o5,
// so o5 is lost and the final score is 14 instead of 15.
//
// The forced-endgame "5-lockout" rule (src/endgame/forced_endgame.cpp)
// detects this: `cards_left == 1`, o5 holder = P1 at offset 1, o4
// holders all at offset >= 1 → rule fires → bot must clue. The
// resulting clue-then-final-turns sequence reaches score 15.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "test_endgame/replay_helpers.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::replay;

namespace {

// Deck order matches hanab.live export verbatim (1885855.json).
const std::vector<std::pair<int, int>> kDeck = {
    {1, 2}, {1, 1}, {1, 3}, {1, 4}, {1, 1},
    {0, 1}, {0, 1}, {2, 1}, {2, 5}, {1, 1},
    {0, 4}, {0, 4}, {2, 3}, {2, 3}, {0, 1},
    {2, 1}, {0, 3}, {0, 2}, {0, 5}, {2, 4},
    {0, 2}, {0, 3}, {2, 2}, {1, 3}, {1, 2},
    {2, 2}, {2, 1}, {1, 5}, {2, 4}, {1, 4},
};

// Actions 0..23 = turns 1..24. Action 24 (the bug turn) is NOT applied;
// the test inspects what take_action() returns at turn 25.
//
// NOTE: T15 (action[14]) is a type=1 outcome for an inverted-suit
// (orange) non-playable card. The shared helper `apply_orig_action`
// conservatively builds DiscardAction(failed=true) for this case,
// which the engine resolves as a misplay STRIKE — but in the actual
// game the player PerformPlay'd the o3, hitting on_play(inverted)
// (with_discard + regain_clue, no strike). We bypass the helper for
// T15 and feed the engine a PlayAction directly, then continue.
const std::vector<OrigAction> kOrigActionsPart1 = {
    {3, 2, 3},   // T1  P0 rank-3 -> P2
    {0, 9, 0},   // T2  P1 plays orig 9 (b1)
    {0, 14, 0},  // T3  P2 plays orig 14 (r1)
    {1, 4, 0},   // T4  P0 discards orig 4 (b1)
    {2, 2, 2},   // T5  P1 colour-2 (orange) -> P2
    {2, 0, 1},   // T6  P2 colour-1 (blue)   -> P0
    {0, 17, 0},  // T7  P0 plays orig 17 (r2)
    {3, 0, 5},   // T8  P1 rank-5 -> P0
    {0, 16, 0},  // T9  P2 plays orig 16 (r3)
    {0, 0, 0},   // T10 P0 plays orig 0 (b2)
    {3, 0, 4},   // T11 P1 rank-4 -> P0
    {0, 11, 0},  // T12 P2 plays orig 11 (r4)
    {0, 2, 0},   // T13 P0 plays orig 2 (b3)
    {0, 15, 0},  // T14 P1 plays orig 15 (o1) — orange inversion advances o
    // T15 applied manually (see kT15Manual notes above).
};
const std::vector<OrigAction> kOrigActionsPart2 = {
    {0, 3, 0},   // T16 P0 plays orig 3 (b4)
    {3, 2, 4},   // T17 P1 rank-4 -> P2
    {3, 0, 2},   // T18 P2 rank-2 -> P0
    {0, 25, 0},  // T19 P0 plays orig 25 (o2) — orange inversion
    {2, 0, 0},   // T20 P1 colour-0 (red)  -> P0
    {0, 12, 0},  // T21 P2 plays orig 12 (o3) — orange inversion
    {3, 1, 5},   // T22 P0 rank-5 -> P1
    {3, 2, 5},   // T23 P1 rank-5 -> P2
    {0, 27, 0},  // T24 P2 plays orig 27 (b5)
};

// Replicate apply_orig_action for T15 with the correct PerformPlay
// semantics on the inverted suit.
void apply_t15_manually(Game& g, const ReplayContext& ctx) {
  int pi = g.state.current_player_index;  // = CATHY (P2) at T15.
  g.catchup = true;
  // orig 13 = (2, 3) = o3. my_order is identity here.
  int my_order = ctx.orig_to_my_order[13];
  g.handle_action(PlayAction{pi, my_order, /*suit=*/2, /*rank=*/3});
  // Draw the next card (orig 24 = b2) into CATHY's hand.
  int new_my_order = g.state.next_card_order;
  auto [d_suit, d_rank] = ctx.deck[new_my_order];
  g.handle_action(DrawAction{pi, new_my_order, d_suit, d_rank});
  g.handle_action(TurnAction{g.state.turn_count, g.state.next_player_index(pi)});
  g.catchup = false;
}

// Build from will-bot67's perspective (orig P0 = ALICE = observer).
// Player cycle: orig P0 -> P1 -> P2 -> P0. Observer is P0 so my BOB
// (next in cycle) = orig P1 (yagami), my CATHY = orig P2 (will-bot69).
// Original starting player = orig P0 = MY ALICE.
Game build_from_bot67_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = bot67 (observer, hidden hand).
      // orig 0..4 (= MY 0..4), newest-first: 4=b1, 3=b4, 2=b3, 1=b1, 0=b2.
      {"xx", "xx", "xx", "xx", "xx"},
      // BOB = yagami. orig 5..9 (= MY 5..9), newest-first.
      // 9=(1,1)=b1, 8=(2,5)=o5, 7=(2,1)=o1, 6=(0,1)=r1, 5=(0,1)=r1.
      {"b1", "o5", "o1", "r1", "r1"},
      // CATHY = will-bot69. orig 10..14 (= MY 10..14), newest-first.
      // 14=(0,1)=r1, 13=(2,3)=o3, 12=(2,3)=o3, 11=(0,4)=r4, 10=(0,4)=r4.
      {"r1", "o3", "o3", "r4", "r4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

ReplayContext make_ctx() {
  ReplayContext ctx;
  ctx.deck = kDeck;
  // orig P0 -> ALICE, orig P1 -> BOB, orig P2 -> CATHY (identity).
  ctx.orig_to_my_player = {0, 1, 2};
  // Card-order remapping is identity (observer is orig P0).
  ctx.orig_to_my_order.resize(kDeck.size());
  for (int o = 0; o < static_cast<int>(kDeck.size()); ++o)
    ctx.orig_to_my_order[o] = o;
  ctx.my_order_to_id.resize(kDeck.size());
  for (size_t orig_o = 0; orig_o < kDeck.size(); ++orig_o) {
    ctx.my_order_to_id[ctx.orig_to_my_order[orig_o]] = kDeck[orig_o];
  }
  return ctx;
}

}  // namespace

// At turn 25, the live bot picked PerformPlay{order=18} (r5), which
// emptied the deck and locked the o5-holder (P1) out of their final
// turn. The forced-endgame layer must override with a clue.
TEST(EndgameReplay1885855, Turn25ForcedEndgameBlocksR5Play) {
  Game g = build_from_bot67_perspective();
  ReplayContext ctx = make_ctx();
  for (const auto& a : kOrigActionsPart1) apply_orig_action(g, a, ctx);
  apply_t15_manually(g, ctx);
  for (const auto& a : kOrigActionsPart2) apply_orig_action(g, a, ctx);

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.state.cards_left, 1);
  ASSERT_GE(g.state.clue_tokens, 1)
      << "test simulation must have at least 1 clue token (live game did)";
  ASSERT_EQ(g.state.strikes, 0);
  // Sanity: stacks at start of T25 should be (4, 4, 3) — red/blue have
  // played up to 4 (with r5 in CP's hand awaiting play, b5 just played
  // at T24), orange at 3.
  ASSERT_EQ(g.state.play_stacks[0], 4);
  ASSERT_EQ(g.state.play_stacks[1], 5);
  ASSERT_EQ(g.state.play_stacks[2], 3);

  PerformAction perform = g.take_action();
  EXPECT_FALSE(std::holds_alternative<PerformPlay>(perform))
      << "forced-endgame 5-lockout: bot must clue, not play r5; "
         "playing here empties the deck and prevents P1 from ever "
         "reaching their last turn with o stack at 4";
  EXPECT_FALSE(std::holds_alternative<PerformDiscard>(perform))
      << "discarding also empties the deck — same lockout effect";
}
