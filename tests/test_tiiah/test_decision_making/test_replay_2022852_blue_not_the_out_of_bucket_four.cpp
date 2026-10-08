// TIIAH replay 2022852, Throw It in a Hole & Omni (5 Suits; buckets {r,y} {g,b} {o})
// (tiiah/CONVENTION.md §1d, the user's ruling). Seats: 0 yagami_black (human),
// 1 will-bot67, 2 will-bot69. Stacks r1 y5 g2 b1 o3 before T33.
//
// T33: will-bot69 gave 4 to will-bot67 for its o34, the r2, answered by black's b2.
// A rank clue names the omni bucket from a blue card, and o34 can be no o4: the
// violation is known. Then the target is the FINESSE from the reacter's card, the
// b3, and only if it cannot be that, any other stack playable. So the 4 means the b3
// and is not will-bot69's to give; Blue is the clue. (The live 4 came from an
// intermediate v22.4.0 build in which the giver took the known-violation exception.)
//
// will-bot69 at T33: it gives Blue to will-bot67, not the 4.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Omni (5 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2022852, BlueNotTheOutOfBucketFour) {
  // Reconstruct exactly the Game the live bot saw at turn 33.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2022852,
  "debug": {
    "cards_left": 15,
    "clue_tokens": 3,
    "common_play_stacks": [
      1,
      5,
      1,
      0,
      3
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 22,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 8,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 27,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 10,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 15,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 16,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 4,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 11,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 18,
        "rank": 1,
        "suit": 4
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
              0,
              4
            ],
            "inferred": 33551743,
            "info_lock": null,
            "order": 33,
            "possible": 33551743,
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
              4
            ],
            "inferred": 33551743,
            "info_lock": null,
            "order": 31,
            "possible": 33551743,
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
              2
            ],
            "inferred": 1045856,
            "info_lock": null,
            "order": 2,
            "possible": 1045856,
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
              2
            ],
            "inferred": 1045856,
            "info_lock": null,
            "order": 1,
            "possible": 1045856,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              0,
              3
            ],
            "inferred": 32505887,
            "info_lock": null,
            "order": 0,
            "possible": 32505887,
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
              2
            ],
            "inferred": 33551743,
            "info_lock": null,
            "order": 34,
            "possible": 33551743,
            "slot": 1,
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
            "inferred": 32641028,
            "info_lock": null,
            "order": 29,
            "possible": 32641028,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              3,
              3
            ],
            "inferred": 32641028,
            "info_lock": null,
            "order": 25,
            "possible": 32641028,
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
              5
            ],
            "inferred": 540688,
            "info_lock": null,
            "order": 23,
            "possible": 540688,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              4
            ],
            "inferred": 270344,
            "info_lock": null,
            "order": 19,
            "possible": 270344,
            "slot": 5,
            "status": "CHOP_MOVED",
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
            "inferred": 33551743,
            "info_lock": null,
            "order": 30,
            "possible": 33551743,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1012765,
            "info_lock": null,
            "order": 28,
            "possible": 1045535,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 979997,
            "info_lock": null,
            "order": 24,
            "possible": 979997,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 135172,
            "info_lock": null,
            "order": 20,
            "possible": 135172,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 30408704,
            "info_lock": null,
            "order": 13,
            "possible": 32505856,
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
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Lock"
      },
      {
        "k": "clue",
        "v": "Stall"
      },
      {
        "k": "play",
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
        "k": "discard",
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "play",
        "v": "None"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        1,
        5,
        2,
        1,
        3
      ],
      [
        1,
        5,
        2,
        0,
        3
      ],
      [
        1,
        5,
        1,
        0,
        3
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      5,
      2,
      1,
      3
    ],
    "strikes": 2,
    "superpositions": [
      {
        "holder": 0,
        "order": 3,
        "superposition": 34816
      },
      {
        "holder": 1,
        "order": 32,
        "superposition": 32770
      }
    ],
    "turn_count": 33,
    "waiting": []
  },
  "game_id": 709,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 2,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 7,
        "max": 25,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 1,
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
        "order": 16,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 2,
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
          10,
          12,
          13,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
        "max": 25,
        "score": 2,
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
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
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
        "order": 18,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          7,
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
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
          17,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 6,
        "max": 25,
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
          13,
          16,
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
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
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 7,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5,
          17,
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 6,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          12,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          5,
          7,
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
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
        "order": 25,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "num": 1,
        "order": 22,
        "t": "strike",
        "turn": 21
      },
      {
        "failed": true,
        "order": 22,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          13,
          26
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "order": 26,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          13,
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 30,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          25,
          29
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
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
        "order": 33,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "order": 32,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 34,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        0,
        3
      ],
      [
        3,
        2
      ],
      [
        3,
        2
      ],
      [
        2,
        2
      ],
      [
        2,
        1
      ],
      [
        1,
        5
      ],
      [
        4,
        1
      ],
      [
        1,
        3
      ],
      [
        0,
        1
      ],
      [
        2,
        1
      ],
      null,
      [
        2,
        2
      ],
      [
        1,
        2
      ],
      null,
      null,
      [
        1,
        1
      ],
      [
        1,
        3
      ],
      [
        4,
        3
      ],
      [
        4,
        1
      ],
      [
        3,
        4
      ],
      null,
      [
        1,
        4
      ],
      [
        0,
        1
      ],
      [
        0,
        5
      ],
      null,
      [
        3,
        3
      ],
      [
        4,
        2
      ],
      [
        0,
        3
      ],
      null,
      [
        4,
        2
      ],
      null,
      [
        0,
        4
      ],
      [
        3,
        1
      ],
      [
        0,
        4
      ],
      [
        0,
        2
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
      "variant_name": "Throw It in a Hole & Omni (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Omni (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-07T19:40:15.550",
  "turn": 33
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformColour>(action)) << "not the 4";
  EXPECT_EQ(std::get<hanabi::PerformColour>(action).target, 1);
  EXPECT_EQ(std::get<hanabi::PerformColour>(action).value, 3) << "Blue";
}
