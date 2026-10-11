// Replay 2026697 (TIIAH & Gray (6 Suits); logged under table id 5339): a RE-TOUCH
// rank call names WHITE or GRAY (v23.28.0, the user's ruling; tiiah/CONVENTION.md
// §1b). At T27 yagami_black's 2 re-touched only will-bot67's clued slot 4, o3
// `{g2,b2,p2,gr2}`, with gray on 1. A rank re-touch is how a gray card is named --
// no colour touches it -- so the call is the gr2, not any playable 2.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// T28, will-bot67's seat: its called o3 reads the gr2 (v23.27.0: `{g2,p2,gr2}`).
// Variant: Throw It in a Hole & Gray (6 Suits). 3 players, our_player_index=0.
TEST(TiiahReplay2026697, TheReceiverReadsTheGrayTwo) {
  // Reconstruct exactly the Game the live bot saw at turn 28.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 0,
  "debug": {
    "cards_left": 24,
    "clue_tokens": 3,
    "common_play_stacks": [
      2,
      2,
      1,
      3,
      1,
      1
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 1,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 2,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 8,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 26,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 16,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 17,
        "rank": 4,
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
            "id": null,
            "inferred": 954135997,
            "info_lock": null,
            "order": 29,
            "possible": 954135997,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              5
            ],
            "inferred": 512,
            "info_lock": null,
            "order": 15,
            "possible": 512,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 537411584,
            "info_lock": null,
            "order": 4,
            "possible": 537411584,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 69208064,
            "info_lock": 69208064,
            "order": 3,
            "possible": 69273600,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 537411584,
            "info_lock": null,
            "order": 0,
            "possible": 537411584,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              1
            ],
            "inferred": 1023409663,
            "info_lock": null,
            "order": 30,
            "possible": 1023409663,
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
              4
            ],
            "inferred": 1023409663,
            "info_lock": null,
            "order": 23,
            "possible": 1023409663,
            "slot": 2,
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
            "inferred": 16777216,
            "info_lock": null,
            "order": 9,
            "possible": 16777216,
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
              2
            ],
            "inferred": 335916043,
            "info_lock": null,
            "order": 7,
            "possible": 335916043,
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
            "inferred": 134352900,
            "info_lock": null,
            "order": 5,
            "possible": 134352900,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 1023409663,
            "info_lock": null,
            "order": 28,
            "possible": 1023409663,
            "slot": 1,
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
            "inferred": 1023409663,
            "info_lock": null,
            "order": 22,
            "possible": 1023409663,
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
            "inferred": 1022327262,
            "info_lock": null,
            "order": 14,
            "possible": 1022327262,
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
              4
            ],
            "inferred": 1022327262,
            "info_lock": null,
            "order": 13,
            "possible": 1022327262,
            "slot": 4,
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
            "order": 10,
            "possible": 1082401,
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
      5,
      5,
      5
    ],
    "move_history": [
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
        "k": "discard",
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
        "v": "Play"
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
        "k": "clue",
        "v": "Play"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        2,
        1,
        1,
        1,
        1
      ],
      [
        2,
        2,
        1,
        3,
        1,
        1
      ],
      [
        2,
        2,
        1,
        3,
        1,
        1
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      2,
      2,
      1,
      3,
      1,
      1
    ],
    "strikes": 0,
    "turn_count": 28,
    "waiting": []
  },
  "game_id": 5339,
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
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "clues": 5,
        "max": 30,
        "score": 0,
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
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 15,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
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
          0,
          4,
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 1,
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
          9,
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 7,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
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
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 3,
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
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 3,
        "suit": 2,
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
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5,
          20
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
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
        "order": 23,
        "p": 1,
        "rank": 4,
        "suit": 1,
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
        "num": 17,
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
        "order": 24,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 0,
        "rank": 4,
        "suit": 4,
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
        "clues": 5,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "giver": 2,
        "kind": "C",
        "list": [
          25
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
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
        "order": 27,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
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
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
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
        "order": 24,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 2,
        "rank": 3,
        "suit": 5,
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
        "num": 24,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      null,
      [
        0,
        3
      ],
      [
        1,
        3
      ],
      null,
      null,
      [
        2,
        3
      ],
      [
        1,
        2
      ],
      [
        2,
        2
      ],
      [
        1,
        4
      ],
      [
        4,
        5
      ],
      [
        1,
        1
      ],
      [
        1,
        1
      ],
      [
        5,
        1
      ],
      [
        0,
        4
      ],
      [
        3,
        4
      ],
      [
        1,
        5
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
        4,
        1
      ],
      [
        3,
        1
      ],
      [
        3,
        3
      ],
      [
        3,
        2
      ],
      [
        2,
        3
      ],
      [
        1,
        4
      ],
      [
        2,
        1
      ],
      [
        0,
        1
      ],
      [
        2,
        4
      ],
      [
        0,
        2
      ],
      [
        5,
        3
      ],
      null,
      [
        4,
        1
      ]
    ],
    "names": [
      "will-bot67",
      "will-bot69",
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
      "variant_name": "Throw It in a Hole & Gray (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Gray (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-10T23:48:00.902",
  "turn": 28
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  EXPECT_EQ(game.meta[3].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(game.common.thoughts[3].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity(5, 2)))
      << "the gr2";
}

