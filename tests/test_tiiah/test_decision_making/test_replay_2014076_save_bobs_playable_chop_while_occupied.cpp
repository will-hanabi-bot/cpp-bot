// TIIAH replay 2014076 (tiiah/CONVENTION.md §2e, reactor0/DECISION_MAKING.md; v18.13.0).
// Throw It in a Hole & Brown (6 Suits). Seats: 0 yagami_blue, 1 yagami_light
// (human), 2 yagami_green (us).
//
// Human diagnostic human_vs_bot_diagnostics/2014076.md T18: light's T17 Blue
// called our o21 as the b2, so we are occupied, and blue's chop o22 is a playable g1
// with nothing else for blue to do. Light's chop, o9, is an n3 with brown on 1: not
// critical, not playable. "yagami_green is expected to save the green 1 by giving a
// stable green clue to yagami_blue". The tier gate rejected every clue of an occupied
// Alice, we played the b2, and blue threw the g1 at T19.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (6 Suits). 3 players, our_player_index=2.

TEST(TiiahChopSave2014076, SaveBobsPlayableChopWhileOccupied) {
  // Reconstruct exactly the Game the live bot saw at turn 18.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2014076,
  "debug": {
    "cards_left": 37,
    "clue_tokens": 2,
    "common_play_stacks": [
      1,
      2,
      0,
      1,
      0,
      0
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 11,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 0,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 5,
        "rank": 2,
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
            "id": [
              2,
              1
            ],
            "inferred": 1073739775,
            "info_lock": null,
            "order": 22,
            "possible": 1073739775,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 1073739744,
            "info_lock": null,
            "order": 20,
            "possible": 1073739744,
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
              3
            ],
            "inferred": 1071576992,
            "info_lock": null,
            "order": 17,
            "possible": 1071576992,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 2097216,
            "info_lock": null,
            "order": 3,
            "possible": 2097216,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              2
            ],
            "inferred": 2048,
            "info_lock": null,
            "order": 2,
            "possible": 2048,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_blue",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              5
            ],
            "inferred": 17318416,
            "info_lock": null,
            "order": 18,
            "possible": 17318416,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 517387693,
            "info_lock": null,
            "order": 9,
            "possible": 1054258605,
            "slot": 2,
            "status": "CALLED_TO_DISCARD",
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
            "inferred": 1054258605,
            "info_lock": null,
            "order": 8,
            "possible": 1054258605,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              4
            ],
            "inferred": 1054258605,
            "info_lock": null,
            "order": 7,
            "possible": 1054258605,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              0,
              3
            ],
            "inferred": 1054258605,
            "info_lock": null,
            "order": 6,
            "possible": 1054258605,
            "slot": 5,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_light",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 65536,
            "info_lock": 65536,
            "order": 21,
            "possible": 1015808,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1015808,
            "info_lock": null,
            "order": 19,
            "possible": 1015808,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1015808,
            "info_lock": null,
            "order": 14,
            "possible": 1015808,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 992,
            "info_lock": null,
            "order": 12,
            "possible": 992,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1072722944,
            "info_lock": null,
            "order": 10,
            "possible": 1072722944,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_green",
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
        "v": "Lock"
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
        "v": "Play"
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
        "k": "discard",
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
        "k": "clue",
        "v": "Play"
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
        "v": "Reveal"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Play"
      }
    ],
    "pairwise_play_stacks": [
      [
        1,
        2,
        0,
        1,
        0,
        0
      ],
      [
        1,
        2,
        0,
        1,
        0,
        1
      ],
      [
        1,
        2,
        0,
        1,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      2,
      0,
      1,
      0,
      1
    ],
    "strikes": 1,
    "superpositions": [
      {
        "holder": 0,
        "order": 4,
        "superposition": 34603008
      }
    ],
    "turn_count": 18,
    "waiting": []
  },
  "game_id": 5459,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 1,
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
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 2,
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
        "rank": 3,
        "suit": 5,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "giver": 1,
        "kind": "C",
        "list": [
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 2,
        "t": "turn"
      },
      {
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 0,
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
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          1
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
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 5
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
        "kind": "C",
        "list": [
          12,
          15
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "giver": 2,
        "kind": "R",
        "list": [
          2,
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          14,
          19,
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 2,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        1,
        3
      ],
      [
        3,
        1
      ],
      [
        2,
        2
      ],
      [
        4,
        2
      ],
      [
        5,
        1
      ],
      [
        2,
        2
      ],
      [
        0,
        3
      ],
      [
        2,
        4
      ],
      [
        1,
        3
      ],
      [
        5,
        3
      ],
      null,
      null,
      null,
      null,
      null,
      [
        1,
        2
      ],
      [
        0,
        2
      ],
      [
        4,
        3
      ],
      [
        0,
        5
      ],
      null,
      [
        1,
        5
      ],
      null,
      [
        2,
        1
      ]
    ],
    "names": [
      "yagami_blue",
      "yagami_light",
      "yagami_green"
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
      "variant_name": "Throw It in a Hole & Brown (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Brown (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-30T01:34:47.490",
  "turn": 18
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  const auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_TRUE(rank || colour) << "a save of blue's g1, not our own b2 play";
  EXPECT_EQ(rank ? rank->target : colour->target, 0) << "to blue";
  if (rank) EXPECT_EQ(rank->value, 1) << "a 1";
  if (colour) EXPECT_EQ(colour->value, 2) << "or Green";
}
