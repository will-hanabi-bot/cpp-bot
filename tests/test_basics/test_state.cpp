// Port of python-bot/tests/test_basics/test_state.py.
#include <set>

#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

using namespace hanabi;

namespace {

State make_no_variant_state() {
  const Variant& v = get_variant("No Variant");
  TableOptions opts;
  opts.num_players = 3;
  opts.variant_name = "No Variant";
  return State::create({"Alice", "Bob", "Cathy"}, /*our_player_index=*/0, v,
                        std::move(opts));
}

}  // namespace

// --- Construction ---

TEST(State, CreateInitialShape) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.turn_count, 0);
  EXPECT_EQ(s.clue_tokens, 8);
  EXPECT_EQ(s.strikes, 0);
  EXPECT_FALSE(s.endgame_turns.has_value());
  EXPECT_EQ(s.num_players, 3);
  ASSERT_EQ(s.names.size(), 3u);
  EXPECT_EQ(s.names[0], "Alice");
  EXPECT_EQ(s.our_player_index, 0);
  EXPECT_EQ(s.hands.size(), 3u);
  for (const auto& h : s.hands) EXPECT_TRUE(h.empty());
  EXPECT_TRUE(s.deck.empty());
  EXPECT_TRUE(s.holders.empty());
  EXPECT_EQ(s.play_stacks, (std::vector<int>{0, 0, 0, 0, 0}));
  EXPECT_EQ(s.max_ranks, (std::vector<int>{5, 5, 5, 5, 5}));
  EXPECT_EQ(s.discard_stacks.size(), 5u);
  for (const auto& suit : s.discard_stacks) {
    for (const auto& pile : suit) EXPECT_TRUE(pile.empty());
  }
}

TEST(State, CardsLeftMatchesTotalDeckSize) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.cards_left, 50);
  EXPECT_EQ(s.cards_total, 50);
}

TEST(State, PlayableSetIsAllRank1s) {
  State s = make_no_variant_state();
  std::set<int> ords;
  for (Identity id : s.playable_set) ords.insert(id.to_ord());
  std::set<int> expected;
  for (int i = 0; i < 5; ++i) expected.insert(Identity(i, 1).to_ord());
  EXPECT_EQ(ords, expected);
}

TEST(State, CriticalSetIsAllRank5s) {
  State s = make_no_variant_state();
  std::set<int> ords;
  for (Identity id : s.critical_set) ords.insert(id.to_ord());
  std::set<int> expected;
  for (int i = 0; i < 5; ++i) expected.insert(Identity(i, 5).to_ord());
  EXPECT_EQ(ords, expected);
}

TEST(State, AllIdsIsFull25) {
  EXPECT_EQ(make_no_variant_state().all_ids.length(), 25);
}

TEST(State, CardCount) {
  State s = make_no_variant_state();
  for (int suit_index = 0; suit_index < 5; ++suit_index) {
    int expected[] = {3, 2, 2, 2, 1};
    for (int rank = 1; rank <= 5; ++rank) {
      EXPECT_EQ(s.card_count[Identity(suit_index, rank).to_ord()], expected[rank - 1]);
    }
  }
}

// --- Pure helpers ---

TEST(State, IsBasicTrash) {
  State s = make_no_variant_state();
  EXPECT_FALSE(s.is_basic_trash(Identity(0, 1)));
  EXPECT_FALSE(s.is_basic_trash(Identity(0, 5)));
  State s2 = s.with_play(Identity(0, 1));
  EXPECT_TRUE(s2.is_basic_trash(Identity(0, 1)));
  EXPECT_FALSE(s2.is_basic_trash(Identity(0, 2)));
}

TEST(State, IsUseful) {
  State s = make_no_variant_state();
  EXPECT_TRUE(s.is_useful(Identity(0, 1)));
  EXPECT_TRUE(s.is_useful(Identity(0, 5)));
}

TEST(State, IsPlayable) {
  State s = make_no_variant_state();
  EXPECT_TRUE(s.is_playable(Identity(0, 1)));
  EXPECT_FALSE(s.is_playable(Identity(0, 2)));
}

TEST(State, PlayableAway) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.playable_away(Identity(0, 1)), 0);
  EXPECT_EQ(s.playable_away(Identity(0, 3)), 2);
}

TEST(State, ScoreAndMaxScore) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.score(), 0);
  EXPECT_EQ(s.max_score(), 25);
  EXPECT_EQ(s.rem_score(), 25);
}

TEST(State, Pace) {
  EXPECT_EQ(make_no_variant_state().pace(), 28);
}

TEST(State, NextAndLastPlayerIndex) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.next_player_index(0), 1);
  EXPECT_EQ(s.next_player_index(2), 0);
  EXPECT_EQ(s.last_player_index(0), 2);
  EXPECT_EQ(s.last_player_index(2), 1);
}

TEST(State, CanClue) {
  State s = make_no_variant_state();
  EXPECT_TRUE(s.can_clue());
  s.clue_tokens = 0;
  EXPECT_FALSE(s.can_clue());
}

TEST(State, Multiplicity) {
  State s = make_no_variant_state();
  IdentitySet ones = IdentitySet::from_iter(
      {Identity(0, 1), Identity(1, 1), Identity(2, 1), Identity(3, 1), Identity(4, 1)});
  EXPECT_EQ(s.multiplicity(ones), 15);
  IdentitySet fives = IdentitySet::from_iter(
      {Identity(0, 5), Identity(1, 5), Identity(2, 5), Identity(3, 5), Identity(4, 5)});
  EXPECT_EQ(s.multiplicity(fives), 5);
}

