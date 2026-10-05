// Replay 2019676 T62, will-bot69 (tiiah/CONVENTION.md §2k, v20.23.0). At T32
// will-bot69 threw its o33 `{r5,y5}` into the hole; at T34 will-bot67's Red named
// its o31 the r5, and o33 was settled as the y5, booking the one y5 as played. No
// play reveals a hidden card, so elimination never learned it, and o6 read
// `{y5,p5}` for the rest of the game. At T62, with purple on 4 in its view,
// will-bot69 chucked o46 instead of playing o6. An identity with no copy left by
// our own accounting is now no candidate when judging our own card's play.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & White (6 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2019676, ASettledHoleCardFreesTheKnownP5) {
  // Reconstruct exactly the Game the live bot saw at turn 62.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2019676,
  "debug": {
    "cards_left": 4,
    "clue_tokens": 2,
    "common_play_stacks": [
      5,
      5,
      5,
      5,
      3,
      2
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 19,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 9,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 44,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 48,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 2,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 39,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 36,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 10,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 29,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 25,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 17,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 47,
        "rank": 3,
        "suit": 4
      },
      {
        "order": 43,
        "rank": 1,
        "suit": 5
      },
      {
        "order": 4,
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
              0,
              3
            ],
            "inferred": 396864295,
            "info_lock": null,
            "order": 55,
            "possible": 396864295,
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
            "inferred": 396372775,
            "info_lock": null,
            "order": 53,
            "possible": 396372775,
            "slot": 2,
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
            "inferred": 396372775,
            "info_lock": null,
            "order": 45,
            "possible": 396372775,
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
              4
            ],
            "inferred": 369099040,
            "info_lock": null,
            "order": 22,
            "possible": 369099040,
            "slot": 4,
            "status": "NONE",
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
            "inferred": 536870912,
            "info_lock": null,
            "order": 0,
            "possible": 536870912,
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
            "inferred": 396864295,
            "info_lock": null,
            "order": 54,
            "possible": 396864295,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 396864295,
            "info_lock": null,
            "order": 51,
            "possible": 396864295,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 379595559,
            "info_lock": null,
            "order": 49,
            "possible": 396372775,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 229376,
            "info_lock": null,
            "order": 46,
            "possible": 491520,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 16777728,
            "info_lock": null,
            "order": 6,
            "possible": 16777728,
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
              0,
              1
            ],
            "inferred": 396864295,
            "info_lock": null,
            "order": 52,
            "possible": 396864295,
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
              4
            ],
            "inferred": 396854055,
            "info_lock": null,
            "order": 37,
            "possible": 396854055,
            "slot": 2,
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
            "inferred": 369591079,
            "info_lock": null,
            "order": 34,
            "possible": 369591079,
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
              1
            ],
            "inferred": 302350631,
            "info_lock": null,
            "order": 27,
            "possible": 369591079,
            "slot": 4,
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
            "inferred": 302350631,
            "info_lock": null,
            "order": 24,
            "possible": 369591079,
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
        "k": "clue",
        "v": "Reveal"
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
        "k": "discard",
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
        "k": "clue",
        "v": "Play"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        5,
        5,
        5,
        5,
        4,
        3
      ],
      [
        5,
        5,
        5,
        5,
        3,
        2
      ],
      [
        5,
        5,
        5,
        5,
        3,
        2
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      5,
      5,
      5,
      5,
      4,
      3
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 42,
        "superposition": 142606336
      },
      {
        "holder": 2,
        "order": 50,
        "superposition": 427819008,
        "support": [
          [
            4,
            5,
            1
          ],
          [
            5,
            3,
            1
          ],
          [
            4,
            4,
            2
          ],
          [
            5,
            4,
            2
          ]
        ],
        "worlds": [
          [
            [
              42,
              4,
              4
            ]
          ],
          [
            [
              42,
              5,
              3
            ]
          ]
        ]
      }
    ],
    "turn_count": 62,
    "waiting": []
  },
  "game_id": 12270,
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
        "rank": 4,
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
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
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
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
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
        "kind": "C",
        "list": [
          10
        ],
        "t": "clue",
        "target": 2,
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
        "order": 8,
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
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 1,
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
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          6,
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 5
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
          11,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 16,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
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
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "order": 9,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "giver": 2,
        "kind": "C",
        "list": [
          2,
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
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
        "order": 18,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
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
        "giver": 1,
        "kind": "R",
        "list": [
          2,
          3,
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
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
        "order": 22,
        "p": 0,
        "rank": 4,
        "suit": 1,
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
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          11,
          12,
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
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
        "order": 23,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
          17,
          23
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "giver": 1,
        "kind": "C",
        "list": [
          1,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 7,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "order": 5,
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
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 27,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 11,
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
          19,
          26
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 1,
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
        "order": 26,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
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
        "order": 30,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 19,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          15,
          28
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
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
        "order": 32,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
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
        "order": 33,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 29,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 2,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 0,
        "kind": "C",
        "list": [
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 3,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 35,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          0,
          30
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 2,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          31
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 1,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
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
        "order": 37,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 36,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          32,
          38
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "order": 38,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "order": 39,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "clues": 3,
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
        "giver": 2,
        "kind": "C",
        "list": [
          40
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 2,
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
          35,
          41
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 1,
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
        "kind": "C",
        "list": [
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 0,
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
        "order": 10,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 4,
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
        "num": 45,
        "t": "turn"
      },
      {
        "order": 40,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          32
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 0,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "order": 42,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 44,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "order": 32,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "order": 41,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 46,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 50,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 44,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 47,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 51,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 43,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 48,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 52,
        "t": "turn"
      },
      {
        "order": 28,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 49,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 53,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 47,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 50,
        "p": 2,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 54,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          35,
          46
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 2,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 55,
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
        "order": 51,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 56,
        "t": "turn"
      },
      {
        "order": 50,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 52,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 57,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 48,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 53,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 58,
        "t": "turn"
      },
      {
        "order": 35,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 54,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 59,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          30
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 2,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 60,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 55,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 27,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 61,
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
        4
      ],
      [
        1,
        3
      ],
      [
        0,
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
      [
        0,
        4
      ],
      [
        2,
        3
      ],
      [
        3,
        2
      ],
      [
        0,
        2
      ],
      [
        1,
        2
      ],
      [
        4,
        1
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
        1
      ],
      [
        1,
        1
      ],
      [
        0,
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
        1,
        4
      ],
      [
        2,
        1
      ],
      [
        3,
        1
      ],
      [
        4,
        1
      ],
      [
        5,
        1
      ],
      [
        5,
        1
      ],
      [
        2,
        4
      ],
      [
        3,
        1
      ],
      [
        3,
        5
      ],
      [
        0,
        5
      ],
      [
        2,
        3
      ],
      null,
      [
        5,
        2
      ],
      [
        3,
        4
      ],
      [
        2,
        1
      ],
      [
        5,
        4
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
        4,
        3
      ],
      [
        3,
        3
      ],
      [
        4,
        4
      ],
      [
        5,
        1
      ],
      [
        1,
        1
      ],
      [
        0,
        2
      ],
      null,
      [
        4,
        3
      ],
      [
        1,
        2
      ],
      null,
      [
        5,
        3
      ],
      null,
      [
        0,
        1
      ],
      [
        4,
        4
      ],
      null,
      [
        0,
        3
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
      "variant_name": "Throw It in a Hole & White (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & White (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-05T03:14:09.913",
  "turn": 62
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::TIIAH);
  EXPECT_TRUE(game.me().order_playable(game, 6))
      << "the one y5 is o33, booked when it settled at T34, so o6 is the p5";

  hanabi::PerformAction action = game.take_action();
  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "the known p5, not a chuck";
  EXPECT_EQ(play->target, 6);
}
