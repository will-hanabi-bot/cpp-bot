// Replay 2018874 T47 (tiiah/CONVENTION.md §2k, v20.11.0, the user's ruling).
// yagami's Blue at T46 stable-called will-bot69's o44 `{b5}`. Blue is on 4 in every
// shared view, but on 3 in will-bot69's private one: it saw yagami's hole card o0 was
// the g4, and keeps the world where its own o29 was a second g4 (yagami's struck). So
// the call did not count as a known play, section 4's 4a opened, and it gave a 3
// stall. A standing call whose reading plays on the shared view plays now.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2018874, StandingCallOnTheSharedViewPlaysNow) {
  // Reconstruct exactly the Game the live bot saw at turn 47.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2018874,
  "debug": {
    "cards_left": 4,
    "clue_tokens": 4,
    "common_play_stacks": [
      3,
      4,
      4,
      4,
      5
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 25,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 4,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 27,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 26,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 34,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 9,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 40,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 17,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 22,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 37,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 39,
        "rank": 2,
        "suit": 4
      },
      {
        "order": 16,
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
              3,
              4
            ],
            "inferred": 6156282,
            "info_lock": null,
            "order": 43,
            "possible": 6156282,
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
              3
            ],
            "inferred": 6088632,
            "info_lock": null,
            "order": 36,
            "possible": 6088632,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              4
            ],
            "inferred": 270600,
            "info_lock": null,
            "order": 28,
            "possible": 270600,
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
            "inferred": 574464,
            "info_lock": null,
            "order": 2,
            "possible": 574464,
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
            "inferred": 574464,
            "info_lock": null,
            "order": 1,
            "possible": 574464,
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
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 524288,
            "info_lock": 524288,
            "order": 44,
            "possible": 6127616,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28666,
            "info_lock": null,
            "order": 32,
            "possible": 28666,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28666,
            "info_lock": null,
            "order": 30,
            "possible": 28666,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1933312,
            "info_lock": null,
            "order": 24,
            "possible": 1933312,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 890,
            "info_lock": null,
            "order": 23,
            "possible": 890,
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
              1,
              1
            ],
            "inferred": 6156282,
            "info_lock": null,
            "order": 45,
            "possible": 6156282,
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
              1
            ],
            "inferred": 1961850,
            "info_lock": null,
            "order": 41,
            "possible": 1961850,
            "slot": 2,
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
            "inferred": 1961850,
            "info_lock": null,
            "order": 38,
            "possible": 1961850,
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
              1
            ],
            "inferred": 1961850,
            "info_lock": null,
            "order": 35,
            "possible": 1961850,
            "slot": 4,
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
            "inferred": 131072,
            "info_lock": null,
            "order": 10,
            "possible": 131072,
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Discard"
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
        "k": "discard",
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
        "v": "Play"
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        4,
        4,
        4,
        5
      ],
      [
        3,
        4,
        4,
        4,
        5
      ],
      [
        3,
        4,
        4,
        4,
        5
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      4,
      4,
      3,
      5
    ],
    "strikes": 2,
    "superpositions": [
      {
        "holder": 0,
        "order": 0,
        "superposition": 270336
      },
      {
        "holder": 1,
        "order": 7,
        "support": [
          [
            0,
            3,
            3
          ],
          [
            1,
            3,
            3
          ],
          [
            0,
            2,
            12
          ],
          [
            1,
            4,
            12
          ]
        ],
        "worlds": [
          [
            [
              8,
              0,
              2
            ],
            [
              29,
              2,
              4
            ]
          ],
          [
            [
              8,
              0,
              2
            ],
            [
              29,
              3,
              4
            ]
          ],
          [
            [
              8,
              1,
              3
            ],
            [
              29,
              2,
              4
            ]
          ],
          [
            [
              8,
              1,
              3
            ],
            [
              29,
              3,
              4
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 29,
        "superposition": 270336
      }
    ],
    "turn_count": 47,
    "waiting": []
  },
  "game_id": 11286,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "suit": 0,
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
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
        "rank": -1,
        "suit": -1,
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
        "rank": 4,
        "suit": 4,
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
          12,
          13,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "order": 15,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 0,
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
        "kind": "C",
        "list": [
          12,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
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
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          3,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
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
        "order": 19,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
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
          21
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 7,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
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
        "kind": "C",
        "list": [
          5,
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
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
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "num": 17,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 2,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
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
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 8,
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
        "clues": 2,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 22,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
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
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
          0,
          28
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
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
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 12,
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
          10,
          11,
          31
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 33,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          11,
          12,
          16,
          33
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 1,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
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
        "order": 34,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 35,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 36,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 34,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
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
        "order": 38,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 39,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
        "order": 37,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
        "order": 16,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 41,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
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
        "order": 39,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 42,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          42
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "order": 42,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 40,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 44,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
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
        "order": 45,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 20,
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
          24,
          44
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 4,
        "max": 25,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        2,
        4
      ],
      [
        2,
        5
      ],
      [
        3,
        1
      ],
      [
        4,
        2
      ],
      [
        0,
        1
      ],
      [
        2,
        3
      ],
      null,
      null,
      null,
      null,
      [
        3,
        3
      ],
      [
        0,
        3
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
        2,
        1
      ],
      null,
      [
        4,
        4
      ],
      null,
      [
        0,
        1
      ],
      [
        1,
        3
      ],
      [
        3,
        3
      ],
      null,
      [
        3,
        1
      ],
      null,
      null,
      [
        0,
        1
      ],
      [
        1,
        2
      ],
      [
        0,
        3
      ],
      [
        0,
        4
      ],
      null,
      null,
      [
        4,
        3
      ],
      null,
      [
        4,
        4
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
        3
      ],
      [
        4,
        1
      ],
      [
        2,
        1
      ],
      [
        4,
        2
      ],
      [
        2,
        3
      ],
      [
        4,
        1
      ],
      [
        0,
        2
      ],
      [
        3,
        4
      ],
      null,
      [
        1,
        1
      ]
    ],
    "names": [
      "yagami_black",
      "will-bot69",
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
      "variant_name": "Throw It in a Hole & Rainbow (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-04T06:58:03.167",
  "turn": 47
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_TRUE(play) << "the called b5, not a section-4 stall";
  EXPECT_EQ(play->target, 44);
}