// T29, will-bot69's seat, outside the clue: the team reads o3 as the gr2 as well.
// Variant: Throw It in a Hole & Gray (6 Suits). 3 players, our_player_index=1.
TEST(TiiahReplay2026697, TheOutsideSeatReadsItToo) {
  // Reconstruct exactly the Game the live bot saw at turn 29.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "debug": {
    "cards_left": 24,
    "clue_tokens": 2,
    "common_play_stacks": [
      2,
      2,
      1,
      3,
      1,
      1
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 1,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 2,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 8,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 26,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 16,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 17,
        "rank": 4,
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
              2,
              5
            ],
            "inferred": 954135997,
            "info_lock": null,
            "order": 29,
            "possible": 954135997,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              5
            ],
            "inferred": 512,
            "info_lock": null,
            "order": 15,
            "possible": 512,
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
              5
            ],
            "inferred": 537411584,
            "info_lock": null,
            "order": 4,
            "possible": 537411584,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              5,
              2
            ],
            "inferred": 69208064,
            "info_lock": 69208064,
            "order": 3,
            "possible": 69273600,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              5
            ],
            "inferred": 537411584,
            "info_lock": null,
            "order": 0,
            "possible": 537411584,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1023409663,
            "info_lock": null,
            "order": 30,
            "possible": 1023409663,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1023409663,
            "info_lock": null,
            "order": 23,
            "possible": 1023409663,
            "slot": 2,
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
            "inferred": 16777216,
            "info_lock": null,
            "order": 9,
            "possible": 16777216,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 2048,
            "info_lock": 67373056,
            "order": 7,
            "possible": 335916043,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 134352900,
            "info_lock": null,
            "order": 5,
            "possible": 134352900,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 1023409632,
            "info_lock": null,
            "order": 28,
            "possible": 1023409632,
            "slot": 1,
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
            "inferred": 1023409632,
            "info_lock": null,
            "order": 22,
            "possible": 1023409632,
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
            "inferred": 1022327232,
            "info_lock": null,
            "order": 14,
            "possible": 1022327232,
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
            "inferred": 30,
            "info_lock": null,
            "order": 13,
            "possible": 30,
            "slot": 4,
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
            "inferred": 1082400,
            "info_lock": null,
            "order": 10,
            "possible": 1082400,
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
      5,
      5,
      5
    ],
    "move_history": [
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
        "k": "discard",
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
        "v": "Play"
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
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Reactive"
      }
    ],
    "pairwise_play_stacks": [
      [
        2,
        2,
        1,
        3,
        1,
        1
      ],
      [
        0,
        2,
        0,
        2,
        1,
        1
      ],
      [
        2,
        2,
        1,
        3,
        1,
        1
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          2,
          2,
          1,
          3,
          1,
          1
        ],
        "focus_slot": 1,
        "giver": 0,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          28,
          22,
          14,
          13,
          10
        ],
        "turn": 28
      }
    ],
    "play_stacks": [
      2,
      2,
      1,
      3,
      1,
      1
    ],
    "strikes": 0,
    "turn_count": 29,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_play_stacks": [
          2,
          2,
          1,
          3,
          1,
          1
        ],
        "clue_value": 0,
        "focus_slot": 1,
        "giver": 0,
        "inverted": false,
        "react_order": 7,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          28,
          22,
          14,
          13,
          10
        ],
        "rlocks": false,
        "turn": 28
      }
    ]
  },
  "game_id": 5339,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 0,
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
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 3,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "clues": 5,
        "max": 30,
        "score": 0,
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
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 15,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
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
          0,
          4,
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 1,
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
          9,
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 7,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
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
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 3,
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
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 3,
        "suit": 2,
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
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5,
          20
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
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
        "order": 23,
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
        "num": 17,
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
        "order": 24,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "giver": 2,
        "kind": "C",
        "list": [
          25
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
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
        "order": 27,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
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
        "order": 24,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 2,
        "rank": 3,
        "suit": 5,
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
        "num": 24,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "giver": 0,
        "kind": "C",
        "list": [
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        5,
        5
      ],
      [
        0,
        3
      ],
      [
        1,
        3
      ],
      [
        5,
        2
      ],
      [
        3,
        5
      ],
      null,
      [
        1,
        2
      ],
      null,
      [
        1,
        4
      ],
      [
        4,
        5
      ],
      [
        1,
        1
      ],
      [
        1,
        1
      ],
      [
        5,
        1
      ],
      [
        0,
        4
      ],
      [
        3,
        4
      ],
      [
        1,
        5
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
        4,
        1
      ],
      [
        3,
        1
      ],
      [
        3,
        3
      ],
      [
        3,
        2
      ],
      [
        2,
        3
      ],
      null,
      [
        2,
        1
      ],
      [
        0,
        1
      ],
      [
        2,
        4
      ],
      [
        0,
        2
      ],
      [
        5,
        3
      ],
      [
        2,
        5
      ],
      null
    ],
    "names": [
      "will-bot67",
      "will-bot69",
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
      "variant_name": "Throw It in a Hole & Gray (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Gray (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-10T23:48:01.431",
  "turn": 29
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  EXPECT_EQ(game.common.thoughts[3].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity(5, 2)))
      << "the gr2";
}
