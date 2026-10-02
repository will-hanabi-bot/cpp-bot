// A stable call that overlaps a call standing in another hand builds on it once
// that call is played (tiiah/CONVENTION.md §1c, the user's ruling; v19.2.0).
//
// TIIAH replay 2017459, Throw It in a Hole & Pink (6 Suits). Seats: 0 yagami_black,
// 1 will-bot67 (Bob), 2 will-bot69 (us, Cathy).
//   T1 yagami's Purple calls will-bot67's o9: the p1.
//   T4 yagami's Purple to us is stable by role inversion and calls o13 on purple
//      0: {p1}. It is a p2.
//   T5 will-bot67 plays its p1. Rule 3 of the call invariants erased our call as
//      dead, and we gave a clue. Rebased on the p1, o13 is the p2, and we play it.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Pink (6 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2017459, StableCallRebasesOnBobsPlayedCall) {
  // Reconstruct exactly the Game the live bot saw at turn 6.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2017459,
  "debug": {
    "cards_left": 43,
    "clue_tokens": 5,
    "common_play_stacks": [
      0,
      0,
      0,
      1,
      1,
      0
    ],
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
              2,
              1
            ],
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
            "id": [
              4,
              4
            ],
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
            "id": [
              3,
              4
            ],
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
            "id": [
              2,
              3
            ],
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
            "id": [
              2,
              1
            ],
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
        "name": "yagami_black",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              0,
              1
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
              0,
              2
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 8,
            "possible": 1041235967,
            "slot": 2,
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
            "inferred": 1041235967,
            "info_lock": null,
            "order": 7,
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
              5,
              4
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
              2,
              5
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
        "name": "will-bot67",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
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
            "focused": false,
            "id": null,
            "inferred": 1048576,
            "info_lock": 3145728,
            "order": 13,
            "possible": 32505856,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 32505856,
            "info_lock": null,
            "order": 12,
            "possible": 32505856,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1040220159,
            "info_lock": null,
            "order": 11,
            "possible": 1040220159,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1040220159,
            "info_lock": null,
            "order": 10,
            "possible": 1040220159,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
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
        "k": "clue",
        "v": "Play"
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
        1,
        1,
        0
      ],
      [
        0,
        0,
        0,
        1,
        1,
        0
      ],
      [
        0,
        0,
        0,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      0,
      0,
      1,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 6,
    "waiting": []
  },
  "game_id": 9511,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
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
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
        "kind": "C",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
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
        "giver": 1,
        "kind": "C",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 14,
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
        "giver": 0,
        "kind": "C",
        "list": [
          12,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 5,
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
        "order": 9,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
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
    "convention": "tiiah",
    "deck": [
      [
        2,
        1
      ],
      [
        2,
        3
      ],
      [
        3,
        4
      ],
      [
        4,
        4
      ],
      [
        2,
        1
      ],
      [
        2,
        5
      ],
      [
        5,
        4
      ],
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
        1
      ],
      null,
      null,
      null,
      null,
      [
        3,
        1
      ],
      null,
      [
        0,
        1
      ]
    ],
    "names": [
      "yagami_black",
      "will-bot67",
      "will-bot69"
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
      "variant_name": "Throw It in a Hole & Pink (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Pink (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-02T22:51:08.387",
  "turn": 6
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_EQ(game.meta[13].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the call stands: it builds on will-bot67's p1";
  EXPECT_EQ(game.common.thoughts[13].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{4, 2}))
      << "rebased on purple 1: the p2";
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr);
  EXPECT_EQ(play->target, 13);
}