// --- with_play ---

TEST(State, WithPlayAdvancesStack) {
  State s = make_no_variant_state().with_play(Identity(0, 1));
  EXPECT_EQ(s.play_stacks, (std::vector<int>{1, 0, 0, 0, 0}));
  EXPECT_EQ(s.score(), 1);
  EXPECT_TRUE(s.playable_set.contains(Identity(0, 2)));
  EXPECT_FALSE(s.playable_set.contains(Identity(0, 1)));
  EXPECT_TRUE(s.trash_set.contains(Identity(0, 1)));
}

TEST(State, WithPlay5RegainsClue) {
  State s = make_no_variant_state();
  for (int r = 1; r <= 4; ++r) s = s.with_play(Identity(0, r));
  s.clue_tokens = 3;
  s = s.with_play(Identity(0, 5));
  EXPECT_EQ(s.play_stacks[0], 5);
  EXPECT_EQ(s.clue_tokens, 4);
}

TEST(State, WithPlay5CapsClueAt8) {
  State s = make_no_variant_state();
  for (int r = 1; r <= 4; ++r) s = s.with_play(Identity(0, r));
  s = s.with_play(Identity(0, 5));
  EXPECT_EQ(s.clue_tokens, 8);
}

// --- with_discard ---

TEST(State, WithDiscardNonCritical) {
  State s = make_no_variant_state().with_discard(Identity(0, 1), /*order=*/42);
  ASSERT_EQ(s.discard_stacks[0][0].size(), 1u);
  EXPECT_EQ(s.discard_stacks[0][0][0], 42);
  EXPECT_EQ(s.base_count[Identity(0, 1).to_ord()], 1);
  EXPECT_TRUE(s.playable_set.contains(Identity(0, 1)));
  EXPECT_FALSE(s.critical_set.contains(Identity(0, 1)));
}

TEST(State, WithDiscardSecondToLastR1BecomesCritical) {
  State s = make_no_variant_state();
  s = s.with_discard(Identity(0, 1), 42);
  s = s.with_discard(Identity(0, 1), 43);
  EXPECT_TRUE(s.critical_set.contains(Identity(0, 1)));
}

TEST(State, WithDiscardCritical5LowersMax) {
  State s = make_no_variant_state().with_discard(Identity(0, 5), 42);
  EXPECT_EQ(s.max_ranks[0], 4);
  EXPECT_EQ(s.max_score(), 4 + 5 * 4);
  EXPECT_TRUE(s.trash_set.contains(Identity(0, 5)));
  EXPECT_FALSE(s.critical_set.contains(Identity(0, 5)));
  EXPECT_FALSE(s.playable_set.contains(Identity(0, 5)));
}

// --- regain_clue ---

TEST(State, RegainClueCapsAt8) {
  EXPECT_EQ(make_no_variant_state().regain_clue().clue_tokens, 8);
}

TEST(State, RegainClueNormalIncrement) {
  State s = make_no_variant_state();
  s.clue_tokens = 5;
  s = s.regain_clue();
  EXPECT_EQ(s.clue_tokens, 6);
}

TEST(State, RegainClueClueStarvedUsesHalfTokens) {
  const Variant& v = get_variant("Clue Starved (5 Suits)");
  TableOptions opts;
  opts.num_players = 3;
  opts.variant_name = "Clue Starved (5 Suits)";
  State s = State::create({"a", "b", "c"}, 0, v, std::move(opts));
  s.clue_tokens = 4;
  State s2 = s.regain_clue();
  EXPECT_EQ(s2.clue_tokens, 4);
  EXPECT_TRUE(s2.half_clue_token);
  State s3 = s2.regain_clue();
  EXPECT_EQ(s3.clue_tokens, 5);
  EXPECT_FALSE(s3.half_clue_token);
}

// --- expand_short ---

TEST(State, ExpandShort) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.expand_short("r1"), Identity(0, 1));
  EXPECT_EQ(s.expand_short("y3"), Identity(1, 3));
  EXPECT_EQ(s.expand_short("g5"), Identity(2, 5));
}

TEST(State, ExpandShortInvalidLength) {
  State s = make_no_variant_state();
  EXPECT_THROW(s.expand_short("r"), std::invalid_argument);
  EXPECT_THROW(s.expand_short("r10"), std::invalid_argument);
}

TEST(State, ExpandShortUnknownColour) {
  EXPECT_THROW(make_no_variant_state().expand_short("z1"), std::invalid_argument);
}

// --- log_id ---

TEST(State, LogId) {
  State s = make_no_variant_state();
  EXPECT_EQ(s.log_id(Identity(0, 1)), "r1");
  EXPECT_EQ(s.log_id(Identity(4, 5)), "p5");
  EXPECT_EQ(s.log_id(std::nullopt), "xx");
}

// --- ended ---

TEST(State, EndedThreeStrikes) {
  State s = make_no_variant_state();
  EXPECT_FALSE(s.ended());
  s.strikes = 3;
  EXPECT_TRUE(s.ended());
}

TEST(State, EndedMaxScore) {
  State s = make_no_variant_state();
  for (int suit_index = 0; suit_index < 5; ++suit_index) {
    for (int rank = 1; rank <= 5; ++rank) {
      s = s.with_play(Identity(suit_index, rank));
    }
  }
  EXPECT_EQ(s.score(), 25);
  EXPECT_TRUE(s.ended());
}
