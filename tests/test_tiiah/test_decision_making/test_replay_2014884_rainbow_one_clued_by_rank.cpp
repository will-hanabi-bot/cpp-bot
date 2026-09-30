// TIIAH replay 2014884 (tiiah/CONVENTION.md §1f, the giver's half; v18.19.0).
// Throw It in a Hole & Rainbow (6 Suits). Seats: 0 will-bot69 (us), 1 yagami_black,
// 2 will-bot67.
//
// T4: Bob's slot 1 (o15) is an unclued ra1. We gave Yellow, which Bob reads under
// §1f as the next playable yellow -- the y1 -- so the ra1 would land as a y1.
// Every other colour on it fails the same way (Purple reads `{p2,ra1}` and leaves
// it unnamed), so the rainbow 1 is clued with 1.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (6 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay2014884, RainbowOneCluedByRank) {
  // Reconstruct exactly the Game the live bot saw at turn 4.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2014884,
  "debug": {
    "cards_left": 43,
    "clue_tokens": 7,
    "common_play_stacks": [
      0,
      0,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 0,
    "discards": [],
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
            "order": 4,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 3,
            "possible": 1073741823,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 2,
            "possible": 1073741823,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 1,
            "possible": 1073741823,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 0,
            "possible": 1073741823,
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
              5,
              1
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
              3,
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
              2
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 8,
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
              1,
              3
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 7,
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
              4
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
            "id": [
              4,
              3
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
              1,
              2
            ],
            "inferred": 762010326,
            "info_lock": null,
            "order": 14,
            "possible": 796647159,
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
              2
            ],
            "inferred": 762010326,
            "info_lock": null,
            "order": 13,
            "possible": 796647159,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              0,
              4
            ],
            "inferred": 277094664,
            "info_lock": null,
            "order": 12,
            "possible": 277094664,
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
              4
            ],
            "inferred": 277094664,
            "info_lock": null,
            "order": 11,
            "possible": 277094664,
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
        "k": "play",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        0,
        0,
        0,
        0,
        0
      ],
      [
        0,
        0,
        0,
        0,
        1,
        0
      ],
      [
        0,
        0,
        1,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      0,
      1,
      0,
      1,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 6,
        "superposition": 33792
      },
      {
        "holder": 2,
        "order": 10,
        "superposition": 34605056
      }
    ],
    "turn_count": 4,
    "waiting": []
  },
  "game_id": 6398,
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
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 1,
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
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 5,
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
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      null,
      null,
      null,
      null,
      null,
      [
        2,
        4
      ],
      [
        2,
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
        3,
        2
      ],
      [
        4,
        1
      ],
      [
        2,
        4
      ],
      [
        0,
        4
      ],
      [
        4,
        2
      ],
      [
        1,
        2
      ],
      [
        5,
        1
      ],
      [
        4,
        3
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
      "variant_name": "Throw It in a Hole & Rainbow (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-30T18:00:12.645",
  "turn": 4
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  ASSERT_TRUE(rank) << "1, not a colour that would read the ra1 as its own suit";
  EXPECT_EQ(rank->target, 1) << "to yagami_black";
  EXPECT_EQ(rank->value, 1);
}
