// Smoke test for Game's action handlers and dispatcher.
// Full coverage of empathy / convention behavior lands once Phase 4 (reactor)
// is in place — for now we test the no-convention base.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

using namespace hanabi;

namespace {

Game make_three_player_game() {
  const Variant& v = get_variant("No Variant");
  TableOptions opts;
  opts.num_players = 3;
  opts.variant_name = "No Variant";
  State s = State::create({"Alice", "Bob", "Cathy"}, /*our_player_index=*/0, v,
                            std::move(opts));
  return Game::create(/*table_id=*/0, std::move(s));
}

// Deal a starter hand: 5 cards per player by issuing 15 DrawAction events.
void deal_starting_hands(Game& g, const std::vector<Identity>& cards) {
  ASSERT_EQ(cards.size(), 15u);
  int order = 0;
  for (int round = 0; round < 5; ++round) {
    for (int p = 0; p < 3; ++p) {
      const Identity id = cards[order];
      g.handle_action(DrawAction{p, order, id.suit_index, id.rank});
      ++order;
    }
  }
}

}  // namespace

TEST(Game, CreateInitialShape) {
  Game g = make_three_player_game();
  EXPECT_EQ(g.table_id, 0);
  EXPECT_EQ(g.state.num_players, 3);
  EXPECT_TRUE(g.in_progress);
  EXPECT_EQ(g.players.size(), 3u);
  EXPECT_FALSE(g.common.is_common == false);  // common.is_common should be true
  EXPECT_EQ(g.last_actions.size(), 3u);
  for (const auto& la : g.last_actions) EXPECT_FALSE(la.has_value());
}

TEST(Game, DrawActionAddsCardToHandAndDeck) {
  Game g = make_three_player_game();
  g.handle_action(DrawAction{0, 0, 0, 1});  // Alice draws r1 at order 0
  ASSERT_EQ(g.state.hands[0].size(), 1u);
  EXPECT_EQ(g.state.hands[0][0], 0);
  ASSERT_EQ(g.state.deck.size(), 1u);
  EXPECT_EQ(g.state.deck[0].order, 0);
  EXPECT_EQ(g.state.deck[0].suit_index, 0);
  EXPECT_EQ(g.state.deck[0].rank, 1);
  EXPECT_EQ(g.state.holders.size(), 1u);
  EXPECT_EQ(g.state.holders[0], 0);
  EXPECT_EQ(g.state.cards_left, 49);
}

TEST(Game, FullDealAdvancesTurnTo1) {
  Game g = make_three_player_game();
  std::vector<Identity> deck;
  // 15 cards, 5 of each rank distributed across suits.
  for (int p = 0; p < 3; ++p) {
    for (int slot = 0; slot < 5; ++slot) {
      deck.emplace_back(slot % 5, 1 + (slot % 5));  // arbitrary
    }
  }
  deal_starting_hands(g, deck);
  EXPECT_EQ(g.state.turn_count, 1);
  for (int p = 0; p < 3; ++p) EXPECT_EQ(g.state.hands[p].size(), 5u);
  EXPECT_EQ(g.state.cards_left, 50 - 15);
}

TEST(Game, PlayActionAdvancesPlayStack) {
  Game g = make_three_player_game();
  std::vector<Identity> deck(15, Identity(0, 1));
  deal_starting_hands(g, deck);
  // Alice plays her newest card (slot 0 = order 12, a r1)
  const int order = g.state.hands[0].front();
  g.handle_action(PlayAction{0, order, 0, 1});
  EXPECT_EQ(g.state.play_stacks[0], 1);
  EXPECT_EQ(g.state.hands[0].size(), 4u);
  for (int o : g.state.hands[0]) EXPECT_NE(o, order);
}

TEST(Game, DiscardActionRegainsClueAndUpdatesPile) {
  Game g = make_three_player_game();
  std::vector<Identity> deck(15, Identity(0, 1));
  deal_starting_hands(g, deck);
  g.state.clue_tokens = 5;
  const int order = g.state.hands[0].front();
  g.handle_action(DiscardAction{0, order, 0, 1, /*failed=*/false});
  EXPECT_EQ(g.state.clue_tokens, 6);
  EXPECT_EQ(g.state.discard_stacks[0][0].size(), 1u);
  EXPECT_EQ(g.state.discard_stacks[0][0][0], order);
}

TEST(Game, FailedDiscardIncrementsStrikes) {
  Game g = make_three_player_game();
  std::vector<Identity> deck(15, Identity(0, 1));
  deal_starting_hands(g, deck);
  const int order = g.state.hands[0].front();
  g.handle_action(DiscardAction{0, order, 0, 2, /*failed=*/true});
  EXPECT_EQ(g.state.strikes, 1);
}

TEST(Game, ClueActionMarksTouchedAndDecrementsTokens) {
  Game g = make_three_player_game();
  std::vector<Identity> deck;
  for (int i = 0; i < 15; ++i) deck.emplace_back(i % 5, 1 + (i % 5));
  deal_starting_hands(g, deck);

  // Alice clues Bob about rank-1s. Some orders in Bob's hand are r1.
  const int bob = 1;
  std::vector<int> touched;
  for (int o : g.state.hands[bob]) {
    if (g.state.deck[o].rank == 1) touched.push_back(o);
  }
  g.handle_action(ClueAction{/*giver=*/0, /*target=*/bob, touched,
                             BaseClue(ClueKind::RANK, 1)});

  EXPECT_EQ(g.state.clue_tokens, 7);
  for (int o : touched) {
    EXPECT_TRUE(g.state.deck[o].clued);
    ASSERT_EQ(g.state.deck[o].clues.size(), 1u);
    EXPECT_EQ(g.state.deck[o].clues[0].kind, ClueKind::RANK);
    EXPECT_EQ(g.state.deck[o].clues[0].value, 1);
  }
}

TEST(Game, GameOverEndsInProgress) {
  Game g = make_three_player_game();
  g.handle_action(GameOverAction{1, 0});
  EXPECT_FALSE(g.in_progress);
}

TEST(Game, IsTouchedAfterClue) {
  Game g = make_three_player_game();
  std::vector<Identity> deck(15, Identity(0, 1));
  deal_starting_hands(g, deck);
  const int bob = 1;
  std::vector<int> touched = g.state.hands[bob];  // every card is r1, so all touched
  g.handle_action(ClueAction{0, bob, touched, BaseClue(ClueKind::RANK, 1)});
  for (int o : touched) EXPECT_TRUE(g.is_touched(o));
  // Alice's cards are not clued.
  for (int o : g.state.hands[0]) EXPECT_FALSE(g.is_touched(o));
}
