// Replay 2028742 T13 (TIIAH & Dark Orange (6 Suits)): human diagnostic
// human_vs_bot_diagnostics/2028742.md (v23.40.0, the user's ruling; tiiah/CONVENTION.md
// §2v). Bob, yagami, holds a called p2, so a clue to Cathy, will-bot67, is stable (role
// inversion). She has no safe action, and her chop o20 is the d4, critical and
// unplayable. will-bot69, occupied by its own call to discard o4, gave the 4 that saves
// the d4 and calls o11, the r2, to discard. Green is a stable play on her o10, the g1:
// she plays, and the card she draws becomes her chop. Rung 2d gives that play first.

#include <gtest/gtest.h>

#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

namespace {

// T13, will-bot69's seat. Variant: Throw It in a Hole & Dark Orange (6 Suits).
// 3 players, our_player_index=0. The embedded JSON is the STATE record's `replay`
// section.
hanabi::Game position() {
  static const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2028742,
  "debug": {
    "cards_left": 34,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      2,
      0,
      1,
      1,
      0
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 13,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 2,
        "rank": 4,
        "suit": 2
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 19,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 553665040,
            "info_lock": null,
            "order": 16,
            "possible": 553665040,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 425984,
            "info_lock": null,
            "order": 4,
            "possible": 425984,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 553665040,
            "info_lock": null,
            "order": 1,
            "possible": 553665040,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 449852845,
            "info_lock": null,
            "order": 0,
            "possible": 449852845,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              4
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 15,
            "possible": 1041235967,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              2
            ],
            "inferred": 2097152,
            "info_lock": 2097152,
            "order": 9,
            "possible": 32505856,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              3
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 8,
            "possible": 1041235967,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              3
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 6,
            "possible": 1041235967,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              2
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 5,
            "possible": 1041235967,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 20,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              3
            ],
            "inferred": 138547332,
            "info_lock": null,
            "order": 18,
            "possible": 138547332,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 277094664,
            "info_lock": null,
            "order": 12,
            "possible": 277094664,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              0,
              2
            ],
            "inferred": 658099827,
            "info_lock": null,
            "order": 11,
            "possible": 658099827,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              1
            ],
            "inferred": 658099827,
            "info_lock": null,
            "order": 10,
            "possible": 658099827,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 2
      }
    ],
    "in_progress": true,
    "max_ranks": [
      5,
      5,
      5,
      5,
      5,
      5
    ],
    "move_history": [
      {
        "k": "clue",
        "v": "Reactive"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Reactive"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        1,
        0,
        0,
        0,
        0
      ],
      [
        0,
        2,
        0,
        1,
        1,
        0
      ],
      [
        0,
        2,
        0,
        1,
        1,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      2,
      0,
      1,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 13,
    "waiting": []
  },
  "game_id": 7790,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 1,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 2,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 6,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          1,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 6,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 4,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      null,
      null,
      [
        2,
        4
      ],
      [
        1,
        2
      ],
      null,
      [
        5,
        2
      ],
      [
        3,
        3
      ],
      [
        3,
        1
      ],
      [
        1,
        3
      ],
      [
        4,
        2
      ],
      [
        2,
        1
      ],
      [
        0,
        2
      ],
      [
        1,
        4
      ],
      [
        1,
        1
      ],
      [
        4,
        1
      ],
      [
        2,
        4
      ],
      null,
      [
        1,
        1
      ],
      [
        2,
        3
      ],
      null,
      [
        5,
        4
      ]
    ],
    "names": [
      "will-bot69",
      "yagami_black",
      "will-bot67"
    ],
    "num_players": 3,
    "options": {
      "deck_plays": false,
      "detrimental_characters": false,
      "empty_clues": false,
      "num_players": 3,
      "one_extra_card": false,
      "one_less_card": false,
      "speedrun": false,
      "starting_player": 0,
      "variant_name": "Throw It in a Hole & Dark Orange (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Dark Orange (6 Suits)",
    "zcs_cleared": 0,
    "zcs_turn": -1
  },
  "ts": "2026-10-12T18:22:22.466",
  "turn": 13
}
  )json";
  return hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
}

std::vector<std::pair<hanabi::PerformAction, hanabi::Action>> all_candidate_clues(
    const hanabi::Game& g) {
  const hanabi::State& s = g.state;
  std::vector<std::pair<hanabi::PerformAction, hanabi::Action>> out;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == s.our_player_index) continue;
    for (const hanabi::Clue& clue : s.all_valid_clues(target)) {
      hanabi::PerformAction perform =
          clue.kind == hanabi::ClueKind::COLOUR
              ? hanabi::PerformAction{hanabi::PerformColour{clue.target, clue.value}}
              : hanabi::PerformAction{hanabi::PerformRank{clue.target, clue.value}};
      hanabi::ClueAction act{s.our_player_index, clue.target,
                             s.clue_touched(s.hands[target], clue.kind, clue.value),
                             clue.base()};
      out.emplace_back(perform, hanabi::Action{act});
    }
  }
  return out;
}

const hanabi::reactor0::ClueCandidate* clue_to_cathy(
    const std::vector<hanabi::reactor0::ClueCandidate>& cands, hanabi::ClueKind kind,
    int value) {
  for (const auto& c : cands) {
    if (c.action.target == 2 && c.action.clue.kind == kind && c.action.clue.value == value) {
      return &c;
    }
  }
  return nullptr;
}

constexpr int kGreen = 2;  // suit index: r y g b p d

}  // namespace

// Green calls the g1 and is lifted to HIGH, past the occupied bar; the 4 is still the
// save that touches the chop. Rung 2d gives Green.
TEST(TiiahReplay2028742, StablePlayToCathyBeforeTheChopSave) {
  hanabi::Game game = position();
  const auto cands = hanabi::reactor0::analyse_clues(game, all_candidate_clues(game));
  const auto* green = clue_to_cathy(cands, hanabi::ClueKind::COLOUR, kGreen);
  ASSERT_NE(green, nullptr);
  EXPECT_EQ(green->reading.stable_subject, 10) << "a call on o10, the g1";
  EXPECT_TRUE(green->cathy_play_saves_chop);
  EXPECT_EQ(green->tier, hanabi::reactor0::ClueTier::HIGH);
  const auto* four = clue_to_cathy(cands, hanabi::ClueKind::RANK, 4);
  ASSERT_NE(four, nullptr);
  EXPECT_TRUE(four->saves_cathys_critical_chop) << "guard: the 4 touches o20, the d4";
  hanabi::PerformAction action = game.take_action();
  const auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(colour, nullptr) << "a colour clue, not the 4 that throws the r2";
  EXPECT_EQ(colour->target, 2) << "to will-bot67";
  EXPECT_EQ(colour->value, kGreen);
}

// With one token, 3.1's condition fails: the play is not offered as the save, and the
// 4 is given as before.
TEST(TiiahReplay2028742, WithOneTokenTheTouchSaveStands) {
  hanabi::Game game = position();
  game.state.clue_tokens = 1;
  const auto cands = hanabi::reactor0::analyse_clues(game, all_candidate_clues(game));
  const auto* green = clue_to_cathy(cands, hanabi::ClueKind::COLOUR, kGreen);
  ASSERT_NE(green, nullptr);
  EXPECT_FALSE(green->cathy_play_saves_chop);
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "a rank clue";
  EXPECT_EQ(rank->target, 2) << "to will-bot67";
  EXPECT_EQ(rank->value, 4);
}
