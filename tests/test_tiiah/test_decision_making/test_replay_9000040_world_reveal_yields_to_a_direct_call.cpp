// Self-play seed 40 T40 (game 9000040, TIIAH 6 Suits; tiiah/CONVENTION.md §1b,
// v22.5.0). We are Alice. Purple to Bob would touch his newly touched o39 and his
// clued o32, the p4. On the pair's view purple is on 2, so the p3 could be o39 and
// Bob, who cannot see it, reads the direct call. We see o39 is no p3, refuse it,
// and in the world where hole card o21 was the p3 find a reveal of o32 instead:
// giver and receiver would split. A reveal found only in a world does not answer
// a seat that refused a direct call by sight, so Purple is no clue of ours.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (6 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay9000040, WorldRevealYieldsToADirectCall) {
  // Reconstruct exactly the Game the live bot saw at turn 40.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-alice",
  "ch": "STATE",
  "current_player_index": 0,
  "debug": {
    "cards_left": 19,
    "clue_tokens": 6,
    "common_play_stacks": [
      3,
      3,
      1,
      2,
      2,
      2
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 1,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 17,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 22,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 18,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 2,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 34,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 30,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 37,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 35,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 23,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 5,
        "rank": 2,
        "suit": 4
      },
      {
        "order": 8,
        "rank": 4,
        "suit": 4
      },
      {
        "order": 28,
        "rank": 1,
        "suit": 5
      },
      {
        "order": 33,
        "rank": 4,
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
            "inferred": 1054310399,
            "info_lock": null,
            "order": 38,
            "possible": 1054310399,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1054310399,
            "info_lock": null,
            "order": 31,
            "possible": 1054310399,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 315883822,
            "info_lock": null,
            "order": 19,
            "possible": 517407215,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 315883822,
            "info_lock": null,
            "order": 15,
            "possible": 517407215,
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
              5
            ],
            "inferred": 524288,
            "info_lock": null,
            "order": 3,
            "possible": 524288,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-alice",
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
            "inferred": 1054310399,
            "info_lock": null,
            "order": 39,
            "possible": 1054310399,
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
            "inferred": 1054310399,
            "info_lock": null,
            "order": 36,
            "possible": 1054310399,
            "slot": 2,
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
            "inferred": 277094400,
            "info_lock": null,
            "order": 32,
            "possible": 277094664,
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
              5
            ],
            "inferred": 5396128,
            "info_lock": null,
            "order": 7,
            "possible": 5396128,
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
              5
            ],
            "inferred": 21,
            "info_lock": null,
            "order": 6,
            "possible": 21,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-bob",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 1054310399,
            "info_lock": null,
            "order": 40,
            "possible": 1054310399,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              4
            ],
            "inferred": 277094664,
            "info_lock": null,
            "order": 29,
            "possible": 277094664,
            "slot": 2,
            "status": "CHOP_MOVED",
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
            "inferred": 536887824,
            "info_lock": null,
            "order": 27,
            "possible": 536887824,
            "slot": 3,
            "status": "CHOP_MOVED",
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
            "inferred": 536887824,
            "info_lock": null,
            "order": 25,
            "possible": 536887824,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              5
            ],
            "inferred": 16777216,
            "info_lock": null,
            "order": 12,
            "possible": 16777216,
            "slot": 5,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-cathy",
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
        "v": "Reactive"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Lock"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        3,
        1,
        2,
        2,
        2
      ],
      [
        3,
        3,
        1,
        2,
        2,
        2
      ],
      [
        3,
        3,
        1,
        2,
        3,
        2
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      3,
      1,
      2,
      3,
      2
    ],
    "strikes": 3,
    "superpositions": [
      {
        "holder": 2,
        "order": 16,
        "superposition": 35651584
      },
      {
        "holder": 2,
        "order": 21,
        "superposition": 6291456,
        "support": [
          [
            4,
            3,
            1
          ],
          [
            4,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              16,
              4,
              2
            ]
          ],
          [
            [
              16,
              5,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 24,
        "superposition": 138412032,
        "support": [
          [
            4,
            3,
            5
          ],
          [
            4,
            4,
            10
          ]
        ],
        "worlds": [
          [
            [
              16,
              4,
              2
            ],
            [
              21,
              4,
              2
            ]
          ],
          [
            [
              16,
              4,
              2
            ],
            [
              21,
              4,
              3
            ]
          ],
          [
            [
              16,
              5,
              1
            ],
            [
              21,
              4,
              2
            ]
          ],
          [
            [
              16,
              5,
              1
            ],
            [
              21,
              4,
              3
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 26,
        "superposition": 133120
      }
    ],
    "turn_count": 40,
    "waiting": []
  },
  "game_id": 9000040,
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
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 3,
        "suit": 1,
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
        "rank": 5,
        "suit": 4,
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          9,
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "giver": 1,
        "kind": "C",
        "list": [
          13,
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
        "max": 25,
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
        "kind": "C",
        "list": [
          4,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
        "order": 15,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
          14,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
        "order": 16,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
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
        "order": 18,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
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
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
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
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
        "order": 22,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
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
        "order": 23,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 22,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
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
        "order": 27,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
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
          21,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
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
        "order": 29,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
          20,
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          27,
          25,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 0,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 18,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 28,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
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
        "order": 32,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
          32
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
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
        "order": 33,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          30,
          29
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 0,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 30,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 33,
        "p": 0,
        "rank": 4,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 35,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 34,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 35,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 38,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
        "rank": 2,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
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
      null,
      null,
      [
        3,
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
        5
      ],
      [
        2,
        5
      ],
      [
        4,
        4
      ],
      [
        5,
        2
      ],
      [
        1,
        3
      ],
      [
        3,
        2
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
        4,
        1
      ],
      null,
      [
        5,
        1
      ],
      null,
      [
        1,
        2
      ],
      null,
      [
        0,
        3
      ],
      [
        4,
        2
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
        4,
        3
      ],
      [
        5,
        5
      ],
      null,
      [
        1,
        5
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
        2,
        4
      ],
      null,
      [
        4,
        4
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
        3,
        1
      ],
      [
        1,
        3
      ],
      [
        3,
        1
      ],
      null,
      [
        4,
        1
      ],
      [
        1,
        4
      ]
    ],
    "names": [
      "sim-alice",
      "sim-bob",
      "sim-cathy"
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
      "variant_name": "Throw It in a Hole (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-07T20:02:09.880",
  "turn": 40
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  if (std::holds_alternative<hanabi::PerformColour>(action)) {
    const auto& c = std::get<hanabi::PerformColour>(action);
    EXPECT_FALSE(c.target == 1 && c.value == 4) << "not Purple to Bob";
  }
}
