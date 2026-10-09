// The called play in a thin endgame (tiiah/CONVENTION.md §2, v23.13.0, the user's
// ruling; TIIAH only for now).
//
// Human diagnostic 2025488 T50 (human_vs_bot_diagnostics/2025488.md), yagami_green's
// seat. Pace 0, six cards in the deck. Green holds o47, called to play (the u2), and
// Bob (blue) holds b4, y4, y4, r4, r3 -- one critical good card, the two y4s. At pace
// <= 1 with at least three cards left, Alice always plays her called card when Bob
// holds fewer than two critical good cards. Green gave a 5 to black instead.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Dark Null (6 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2025488, ThinEndgameCalledPlay) {
  // Reconstruct exactly the Game the live bot saw at turn 50.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2025488,
  "debug": {
    "cards_left": 6,
    "clue_tokens": 5,
    "common_play_stacks": [
      5,
      3,
      4,
      4,
      4,
      1
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 22,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 27,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 39,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 25,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 21,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 28,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 32,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 38,
        "rank": 2,
        "suit": 3
      },
      {
        "order": 30,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 16,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 29,
        "rank": 2,
        "suit": 4
      },
      {
        "order": 5,
        "rank": 3,
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
              4,
              4
            ],
            "inferred": 1033692138,
            "info_lock": null,
            "order": 48,
            "possible": 1033692138,
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
              3
            ],
            "inferred": 1033692138,
            "info_lock": null,
            "order": 44,
            "possible": 1033692138,
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
              5
            ],
            "inferred": 1033692138,
            "info_lock": null,
            "order": 36,
            "possible": 1033692138,
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
              3
            ],
            "inferred": 1031799680,
            "info_lock": null,
            "order": 3,
            "possible": 1031799680,
            "slot": 4,
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
            "inferred": 956302208,
            "info_lock": null,
            "order": 1,
            "possible": 1031799680,
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
            "focused": true,
            "id": null,
            "inferred": 218103808,
            "info_lock": null,
            "order": 47,
            "possible": 1033692072,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1033692072,
            "info_lock": null,
            "order": 42,
            "possible": 1033692072,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 66,
            "info_lock": null,
            "order": 40,
            "possible": 66,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1006658336,
            "info_lock": null,
            "order": 9,
            "possible": 1006658336,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1006658336,
            "info_lock": null,
            "order": 6,
            "possible": 1006658336,
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
            "clued": false,
            "focused": false,
            "id": [
              3,
              4
            ],
            "inferred": 1033692138,
            "info_lock": null,
            "order": 45,
            "possible": 1033692138,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 960,
            "info_lock": null,
            "order": 41,
            "possible": 992,
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
              4
            ],
            "inferred": 960,
            "info_lock": null,
            "order": 20,
            "possible": 992,
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
            "inferred": 940064776,
            "info_lock": null,
            "order": 14,
            "possible": 1007476746,
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
            "inferred": 4,
            "info_lock": null,
            "order": 12,
            "possible": 4,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_blue",
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
      }
    ],
    "pairwise_play_stacks": [
      [
        5,
        3,
        4,
        4,
        4,
        1
      ],
      [
        5,
        3,
        4,
        4,
        4,
        1
      ],
      [
        5,
        3,
        4,
        4,
        4,
        1
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      5,
      3,
      4,
      4,
      4,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 11,
        "superposition": 270336
      },
      {
        "holder": 1,
        "order": 37,
        "superposition": 83886080
      },
      {
        "holder": 0,
        "order": 46,
        "superposition": 270336
      },
      {
        "holder": 1,
        "order": 47,
        "support": [
          [
            5,
            2,
            1
          ],
          [
            4,
            5,
            2
          ],
          [
            5,
            3,
            2
          ]
        ],
        "worlds": [
          [
            [
              37,
              4,
              5
            ]
          ],
          [
            [
              37,
              5,
              2
            ]
          ]
        ]
      }
    ],
    "turn_count": 50,
    "waiting": []
  },
  "game_id": 3867,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 4,
        "suit": 5,
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
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 1,
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
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 0,
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
        "order": 7,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 1,
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
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          5,
          8,
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
        "failed": false,
        "order": 16,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 3,
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
        "num": 6,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          10,
          12,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 8,
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
        "clues": 6,
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
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
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
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 5,
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
        "order": 19,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 1,
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
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
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
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 18,
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
        "clues": 3,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
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
        "order": 24,
        "p": 0,
        "rank": 2,
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
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          4,
          24
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 9,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 21,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
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
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 26,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 1,
        "rank": 1,
        "suit": 0,
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
        "clues": 3,
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
        "order": 25,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 2,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "order": 29,
        "p": 1,
        "rank": 2,
        "suit": 4,
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
        "clues": 6,
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
          22,
          31
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 5,
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
        "order": 31,
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
        "clues": 5,
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
        "failed": false,
        "order": 32,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 13,
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
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 13,
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
          17,
          34
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "order": 34,
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
        "clues": 4,
        "max": 30,
        "score": 14,
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
          5,
          35
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
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
        "failed": false,
        "order": 30,
        "p": 2,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 38,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 3,
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
        "clues": 6,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 38,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 41,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          20,
          41
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "order": 37,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 42,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
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
        "order": 43,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "failed": false,
        "order": 39,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 44,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
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
          20,
          41,
          43
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
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
        "order": 43,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 45,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
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
        "order": 46,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
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
        "order": 47,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          40
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 5,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "order": 46,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 48,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        4,
        4
      ],
      [
        5,
        4
      ],
      [
        3,
        2
      ],
      [
        5,
        3
      ],
      [
        1,
        2
      ],
      [
        4,
        3
      ],
      null,
      [
        0,
        1
      ],
      [
        4,
        2
      ],
      null,
      [
        4,
        3
      ],
      [
        2,
        4
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
        0,
        4
      ],
      [
        4,
        1
      ],
      [
        4,
        1
      ],
      [
        0,
        5
      ],
      [
        3,
        3
      ],
      [
        3,
        1
      ],
      [
        1,
        4
      ],
      [
        2,
        2
      ],
      [
        0,
        1
      ],
      [
        2,
        1
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
        2,
        3
      ],
      [
        0,
        1
      ],
      [
        2,
        3
      ],
      [
        4,
        2
      ],
      [
        3,
        3
      ],
      [
        0,
        2
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
        3
      ],
      [
        1,
        3
      ],
      [
        3,
        5
      ],
      null,
      [
        3,
        2
      ],
      [
        2,
        1
      ],
      null,
      [
        1,
        4
      ],
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
        3,
        4
      ],
      [
        3,
        4
      ],
      null,
      [
        4,
        4
      ]
    ],
    "names": [
      "yagami_black",
      "yagami_green",
      "yagami_blue"
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
      "variant_name": "Throw It in a Hole & Dark Null (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Dark Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-09T21:50:39.845",
  "turn": 50
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::TIIAH);
  EXPECT_EQ(game.meta[47].status, hanabi::CardStatus::CALLED_TO_PLAY);
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action)) << "not the 5 to black";
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 47) << "the called u2";
}
