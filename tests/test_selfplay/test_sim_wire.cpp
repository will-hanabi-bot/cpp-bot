// The self-play simulator's wire (v17_self_play_diagnostics/sim.h): what each seat
// is shown for one action, in Throw It in a Hole and in an ordinary variant.
// Message-level only -- no game is simulated here.

#include <gtest/gtest.h>

#include <algorithm>
#include <map>

#include "hanabi/basics/variant.h"
#include "sim.h"

using hanabi::ClueKind;
using hanabi::Identity;
using hanabi::get_variant;
using namespace hanabi::selfplay;
using nlohmann::json;

namespace {

WireEvent play_event(Outcome::Kind kind) {
  WireEvent ev;
  ev.outcome.kind = kind;
  ev.outcome.actor = 0;
  ev.outcome.turn = 4;
  ev.outcome.order = 3;
  ev.outcome.id = Identity{0, 1};
  ev.outcome.pressed_play = true;
  ev.outcome.drawn_order = 15;
  ev.drawn_id = Identity{2, 2};
  ev.strikes_after = kind == Outcome::Kind::PLAY_MISSED ? 1 : 0;
  ev.clues_after = 6;
  ev.score_after = kind == Outcome::Kind::PLAY_LANDED ? 1 : 0;
  ev.max_score_after = 25;
  ev.next_turn_num = 5;
  ev.next_player = 1;
  return ev;
}

bool has_type(const std::vector<json>& msgs, const std::string& type) {
  return std::any_of(msgs.begin(), msgs.end(),
                     [&](const json& m) { return m.at("type") == type; });
}

}  // namespace

TEST(SelfPlayWire, HoleHidesEveryPlayFromEverySeat) {
  const auto& v = get_variant("Throw It in a Hole (5 Suits)");
  for (auto kind : {Outcome::Kind::PLAY_LANDED, Outcome::Kind::PLAY_MISSED}) {
    const auto msgs = wire_messages(v, 3, play_event(kind));
    ASSERT_EQ(msgs.size(), 3u);
    for (int seat = 0; seat < 3; ++seat) {
      const auto& m = msgs[seat];
      ASSERT_EQ(m.size(), 4u) << "play, draw, status, turn";
      EXPECT_EQ(m[0].at("type"), "play");
      EXPECT_EQ(m[0].at("suitIndex"), -1);
      EXPECT_EQ(m[0].at("rank"), -1);
      // A miss is not announced: no strike, no failed discard.
      EXPECT_FALSE(has_type(m, "strike"));
      EXPECT_FALSE(has_type(m, "discard"));
      EXPECT_EQ(m[1].at("type"), "draw");
      EXPECT_EQ(m[1].at("suitIndex"), seat == 0 ? -1 : 2) << "the drawer's own card is hidden";
      EXPECT_EQ(m[2].at("type"), "status");
      EXPECT_EQ(m[2].at("clues"), 6);
      EXPECT_EQ(m[2].at("score"), 0) << "the score is not public";
      EXPECT_EQ(m[3].at("type"), "turn");
      EXPECT_EQ(m[3].at("num"), 5);
      EXPECT_EQ(m[3].at("currentPlayerIndex"), 1);
    }
  }
}

TEST(SelfPlayWire, OrdinaryVariantShowsPlaysAndStrikes) {
  const auto& v = get_variant("No Variant");
  {
    const auto msgs = wire_messages(v, 3, play_event(Outcome::Kind::PLAY_LANDED));
    for (int seat = 0; seat < 3; ++seat) {
      const auto& m = msgs[seat];
      EXPECT_EQ(m[0].at("type"), "play");
      EXPECT_EQ(m[0].at("suitIndex"), 0);
      EXPECT_EQ(m[0].at("rank"), 1);
      EXPECT_EQ(m[2].at("type"), "status");
      EXPECT_EQ(m[2].at("score"), 1);
    }
  }
  {
    const auto msgs = wire_messages(v, 3, play_event(Outcome::Kind::PLAY_MISSED));
    for (int seat = 0; seat < 3; ++seat) {
      const auto& m = msgs[seat];
      ASSERT_EQ(m.size(), 5u) << "strike, failed discard, draw, status, turn";
      EXPECT_EQ(m[0].at("type"), "strike");
      EXPECT_EQ(m[0].at("num"), 1);
      EXPECT_EQ(m[1].at("type"), "discard");
      EXPECT_EQ(m[1].at("failed"), true);
      EXPECT_EQ(m[1].at("suitIndex"), 0);
      EXPECT_EQ(m[1].at("rank"), 1);
    }
  }
}

TEST(SelfPlayWire, ClueAndGameOver) {
  const auto& v = get_variant("Throw It in a Hole (5 Suits)");
  WireEvent ev;
  ev.outcome.kind = Outcome::Kind::CLUE;
  ev.outcome.actor = 2;
  ev.outcome.turn = 9;
  ev.outcome.clue_target = 0;
  ev.outcome.clue_kind = ClueKind::RANK;
  ev.outcome.clue_value = 3;
  ev.outcome.touched = {4, 1};
  ev.clues_after = 3;
  ev.game_over = EndCondition::STRIKEOUT;
  const auto msgs = wire_messages(v, 3, ev);
  for (const auto& m : msgs) {
    ASSERT_EQ(m.size(), 3u);
    EXPECT_EQ(m[0].at("type"), "clue");
    EXPECT_EQ(m[0].at("clue").at("type"), 1);
    EXPECT_EQ(m[0].at("clue").at("value"), 3);
    EXPECT_EQ(m[0].at("list"), json::array({4, 1}));
    EXPECT_EQ(m[2].at("type"), "gameOver");
    EXPECT_EQ(m[2].at("endCondition"), 2);
  }
}

TEST(SelfPlayWire, DeckFollowsTheVariantAndTheSeed) {
  EXPECT_EQ(build_deck(get_variant("No Variant"), 1).size(), 50u);
  EXPECT_EQ(build_deck(get_variant("Throw It in a Hole (5 Suits)"), 1).size(), 50u);
  // A dark suit has one copy of each rank.
  EXPECT_EQ(build_deck(get_variant("Black (6 Suits)"), 1).size(), 55u);

  const auto a = build_deck(get_variant("No Variant"), 7);
  EXPECT_EQ(a, build_deck(get_variant("No Variant"), 7));
  EXPECT_NE(a, build_deck(get_variant("No Variant"), 8));
  std::map<std::pair<int, int>, int> counts;
  for (Identity id : a) ++counts[{id.suit_index, id.rank}];
  EXPECT_EQ((counts[{0, 1}]), 3);
  EXPECT_EQ((counts[{0, 5}]), 1);
}

TEST(SelfPlayWire, TheDealHidesOnlyYourOwnCards) {
  const auto deck = build_deck(get_variant("No Variant"), 3);
  const auto msgs = deal_messages(deck, 3, hand_size_for(3));
  for (int seat = 0; seat < 3; ++seat) {
    ASSERT_EQ(msgs[seat].size(), 15u);
    for (const json& m : msgs[seat]) {
      const bool own = m.at("playerIndex") == seat;
      EXPECT_EQ(m.at("suitIndex") == -1, own);
    }
  }
  EXPECT_EQ(hand_size_for(4), 4);
  EXPECT_EQ(hand_size_for(6), 3);
}
