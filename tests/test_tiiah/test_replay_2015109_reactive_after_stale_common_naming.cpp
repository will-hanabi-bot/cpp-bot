// A watched hole play is named by sight, not by a stale common reading
// (tiiah/CONVENTION.md §1e; v19.1.0).
//
// TIIAH replay 2015109, Throw It in a Hole & White (5 Suits). Seats: 0 yagami_black,
// 1 barakeel, 2 will-bot67 (us).
//   T6, T9  our own y1 and y2 went into the hole unnamed: common yellow stays 0.
//   T28 yagami's Yellow called barakeel's o31. The pair had watched our y1 and y2,
//       so it was the y3; our common copy, read on yellow 0, said {y1}.
//   T29 barakeel played o31. We could see the y3, but the shared view booked a y1,
//       and the y3 vanished from every shared world.
//   T43 yagami's y4 could not be playable in any world, settled as the w4: white
//       went to 4 in common, against a true 3.
//   T47 barakeel's 1 to yagami is a reactive clue pairing her w4 (slot 2) with our
//       b3 (slot 4, o17). On white 4 the w4 looked like trash and the clue read
//       as a MISTAKE; we gave Blue instead of reacting.

#include <gtest/gtest.h>


#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & White (5 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2015109, ReactiveAfterStaleCommonNaming) {
  // Reconstruct exactly the Game the live bot saw at turn 48.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2015109,
  "debug": {
    "cards_left": 3,
    "clue_tokens": 5,
    "common_play_stacks": [
      5,
      5,
      3,
      1,
      4
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 34,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 28,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 23,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 32,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 27,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 30,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 33,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 4,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 15,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 29,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 25,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 42,
        "rank": 1,
        "suit": 4
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
              4,
              1
            ],
            "inferred": 1082400,
            "info_lock": null,
            "order": 46,
            "possible": 1082400,
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
            "inferred": 32454720,
            "info_lock": null,
            "order": 43,
            "possible": 32454720,
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
              2
            ],
            "inferred": 2164800,
            "info_lock": null,
            "order": 37,
            "possible": 2164800,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              3,
              1
            ],
            "inferred": 1082400,
            "info_lock": null,
            "order": 21,
            "possible": 1082400,
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
              4
            ],
            "inferred": 8396800,
            "info_lock": null,
            "order": 1,
            "possible": 8396800,
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
              1,
              1
            ],
            "inferred": 33537120,
            "info_lock": null,
            "order": 44,
            "possible": 33537120,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              5
            ],
            "inferred": 17301504,
            "info_lock": null,
            "order": 38,
            "possible": 17301504,
            "slot": 2,
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
            "inferred": 11705344,
            "info_lock": null,
            "order": 26,
            "possible": 16235520,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
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
            "inferred": 12988416,
            "info_lock": null,
            "order": 9,
            "possible": 12988416,
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
              3
            ],
            "inferred": 12988416,
            "info_lock": null,
            "order": 7,
            "possible": 12988416,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "barakeel",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33537120,
            "info_lock": null,
            "order": 45,
            "possible": 33537120,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33537120,
            "info_lock": null,
            "order": 39,
            "possible": 33537120,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33521760,
            "info_lock": null,
            "order": 19,
            "possible": 33521760,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33521760,
            "info_lock": null,
            "order": 17,
            "possible": 33521760,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33521760,
            "info_lock": null,
            "order": 10,
            "possible": 33521760,
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
        "v": "Discard"
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
        "v": "Mistake"
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
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Mistake"
      }
    ],
    "pairwise_play_stacks": [
      [
        5,
        5,
        4,
        1,
        4
      ],
      [
        5,
        5,
        5,
        2,
        4
      ],
      [
        5,
        5,
        3,
        1,
        4
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          5,
          5,
          3,
          1,
          4
        ],
        "focus_slot": 1,
        "giver": 1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          46,
          43,
          37,
          21,
          1
        ],
        "turn": 47
      }
    ],
    "play_stacks": [
      5,
      5,
      5,
      2,
      3
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 0,
        "superposition": 3138,
        "support": [
          [
            2,
            1,
            11
          ],
          [
            2,
            2,
            4
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
              2,
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
        "order": 2,
        "superposition": 2099266
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 1049633
      },
      {
        "holder": 1,
        "order": 8,
        "support": [
          [
            4,
            1,
            16777215
          ],
          [
            4,
            2,
            1996488704
          ],
          [
            4,
            3,
            2281701376
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              1,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              2,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              3,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              4,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              1,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              2,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              3,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              4,
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
              2
            ]
          ]
        ]
      },
      {
        "holder": 0,
        "order": 16,
        "superposition": 264
      },
      {
        "holder": 0,
        "order": 20,
        "superposition": 17317888
      },
      {
        "holder": 1,
        "order": 22,
        "superposition": 5,
        "support": [
          [
            0,
            3,
            1
          ],
          [
            0,
            2,
            2
          ],
          [
            0,
            1,
            60
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
              2,
              0,
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
              0,
              2,
              1
            ],
            [
              2,
              2,
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
              0,
              2,
              1
            ],
            [
              2,
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
              0,
              2,
              1
            ],
            [
              2,
              2,
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
              0,
              2,
              1
            ],
            [
              2,
              2,
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
              0,
              2,
              1
            ],
            [
              2,
              4,
              2
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 24,
        "superposition": 2164800
      },
      {
        "holder": 0,
        "order": 35,
        "superposition": 2164800
      },
      {
        "holder": 1,
        "order": 36,
        "superposition": 204800,
        "support": [
          [
            3,
            2,
            3149642683
          ],
          [
            3,
            3,
            1145324612
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              1,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              2,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              3,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              1
            ],
            [
              24,
              4,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              1,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              2,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              3,
              2
            ]
          ],
          [
            [
              5,
              2,
              1
            ],
            [
              22,
              0,
              3
            ],
            [
              24,
              4,
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
              22,
              0,
              1
            ],
            [
              24,
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
              22,
              0,
              1
            ],
            [
              24,
              2,
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
              22,
              0,
              1
            ],
            [
              24,
              3,
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
              22,
              0,
              1
            ],
            [
              24,
              4,
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
              22,
              0,
              3
            ],
            [
              24,
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
              22,
              0,
              3
            ],
            [
              24,
              2,
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
              22,
              0,
              3
            ],
            [
              24,
              3,
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
              22,
              0,
              3
            ],
            [
              24,
              4,
              2
            ]
          ]
        ]
      }
    ],
    "turn_count": 48,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          5,
          5,
          3,
          1,
          4
        ],
        "clue_value": 1,
        "focus_slot": 1,
        "giver": 1,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          46,
          43,
          37,
          21,
          1
        ],
        "rlocks": false,
        "turn": 47
      }
    ]
  },
  "game_id": 6648,
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
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "suit": 4,
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
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 0,
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
        "rank": 4,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 4,
        "suit": 0,
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
        "giver": 1,
        "kind": "R",
        "list": [
          1,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 5,
        "max": 25,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
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
        "order": 17,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
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
        "giver": 1,
        "kind": "R",
        "list": [
          2,
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
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
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
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
        "kind": "R",
        "list": [
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 2,
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
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 1,
        "rank": 1,
        "suit": 0,
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
        "order": 23,
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
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          8,
          22
        ],
        "t": "clue",
        "target": 1,
        "value": 0
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
        "order": 22,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
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
        "num": 17,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 0,
        "kind": "R",
        "list": [
          24
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 8,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 9,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 2,
        "max": 25,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
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
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "order": 8,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "failed": false,
        "order": 25,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "failed": false,
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "failed": false,
        "order": 28,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "failed": false,
        "order": 29,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 11,
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
          6,
          31
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
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
        "order": 33,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "failed": false,
        "order": 32,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "failed": false,
        "order": 30,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 35,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "failed": false,
        "order": 33,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 25,
        "score": 12,
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
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 7,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
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
        "order": 37,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 36,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 38,
        "p": 1,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 14,
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
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 25,
        "score": 14,
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
          6,
          38
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 7,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
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
        "value": 0
      },
      {
        "clues": 6,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
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
        "order": 40,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
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
        "order": 41,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 16,
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
          35,
          37
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "order": 40,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 42,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "order": 41,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 4,
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
        "cpi": 1,
        "num": 43,
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
        "order": 44,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
        "failed": false,
        "order": 42,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 45,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "order": 35,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 46,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          21,
          46
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
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
        4,
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
        4,
        1
      ],
      [
        1,
        5
      ],
      [
        2,
        3
      ],
      [
        0,
        3
      ],
      [
        3,
        4
      ],
      null,
      null,
      [
        0,
        5
      ],
      null,
      [
        2,
        3
      ],
      [
        1,
        4
      ],
      [
        0,
        4
      ],
      null,
      [
        0,
        2
      ],
      null,
      [
        2,
        5
      ],
      [
        3,
        1
      ],
      [
        0,
        1
      ],
      [
        0,
        2
      ],
      [
        4,
        2
      ],
      [
        3,
        4
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
        0,
        1
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
        1,
        3
      ],
      [
        0,
        3
      ],
      [
        1,
        2
      ],
      [
        0,
        1
      ],
      [
        3,
        2
      ],
      [
        2,
        4
      ],
      [
        2,
        2
      ],
      [
        3,
        5
      ],
      null,
      [
        4,
        3
      ],
      [
        1,
        4
      ],
      [
        4,
        1
      ],
      [
        4,
        4
      ],
      [
        1,
        1
      ],
      null,
      [
        4,
        1
      ]
    ],
    "names": [
      "yagami_black",
      "barakeel",
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
      "variant_name": "Throw It in a Hole & White (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & White (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-30T21:08:22.092",
  "turn": 48
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  // State only: three cards are left, so `take_action` runs the endgame solver to
  // its 6 s budget. The urgent call on o17 is what makes the turn play it
  // (`replay_log --rerun` at this turn: play(order=17)).
  EXPECT_EQ(game.state.common_play_stacks[1], 5)
      << "the y3 barakeel played at T29 is booked as the y3, so the y4 and y5 land";
  EXPECT_EQ(game.state.common_play_stacks[4], 3)
      << "white is on 3: yagami's T43 play was the y4, not the w4";
  EXPECT_EQ(game.meta[17].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(game.common.thoughts[17].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{3, 3}))
      << "o17 is the reaction, the b3";
  EXPECT_TRUE(game.meta[17].urgent) << "the reaction barakeel's 1 asked of us";
}
