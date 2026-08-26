// The same defect as replay 1974046, reached through a colour clue rather than
// a played duplicate -- which is why both are pinned.
//
// Replay 1974052 T6, "Null-Fives & Dark Orange (6 Suits)", yellow on 1.
// will-bot67's slot 2 (order 13) is read {y1}: the reactive rank-4 clue at T1
// made it a playable, and yagami's yellow clue at T5 intersected that down to
// the one yellow that could have been it. y1 is trash with yellow on 1.
//
// `possible` was still {y1,y2,y3}, and y2 is playable -- so v8.8.0's guard
// refused to chuck it, the chuck list came back empty, and the bot discarded
// its chop instead of the card it could name.
//
// Note the inference here is CORRECT and narrow, arrived at exactly as the
// convention says a stable colour play clue should: it names the leftmost
// touched card that can be playable, judged against what the card was already
// known to be.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Null-Fives & Dark Orange (6 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1974052, KnownTrashIsChuckedAfterAColourClueNarrows) {
  // Reconstruct exactly the Game the live bot saw at turn 6.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1974052,
  "debug": {
    "cards_left": 38,
    "clue_tokens": 5,
    "current_player_index": 2,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 16,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              5
            ],
            "inferred": 1073741343,
            "info_lock": null,
            "order": 3,
            "possible": 1073741343,
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
              3
            ],
            "inferred": 480,
            "info_lock": null,
            "order": 2,
            "possible": 480,
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
              1
            ],
            "inferred": 1073741343,
            "info_lock": null,
            "order": 1,
            "possible": 1073741343,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 1073741343,
            "info_lock": null,
            "order": 0,
            "possible": 1073741343,
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
              2
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 15,
            "possible": 1073741823,
            "slot": 1,
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 9,
            "possible": 1073741823,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 7,
            "possible": 1073741823,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 6,
            "possible": 1073741823,
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
              5
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 5,
            "possible": 1073741823,
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
            "id": null,
            "inferred": 796646935,
            "info_lock": null,
            "order": 14,
            "possible": 796646935,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 32,
            "info_lock": null,
            "order": 13,
            "possible": 224,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 277094408,
            "info_lock": null,
            "order": 12,
            "possible": 277094408,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 277094408,
            "info_lock": null,
            "order": 11,
            "possible": 277094408,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 796646935,
            "info_lock": null,
            "order": 10,
            "possible": 796646935,
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
        "v": "Play"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Stall"
      }
    ],
    "play_stacks": [
      0,
      1,
      0,
      1,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 6,
    "waiting": []
  },
  "game_id": 128,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          11,
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
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 2,
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
        "kind": "C",
        "list": [
          2,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        4,
        2
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
        3,
        5
      ],
      [
        1,
        1
      ],
      [
        2,
        5
      ],
      [
        4,
        4
      ],
      [
        4,
        4
      ],
      [
        3,
        1
      ],
      [
        0,
        2
      ],
      null,
      null,
      null,
      null,
      null,
      [
        2,
        2
      ],
      [
        4,
        2
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
      "variant_name": "Null-Fives & Dark Orange (6 Suits)"
    },
    "our_player_index": 2,
    "reactive_overrides": [
      {
        "clue_value": 5,
        "even": true,
        "kind": "C",
        "reactive_value": 5
      }
    ],
    "rlocks": false,
    "variant": "Null-Fives & Dark Orange (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T20:09:58.381",
  "turn": 6
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards ---------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 2) << "we are will-bot67";
  ASSERT_EQ(s.play_stacks[1], 1) << "guard: yellow is on 1, so y1 is trash";

  const hanabi::Thought& t = game.common.thoughts[13];
  EXPECT_EQ(t.inferred, hanabi::IdentitySet::from_iter({hanabi::Identity{1, 1}}))
      << "guard: the yellow clue narrowed to {y1} -- the intersection with what "
         "the reactive clue had already established, not a replacement of it";
  EXPECT_TRUE(t.possible.exists([&s](hanabi::Identity i) {
    return s.is_playable(i);
  })) << "guard: `possible` still admits the playable y2";
  EXPECT_TRUE(game.common.order_trash(game, 13));

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformDiscard>(action))
      << "got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformDiscard>(action).target, 13)
      << "discard the card we can name as trash, not the chop";
}
