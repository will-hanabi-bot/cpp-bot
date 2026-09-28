// TIIAH replay 2011854 (tiiah/CONVENTION.md §1c, §1e rules 5 and 8, v16.27.0).
// Seats: 0 will-bot69, 1 yagami, 2 will-bot67 (us).
//
//   1. THE REFUSAL. yagami's o24 `{r3,p1,p2}` went into the hole at T20 as the p1,
//      and at T26 his Rank 3 to will-bot69 named o29 -- another p1 -- with us
//      reacting. We could see it was dead. Five refusals were on offer and the tier
//      gate dropped all of them (TODO.md 51), so we reacted, and will-bot69 struck.
//      Now we refuse with Purple, which also names will-bot69's p2; our own o20
//      stays called as the g4; and reading the refusal no longer settles the first
//      of yagami's three hole cards that admit the p1 (o5, really the b1).
//   2. THE DUPE STRIKE. The p1 that struck at T28 was a copy of one will-bot69 had
//      watched go in, so every seat knew purple was on 1 -- but the shared view
//      stayed at 0, and yagami's T41 Rank 5 (our b4 with will-bot69's p2) was
//      unreadable to us. We discarded chop.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

namespace {

constexpr int kPurple = 4;
const hanabi::Identity kG4{2, 4}, kP2{4, 2}, kB4{3, 4};

}  // namespace

