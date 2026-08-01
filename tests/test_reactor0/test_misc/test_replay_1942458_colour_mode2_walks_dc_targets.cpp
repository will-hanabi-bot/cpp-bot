// Replay 1942458 turn 47 (reactor0). A Blue colour clue with no playable in
// the receiver's hand is colour mode 2: the reacter blind-plays and the
// receiver discards. The receiver had TWO trash cards, i.e. two legal
// dc-targets, but `dc_candidates` returned only the leftmost and mode 2
// committed to it with no walk. That pairing mapped onto a react slot every
// seat could see was dead, so the clue was rejected as a MISTAKE. Mode 2 now
// walks the candidates, skipping a pairing only on SHARED knowledge (§1g) —
// a react slot only the giver can see is unplayable still rejects.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Extremely Ambiguous & Null (6 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1942458, ColourModeTwoWalksToALiveDcTarget) {
  // Reconstruct exactly the Game the live bot saw at turn 47.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1942458,
  "debug": {
    "cards_left": 16,
    "clue_tokens": 3,
    "current_player_index": 1,
    "discards": [
      {
        "order": 39,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 33,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 34,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 9,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 42,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 22,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 31,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 37,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 18,
        "rank": 1,
        "suit": 5
      },
      {
        "order": 10,
        "rank": 1,
        "suit": 5
      },
      {
        "order": 38,
        "rank": 3,
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
            "id": [
              4,
              1
            ],
            "inferred": 888618467,
            "info_lock": null,
            "order": 40,
            "possible": 888618467,
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
              4
            ],
            "inferred": 888618467,
            "info_lock": null,
            "order": 28,
            "possible": 888618467,
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
              5
            ],
            "inferred": 512,
            "info_lock": null,
            "order": 16,
            "possible": 512,
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
              4
            ],
            "inferred": 14039459,
            "info_lock": null,
            "order": 2,
            "possible": 16203235,
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
            "inferred": 12990851,
            "info_lock": null,
            "order": 1,
            "possible": 16203235,
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
            "id": null,
            "inferred": 905919971,
            "info_lock": null,
            "order": 41,
            "possible": 905919971,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 17301504,
            "info_lock": null,
            "order": 36,
            "possible": 17301504,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 17301504,
            "info_lock": null,
            "order": 30,
            "possible": 17301504,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 2114,
            "info_lock": null,
            "order": 24,
            "possible": 2164802,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 2164800,
            "info_lock": null,
            "order": 8,
            "possible": 2164802,
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
            "clued": true,
            "focused": false,
            "id": [
              2,
              1
            ],
            "inferred": 16203235,
            "info_lock": null,
            "order": 43,
            "possible": 16203235,
            "slot": 1,
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
            "inferred": 15153602,
            "info_lock": null,
            "order": 25,
            "possible": 15153602,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              0,
              1
            ],
            "inferred": 1049633,
            "info_lock": null,
            "order": 21,
            "possible": 1049633,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              5
            ],
            "inferred": 16384,
            "info_lock": null,
            "order": 14,
            "possible": 16384,
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
            "inferred": 4329600,
            "info_lock": null,
            "order": 13,
            "possible": 4329600,
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
        "k": "clue",
        "v": "Reactive"
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
        "v": "Mistake"
      },
      {
        "k": "clue",
        "v": "Stall"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Mistake"
      }
    ],
    "play_stacks": [
      5,
      3,
      1,
      3,
      2,
      4
    ],
    "strikes": 0,
    "turn_count": 47,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_value": 0,
        "focus_slot": 4,
        "giver": 0,
        "inverted": false,
        "react_order": -1,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          43,
          25,
          21,
          14,
          13
        ],
        "rlocks": true,
        "turn": 46
      }
    ]
  },
  "game_id": 3426,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 5
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
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
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
        "cpi": 2,
        "num": 2,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          7
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
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 5,
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
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
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
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          9,
          15,
          17
        ],
        "t": "clue",
        "target": 1,
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 4,
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
          0,
          1,
          2,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
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
        "value": 0
      },
      {
        "clues": 3,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 3,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          6,
          8,
          15,
          17,
          20
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 3,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 22,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 27,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 4,
        "suit": 3,
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
        "num": 22,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          13,
          14,
          21,
          25,
          27
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          8,
          24
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 31,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          30
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 0,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 22,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 14,
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
          8,
          24
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 0,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 3,
        "suit": 1,
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
        "clues": 0,
        "max": 30,
        "score": 15,
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
        "rank": 5,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 34,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 31,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 35,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 1,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 36,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "order": 35,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 37,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          30,
          36
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 0,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 34,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 38,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 37,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 33,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 38,
        "p": 1,
        "rank": 3,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 41,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 39,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          13,
          14,
          21,
          25,
          42
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          21,
          42
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 3,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 42,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 43,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          13,
          14,
          21,
          25,
          43
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        1,
        2
      ],
      [
        2,
        2
      ],
      [
        4,
        4
      ],
      [
        0,
        2
      ],
      [
        5,
        1
      ],
      [
        0,
        1
      ],
      [
        0,
        4
      ],
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
        5,
        1
      ],
      [
        5,
        4
      ],
      [
        5,
        2
      ],
      [
        2,
        3
      ],
      [
        2,
        5
      ],
      [
        3,
        3
      ],
      [
        1,
        5
      ],
      [
        0,
        3
      ],
      [
        5,
        1
      ],
      [
        5,
        3
      ],
      [
        4,
        1
      ],
      [
        0,
        1
      ],
      [
        3,
        1
      ],
      [
        1,
        1
      ],
      null,
      [
        2,
        4
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
        3,
        4
      ],
      [
        1,
        3
      ],
      null,
      [
        3,
        4
      ],
      [
        0,
        5
      ],
      [
        0,
        4
      ],
      [
        1,
        1
      ],
      [
        2,
        1
      ],
      null,
      [
        4,
        1
      ],
      [
        5,
        3
      ],
      [
        0,
        3
      ],
      [
        4,
        1
      ],
      null,
      [
        3,
        1
      ],
      [
        2,
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
      "variant_name": "Extremely Ambiguous & Null (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Extremely Ambiguous & Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-01T02:37:41.403",
  "turn": 47
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::REACTOR0);

  const hanabi::State& s = game.state;
  const int me = s.our_player_index;  // will-bot67, the reacter
  ASSERT_EQ(me, 1);

  // The pairing the old code committed to: the leftmost dc-target mapped to
  // this seat's slot 3, whose possibilities are all unplayable. Crucially
  // that is SHARED knowledge — the reacter can see it too — which is what
  // makes walking past it POV-safe.
  const int slot3 = s.hands[me][2];
  const hanabi::Thought& t3 = game.common.thoughts[slot3];
  bool any_playable = false;
  for (hanabi::Identity id : (t3.inferred.non_empty() ? t3.inferred : t3.possible)) {
    if (s.is_playable(id)) any_playable = true;
  }
  EXPECT_FALSE(any_playable)
      << "slot 3 must be dead by shared knowledge for the retarget to be legal";

  hanabi::PerformAction action = game.take_action();

  // Before the fix, mode 2 took dc_candidates().front(), found slot 3 dead
  // and returned nullopt -> MISTAKE; with no call to action the reacter just
  // chucked its chop.
  auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  EXPECT_FALSE(discard != nullptr && discard->target == s.hands[me][0])
      << "chucking the chop was the bug — the clue was rejected outright";

  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "expected the blind play";
  EXPECT_EQ(play->target, s.hands[me][0])
      << "the walk reaches the next trash dc-target, which pairs with slot 1";
}
