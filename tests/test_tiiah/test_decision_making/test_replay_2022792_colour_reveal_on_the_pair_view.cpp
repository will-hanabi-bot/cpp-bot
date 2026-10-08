// TIIAH replay 2022792 T30, yagami_blue (tiiah/CONVENTION.md §1b; v22.5.0, the
// user's ruling). Throw It in a Hole & Muddy Rainbow (6 Suits). Seats: 0
// yagami_blue (us), 1 yagami_green, 2 yagami_black (human).
//
// T30: black's Green re-touched our clued o2, the g3, and newly touched o26. Green
// was on 2 on the stacks black and we share, on 1 only on the common stacks
// (yagami_green could not name the g2). A colour play reveal judged on the pair's
// view outranks the leftmost newly touched card: o2 is the call, and o26 carries no
// play inference.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Muddy Rainbow (6 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay2022792, ColourRevealOnThePairView) {
  // Reconstruct exactly the Game the live bot saw at turn 31.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_blue",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2022792,
  "debug": {
    "cards_left": 27,
    "clue_tokens": 2,
    "common_play_stacks": [
      3,
      3,
      1,
      3,
      1,
      0
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 23,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 1,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 18,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 0,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 17,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 20,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 15,
        "rank": 2,
        "suit": 5
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
            "inferred": 33391615,
            "info_lock": null,
            "order": 31,
            "possible": 33391615,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 33558528,
            "info_lock": 33558528,
            "order": 26,
            "possible": 1040219136,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28312443,
            "info_lock": null,
            "order": 16,
            "possible": 28312443,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1040187392,
            "info_lock": null,
            "order": 4,
            "possible": 1040187392,
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
              3
            ],
            "inferred": 4096,
            "info_lock": null,
            "order": 2,
            "possible": 4096,
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
            "clued": false,
            "focused": false,
            "id": [
              5,
              4
            ],
            "inferred": 1073610751,
            "info_lock": null,
            "order": 32,
            "possible": 1073610751,
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
              5
            ],
            "inferred": 17318416,
            "info_lock": null,
            "order": 19,
            "possible": 17318416,
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
              1
            ],
            "inferred": 1082401,
            "info_lock": null,
            "order": 8,
            "possible": 1082401,
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
            "inferred": 449065356,
            "info_lock": null,
            "order": 7,
            "possible": 1053045132,
            "slot": 4,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 17318416,
            "info_lock": null,
            "order": 6,
            "possible": 17318416,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_green",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              5
            ],
            "inferred": 1040219136,
            "info_lock": null,
            "order": 29,
            "possible": 1040219136,
            "slot": 1,
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
            "inferred": 4,
            "info_lock": null,
            "order": 24,
            "possible": 31,
            "slot": 2,
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
            "inferred": 27648,
            "info_lock": null,
            "order": 22,
            "possible": 27648,
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
            "inferred": 28311552,
            "info_lock": null,
            "order": 11,
            "possible": 28311552,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              3
            ],
            "inferred": 4194304,
            "info_lock": null,
            "order": 10,
            "possible": 4194304,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
        "player": 2
      }
    ],
    "in_progress": true,
    "max_ranks": [
      5,
      5,
      5,
      0,
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
        "k": "clue",
        "v": "Stall"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        3,
        1,
        3,
        1,
        0
      ],
      [
        3,
        3,
        1,
        3,
        1,
        0
      ],
      [
        3,
        3,
        2,
        3,
        1,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      3,
      2,
      3,
      1,
      0
    ],
    "strikes": 1,
    "superpositions": [
      {
        "holder": 1,
        "order": 30,
        "superposition": 264192
      }
    ],
    "turn_count": 31,
    "waiting": []
  },
  "game_id": 639,
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
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 5,
        "suit": 1,
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
        "suit": 1,
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
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 3,
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
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          13,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "order": 5,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 2,
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
        "giver": 2,
        "kind": "R",
        "list": [
          8,
          9
        ],
        "t": "clue",
        "target": 1,
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
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
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
          6,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 6,
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
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 2,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "order": 20,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 8,
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
        "giver": 0,
        "kind": "R",
        "list": [
          10,
          12,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 7,
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
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
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
        "order": 24,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 6,
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
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 7,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          4,
          25
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 6,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "order": 25,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
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
        "order": 28,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          27
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 6,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          24,
          28
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 5,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "order": 28,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 2,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 9,
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
          22,
          29
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 30,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          6,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 3,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
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
        "order": 31,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          2,
          4,
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        3,
        1
      ],
      null,
      [
        2,
        3
      ],
      [
        0,
        3
      ],
      null,
      [
        2,
        1
      ],
      [
        1,
        5
      ],
      [
        1,
        3
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
        4,
        3
      ],
      [
        4,
        4
      ],
      [
        3,
        3
      ],
      [
        1,
        1
      ],
      [
        1,
        3
      ],
      [
        5,
        2
      ],
      null,
      [
        3,
        3
      ],
      [
        3,
        1
      ],
      [
        4,
        5
      ],
      [
        4,
        1
      ],
      [
        1,
        2
      ],
      [
        2,
        4
      ],
      [
        1,
        1
      ],
      [
        0,
        3
      ],
      null,
      null,
      [
        0,
        2
      ],
      [
        0,
        1
      ],
      [
        5,
        5
      ],
      [
        2,
        2
      ],
      null,
      [
        5,
        4
      ]
    ],
    "names": [
      "yagami_blue",
      "yagami_green",
      "yagami_black"
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
      "variant_name": "Throw It in a Hole & Muddy Rainbow (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Muddy Rainbow (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-07T18:33:53.366",
  "turn": 31
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  EXPECT_NE(game.meta[26].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the newly touched o26 is not called";
  EXPECT_EQ(game.meta[2].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the Green revealed o2, the g3";
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action));
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 2);
}