TEST(TiiahReplay2011854, WeRefuseTheDeadPOneWithPurple) {
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2011854,
  "debug": {
    "cards_left": 20,
    "clue_tokens": 1,
    "common_play_stacks": [
      2,
      3,
      3,
      0,
      0
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 22,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 25,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 1,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 14,
        "rank": 1,
        "suit": 3
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
            "inferred": 29192043,
            "info_lock": null,
            "order": 29,
            "possible": 29192043,
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
              1
            ],
            "inferred": 29192043,
            "info_lock": null,
            "order": 28,
            "possible": 29192043,
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
              3
            ],
            "inferred": 4325508,
            "info_lock": null,
            "order": 23,
            "possible": 4325508,
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
              5
            ],
            "inferred": 27026441,
            "info_lock": null,
            "order": 3,
            "possible": 27026441,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              2
            ],
            "inferred": 2164738,
            "info_lock": null,
            "order": 2,
            "possible": 2164738,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              2
            ],
            "inferred": 16199151,
            "info_lock": null,
            "order": 26,
            "possible": 16199151,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              4
            ],
            "inferred": 8192,
            "info_lock": null,
            "order": 17,
            "possible": 10240,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 15138816,
            "info_lock": null,
            "order": 9,
            "possible": 15138816,
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
            "inferred": 16,
            "info_lock": null,
            "order": 8,
            "possible": 16,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 15138816,
            "info_lock": null,
            "order": 7,
            "possible": 15138816,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24858343,
            "info_lock": null,
            "order": 27,
            "possible": 24858343,
            "slot": 1,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 8192,
            "info_lock": 8192,
            "order": 20,
            "possible": 8659208,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24838887,
            "info_lock": null,
            "order": 13,
            "possible": 24838887,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24838887,
            "info_lock": null,
            "order": 12,
            "possible": 24838887,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 8651016,
            "info_lock": null,
            "order": 10,
            "possible": 8651016,
            "slot": 5,
            "status": "CHOP_MOVED",
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
        "k": "clue",
        "v": "Lock"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        2,
        4,
        3,
        1,
        1
      ],
      [
        2,
        4,
        3,
        0,
        0
      ],
      [
        2,
        3,
        3,
        0,
        0
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          2,
          3,
          3,
          0,
          0
        ],
        "focus_slot": 3,
        "giver": 1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          29,
          28,
          23,
          3,
          2
        ],
        "turn": 26
      }
    ],
    "play_stacks": [
      2,
      4,
      3,
      1,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 4,
        "superposition": 384,
        "support": [
          [
            0,
            2,
            15
          ],
          [
            1,
            3,
            21845
          ],
          [
            1,
            4,
            43690
          ],
          [
            0,
            1,
            65520
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 1081377
      },
      {
        "holder": 1,
        "order": 6,
        "superposition": 192,
        "support": [
          [
            1,
            2,
            13
          ],
          [
            1,
            3,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 15,
        "superposition": 96,
        "support": [
          [
            0,
            2,
            1
          ],
          [
            1,
            1,
            13
          ],
          [
            0,
            1,
            14
          ],
          [
            1,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 16,
        "superposition": 768,
        "support": [
          [
            1,
            4,
            1
          ],
          [
            1,
            5,
            2
          ]
        ],
        "worlds": [
          [
            [
              4,
              1,
              3
            ]
          ],
          [
            [
              4,
              1,
              4
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 18,
        "superposition": 1049601
      },
      {
        "holder": 2,
        "order": 19,
        "support": [
          [
            2,
            2,
            1023
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 21,
        "support": [
          [
            1,
            4,
            1
          ],
          [
            1,
            5,
            2
          ]
        ],
        "worlds": [
          [
            [
              4,
              1,
              3
            ]
          ],
          [
            [
              4,
              1,
              4
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 24,
        "superposition": 3145732,
        "support": [
          [
            4,
            1,
            29451204315
          ],
          [
            4,
            2,
            281445525506340
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ]
        ]
      }
    ],
    "turn_count": 27,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          2,
          3,
          3,
          0,
          0
        ],
        "clue_value": 3,
        "focus_slot": 3,
        "giver": 1,
        "inverted": false,
        "react_order": 20,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          29,
          28,
          23,
          3,
          2
        ],
        "rlocks": false,
        "turn": 26
      }
    ]
  },
  "game_id": 2749,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "order": 5,
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
        "giver": 2,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 6,
        "max": 25,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
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
        "order": 16,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
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
        "rank": 4,
        "suit": 2,
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
        "giver": 2,
        "kind": "R",
        "list": [
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
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
        "kind": "C",
        "list": [
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
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
        "order": 18,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
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
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
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
        "kind": "C",
        "list": [
          11,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
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
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
          4,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
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
        "order": 23,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
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
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 1,
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
        "num": 17,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 21,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": 2,
        "suit": 2,
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
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 2,
        "rank": -1,
        "suit": -1,
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
        "failed": false,
        "order": 25,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 10,
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
          10,
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 2,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
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
        "giver": 1,
        "kind": "R",
        "list": [
          23
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 1,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
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
        3,
        1
      ],
      [
        4,
        2
      ],
      [
        4,
        5
      ],
      [
        1,
        3
      ],
      [
        3,
        1
      ],
      [
        1,
        2
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
        3,
        2
      ],
      null,
      [
        2,
        3
      ],
      null,
      null,
      [
        3,
        1
      ],
      [
        1,
        1
      ],
      [
        1,
        4
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
        2
      ],
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
        4,
        3
      ],
      [
        4,
        1
      ],
      [
        2,
        3
      ],
      [
        2,
        2
      ],
      null,
      [
        1,
        1
      ],
      [
        4,
        1
      ]
    ],
    "names": [
      "will-bot69",
      "yagami_black",
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-28T05:03:11.492",
  "turn": 27
}
  )json";
  hanabi::Game g = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  ASSERT_FALSE(g.waiting.empty()) << "guard: yagami's Rank 3 is waiting on us";
  ASSERT_EQ(g.meta[20].status, hanabi::CardStatus::CALLED_TO_PLAY);
  ASSERT_TRUE(g.meta[5].superposed()) << "guard: yagami's o5 {r1,y1,b1,p1} is open";

  hanabi::PerformAction action = g.take_action();
  const auto* clue = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(clue, nullptr) << "it played o20 as the reaction here before";
  EXPECT_EQ(clue->target, 0);
  EXPECT_EQ(clue->value, kPurple);

  // ...and what the refusal leaves behind.
  g.catchup = true;
  g.handle_action(hanabi::ClueAction{2, 0, {29, 23, 3, 2},
                                     hanabi::BaseClue{hanabi::ClueKind::COLOUR, kPurple}});
  g.handle_action(hanabi::TurnAction{g.state.turn_count, 0});
  g.catchup = false;
  EXPECT_TRUE(g.waiting.empty());
  EXPECT_NE(g.meta[29].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "will-bot69's half of the pairing is void";
  EXPECT_EQ(g.meta[20].status, hanabi::CardStatus::CALLED_TO_PLAY) << "ours stands";
  EXPECT_EQ(g.common.thoughts[20].inferred, hanabi::IdentitySet::single(kG4));
  EXPECT_EQ(g.meta[2].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(g.common.thoughts[2].inferred, hanabi::IdentitySet::single(kP2));
  EXPECT_EQ(g.state.common_play_stacks[kPurple], 1) << "the refused p1 is down";
  EXPECT_EQ(g.state.common_play_stacks[0], 2)
      << "settling o5 as the p1 left o24 the r3, and the shared view on red 3";
  EXPECT_TRUE(g.meta[5].superposed()) << "o5 was the b1: nothing may name it the p1";
}

TEST(TiiahReplay2011854, TheDupeStrikeIsCommonKnowledge) {
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2011854,
  "debug": {
    "cards_left": 9,
    "clue_tokens": 1,
    "common_play_stacks": [
      2,
      3,
      4,
      3,
      0
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 22,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 35,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 26,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 25,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 17,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 1,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 14,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 34,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 29,
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
              2
            ],
            "inferred": 16188847,
            "info_lock": null,
            "order": 40,
            "possible": 16188847,
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
              1
            ],
            "inferred": 11535659,
            "info_lock": null,
            "order": 28,
            "possible": 11535659,
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
              3
            ],
            "inferred": 4194436,
            "info_lock": null,
            "order": 23,
            "possible": 4194436,
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
              5
            ],
            "inferred": 16793600,
            "info_lock": null,
            "order": 3,
            "possible": 16793600,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              2
            ],
            "inferred": 2097154,
            "info_lock": null,
            "order": 2,
            "possible": 2097154,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              1
            ],
            "inferred": 33506735,
            "info_lock": null,
            "order": 38,
            "possible": 33506735,
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
            "inferred": 33506735,
            "info_lock": null,
            "order": 36,
            "possible": 33506735,
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
            "inferred": 15138816,
            "info_lock": null,
            "order": 9,
            "possible": 15138816,
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
            "inferred": 16,
            "info_lock": null,
            "order": 8,
            "possible": 16,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 15138816,
            "info_lock": null,
            "order": 7,
            "possible": 15138816,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33506735,
            "info_lock": null,
            "order": 39,
            "possible": 33506735,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 8651008,
            "info_lock": null,
            "order": 37,
            "possible": 8651016,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4325508,
            "info_lock": null,
            "order": 13,
            "possible": 4325508,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4325508,
            "info_lock": null,
            "order": 12,
            "possible": 4325508,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 8651016,
            "info_lock": null,
            "order": 10,
            "possible": 8651016,
            "slot": 5,
            "status": "CHOP_MOVED",
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
        "k": "clue",
        "v": "Lock"
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Mistake"
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        5,
        4,
        3,
        1
      ],
      [
        2,
        4,
        4,
        3,
        0
      ],
      [
        2,
        3,
        4,
        2,
        0
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          2,
          3,
          4,
          3,
          0
        ],
        "focus_slot": 5,
        "giver": 1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          40,
          28,
          23,
          3,
          2
        ],
        "turn": 41
      }
    ],
    "play_stacks": [
      3,
      5,
      5,
      3,
      1
    ],
    "strikes": 1,
    "superpositions": [
      {
        "holder": 0,
        "order": 4,
        "superposition": 384,
        "support": [
          [
            0,
            2,
            15
          ],
          [
            1,
            3,
            21845
          ],
          [
            1,
            4,
            43690
          ],
          [
            0,
            1,
            65520
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 1081377
      },
      {
        "holder": 1,
        "order": 6,
        "superposition": 192,
        "support": [
          [
            1,
            2,
            13
          ],
          [
            1,
            3,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              0,
              2,
              1
            ],
            [
              15,
              1,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 15,
        "superposition": 96,
        "support": [
          [
            0,
            2,
            1
          ],
          [
            1,
            1,
            13
          ],
          [
            0,
            1,
            14
          ],
          [
            1,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 16,
        "superposition": 768,
        "support": [
          [
            1,
            4,
            1
          ],
          [
            1,
            5,
            2
          ]
        ],
        "worlds": [
          [
            [
              4,
              1,
              3
            ]
          ],
          [
            [
              4,
              1,
              4
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 18,
        "superposition": 1049601
      },
      {
        "holder": 2,
        "order": 19,
        "support": [
          [
            2,
            2,
            1023
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 21,
        "support": [
          [
            1,
            4,
            1
          ],
          [
            1,
            5,
            2
          ]
        ],
        "worlds": [
          [
            [
              4,
              1,
              3
            ]
          ],
          [
            [
              4,
              1,
              4
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 24,
        "superposition": 3145732,
        "support": [
          [
            4,
            1,
            29451204315
          ],
          [
            4,
            2,
            281445525506340
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              1
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              2
            ],
            [
              18,
              4,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              0,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              2,
              1
            ]
          ],
          [
            [
              5,
              4,
              1
            ],
            [
              15,
              1,
              2
            ],
            [
              6,
              1,
              3
            ],
            [
              18,
              4,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 32,
        "superposition": 260
      },
      {
        "holder": 1,
        "order": 33,
        "superposition": 260
      }
    ],
    "turn_count": 42,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          2,
          3,
          4,
          3,
          0
        ],
        "clue_value": 5,
        "focus_slot": 5,
        "giver": 1,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          40,
          28,
          23,
          3,
          2
        ],
        "rlocks": false,
        "turn": 41
      }
    ]
  },
  "game_id": 2749,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "order": 5,
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
        "giver": 2,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 6,
        "max": 25,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
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
        "order": 16,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
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
        "rank": 4,
        "suit": 2,
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
        "giver": 2,
        "kind": "R",
        "list": [
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
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
        "kind": "C",
        "list": [
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
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
        "order": 18,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
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
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
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
        "kind": "C",
        "list": [
          11,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
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
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
          4,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
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
        "order": 23,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
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
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 1,
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
        "num": 17,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 21,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": 2,
        "suit": 2,
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
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 2,
        "rank": -1,
        "suit": -1,
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
        "failed": false,
        "order": 25,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 10,
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
          10,
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 2,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
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
        "giver": 1,
        "kind": "R",
        "list": [
          23
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 1,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 20,
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
        "clues": 1,
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
        "num": 1,
        "order": 29,
        "t": "strike",
        "turn": 27
      },
      {
        "failed": true,
        "order": 29,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
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
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 3,
        "suit": 0,
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
        "num": 29,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          31
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 1,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          12,
          13,
          30
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 0,
        "max": 25,
        "score": 12,
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
        "order": 33,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 25,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 34,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
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
        "order": 35,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 4,
        "suit": 0,
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
        "num": 35,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 34,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 15,
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
          10,
          37
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 1,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
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
        "order": 38,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 39,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 35,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 17,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 1,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
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
        3,
        1
      ],
      [
        4,
        2
      ],
      [
        4,
        5
      ],
      [
        1,
        3
      ],
      [
        3,
        1
      ],
      [
        1,
        2
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
        3,
        2
      ],
      null,
      [
        2,
        3
      ],
      null,
      null,
      [
        3,
        1
      ],
      [
        1,
        1
      ],
      [
        1,
        4
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
        2
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
        0,
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
        3
      ],
      [
        2,
        2
      ],
      null,
      [
        1,
        1
      ],
      [
        4,
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
        0,
        3
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
        1,
        2
      ],
      [
        0,
        4
      ],
      null,
      [
        2,
        1
      ],
      null,
      [
        0,
        2
      ]
    ],
    "names": [
      "will-bot69",
      "yagami_black",
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-28T05:06:39.465",
  "turn": 42
}
  )json";
  hanabi::Game g = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  EXPECT_GE(g.state.common_play_stacks[kPurple], 1) << "the p1 struck as a dupe at T28";
  ASSERT_FALSE(g.waiting.empty()) << "yagami's Rank 5 was unreadable before";
  EXPECT_EQ(g.common.thoughts[10].inferred, hanabi::IdentitySet::single(kB4));

  hanabi::PerformAction action = g.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "it discarded chop (o39) here before";
  EXPECT_EQ(play->target, 10);
}
