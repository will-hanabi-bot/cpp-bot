// TIIAH replay 2014538 (reactor0/DECISION_MAKING.md priority 3, rung 3.6b; v18.16.0).
// Throw It in a Hole & Brown (6 Suits). Seats: 0 yagami_black (human),
// 1 yagami_blue, 2 yagami_green (us).
//
// Human diagnostic human_vs_bot_diagnostics/2014538.md T24: one token, and we
// hold a called n2. Black, our Bob, is stuck with a non-critical y4 on chop; blue is
// locked. Rung 3.1's stable play needs two tokens, so 3.7 locked black with a 4 --
// "does not provide yagami_black a safe action at all". Green names black's o26, the
// g3 we can see is playable, and costs the same one clue.
//
// v20.0.0 (rank referential discards on the lock slot) removes the premise. Blue's
// T23 3 to black touched black's lock slot but had a referential target, o25, so it
// is no longer a lock: it calls o25 to discard, and black is not stuck. With black
// holding a safe action, an occupied green at one token gives no clue (the tier gate
// rejects every candidate) and plays its called n2, o10. The diagnostic was written
// against the old reading of T23.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (6 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2014538, StablePlayOverLockAtOneClue) {
  // Reconstruct exactly the Game the live bot saw at turn 24.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2014538,
  "debug": {
    "cards_left": 33,
    "clue_tokens": 1,
    "common_play_stacks": [
      1,
      1,
      2,
      2,
      0,
      0
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 19,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 18,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 1,
        "rank": 2,
        "suit": 5
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              3
            ],
            "inferred": 4329604,
            "info_lock": null,
            "order": 26,
            "possible": 4329604,
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
            "inferred": 29224827,
            "info_lock": null,
            "order": 25,
            "possible": 29224827,
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
            "inferred": 17318416,
            "info_lock": null,
            "order": 23,
            "possible": 17318416,
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
              3
            ],
            "inferred": 4198528,
            "info_lock": null,
            "order": 4,
            "possible": 4198528,
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
              3
            ],
            "inferred": 4198528,
            "info_lock": null,
            "order": 0,
            "possible": 4198528,
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
              2,
              4
            ],
            "inferred": 1056423407,
            "info_lock": null,
            "order": 20,
            "possible": 1056423407,
            "slot": 1,
            "status": "CHOP_MOVED",
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
            "inferred": 1021786560,
            "info_lock": null,
            "order": 9,
            "possible": 1055340992,
            "slot": 2,
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
            "inferred": 17318400,
            "info_lock": null,
            "order": 8,
            "possible": 17318400,
            "slot": 3,
            "status": "CHOP_MOVED",
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
            "inferred": 14,
            "info_lock": null,
            "order": 7,
            "possible": 14,
            "slot": 4,
            "status": "CHOP_MOVED",
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
            "inferred": 14,
            "info_lock": null,
            "order": 5,
            "possible": 14,
            "slot": 5,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_blue",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 24,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 22,
            "possible": 33554431,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33522687,
            "info_lock": null,
            "order": 21,
            "possible": 33522687,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31744,
            "info_lock": null,
            "order": 11,
            "possible": 31744,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 67108864,
            "info_lock": 67108864,
            "order": 10,
            "possible": 1040187392,
            "slot": 5,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_green",
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
        "v": "Lock"
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
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Stall"
      }
    ],
    "pairwise_play_stacks": [
      [
        1,
        1,
        2,
        2,
        0,
        0
      ],
      [
        1,
        1,
        2,
        2,
        1,
        1
      ],
      [
        1,
        1,
        2,
        2,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      1,
      2,
      2,
      1,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 1,
        "support": [
          [
            5,
            1,
            3
          ],
          [
            5,
            2,
            28
          ]
        ],
        "worlds": [
          [
            [
              3,
              4,
              1
            ],
            [
              17,
              2,
              3
            ]
          ],
          [
            [
              3,
              4,
              1
            ],
            [
              17,
              4,
              2
            ]
          ],
          [
            [
              3,
              4,
              1
            ],
            [
              17,
              5,
              1
            ]
          ],
          [
            [
              3,
              5,
              1
            ],
            [
              17,
              2,
              3
            ]
          ],
          [
            [
              3,
              5,
              1
            ],
            [
              17,
              4,
              1
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 3,
        "superposition": 34603008
      },
      {
        "holder": 0,
        "order": 17,
        "superposition": 36704256,
        "support": [
          [
            4,
            2,
            1
          ],
          [
            5,
            1,
            1
          ],
          [
            4,
            1,
            2
          ],
          [
            5,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              3,
              4,
              1
            ]
          ],
          [
            [
              3,
              5,
              1
            ]
          ]
        ]
      }
    ],
    "turn_count": 24,
    "waiting": []
  },
  "game_id": 5951,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
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
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 4,
        "suit": 3,
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
          6
        ],
        "t": "clue",
        "target": 1,
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
        "giver": 1,
        "kind": "C",
        "list": [
          14
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
        "kind": "C",
        "list": [
          5,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 0
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
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
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
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
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
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 4,
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
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 18,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 4,
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
        "cpi": 2,
        "num": 11,
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
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
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
        "giver": 0,
        "kind": "R",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 4,
        "max": 30,
        "score": 5,
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
          11,
          12,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "order": 19,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "kind": "R",
        "list": [
          23
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
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
        "clues": 3,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
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
        "order": 25,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
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
        "value": 5
      },
      {
        "clues": 2,
        "max": 30,
        "score": 8,
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
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 1,
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
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 5,
        "t": "discard"
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
        "kind": "R",
        "list": [
          0,
          4,
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 1,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
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
        2
      ],
      [
        5,
        1
      ],
      [
        4,
        3
      ],
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
        3
      ],
      [
        4,
        5
      ],
      [
        3,
        4
      ],
      null,
      null,
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
        1
      ],
      [
        0,
        1
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
        2,
        4
      ],
      [
        0,
        1
      ],
      [
        2,
        4
      ],
      null,
      null,
      [
        3,
        5
      ],
      null,
      [
        1,
        4
      ],
      [
        2,
        3
      ]
    ],
    "names": [
      "yagami_black",
      "yagami_blue",
      "yagami_green"
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
      "variant_name": "Throw It in a Hole & Brown (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Brown (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-30T04:59:57.908",
  "turn": 24
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  // Not the rank 4 lock either way.
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_TRUE(play) << "black has T23's discard to make; we play our called n2";
  EXPECT_EQ(play->target, 10);
}
