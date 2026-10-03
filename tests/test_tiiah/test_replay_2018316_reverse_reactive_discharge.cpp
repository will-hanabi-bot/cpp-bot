// The dupe discharge, read from its rule (tiiah/CONVENTION.md §1k; v20.1.0).
//
// TIIAH replay 2018316, Throw It in a Hole & Pink (6 Suits). Seats: 0 yagami_green
// (us), 1 yagami_black (human), 2 yagami_blue.
//   T2 black played its o9, the i1, into the hole unknown: {p1, i1}.
//   T8 black's 5 to blue (loaded for its b2) is a reverse reactive: our slot 4
//      (o2) into blue's slot 1 (o17, the i2). On black's frame pink is 0, so it is
//      a finesse naming o2 as the i1 -- which we watched land. We read the bucket's
//      {b3} on our own stacks and played the i1 into a strike. It is the i1, and
//      we throw it.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Pink (6 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay2018316, ReverseReactiveDischarge) {
  // Reconstruct exactly the Game the live bot saw at turn 10.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2018316,
  "debug": {
    "cards_left": 40,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      0,
      1,
      2,
      0,
      0
    ],
    "current_player_index": 0,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 18,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 16,
            "possible": 1073741823,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073710079,
            "info_lock": null,
            "order": 3,
            "possible": 1073710079,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 131072,
            "info_lock": 68354113,
            "order": 2,
            "possible": 1073710079,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073710079,
            "info_lock": null,
            "order": 1,
            "possible": 1073710079,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_green",
        "player": 0
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 15,
            "possible": 1073741823,
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
              3
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 8,
            "possible": 1073741823,
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 7,
            "possible": 1073741823,
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 6,
            "possible": 1073741823,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              0,
              1
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 5,
            "possible": 1073741823,
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
            "id": [
              2,
              1
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 19,
            "possible": 1073741823,
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
              2
            ],
            "inferred": 1057505808,
            "info_lock": null,
            "order": 17,
            "possible": 1057505808,
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
              4
            ],
            "inferred": 262144,
            "info_lock": null,
            "order": 14,
            "possible": 262144,
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
              4
            ],
            "inferred": 8397064,
            "info_lock": null,
            "order": 13,
            "possible": 8397064,
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
            "inferred": 7347431,
            "info_lock": null,
            "order": 10,
            "possible": 7347431,
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
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        0,
        1,
        2,
        0,
        0
      ],
      [
        0,
        1,
        1,
        2,
        0,
        0
      ],
      [
        0,
        0,
        1,
        2,
        0,
        1
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          1,
          1,
          0,
          0
        ],
        "focus_slot": 5,
        "giver": 1,
        "reacter": 0,
        "receiver": 2,
        "receiver_hand": [
          17,
          14,
          13,
          11,
          10
        ],
        "turn": 8
      }
    ],
    "play_stacks": [
      0,
      1,
      1,
      2,
      0,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 9,
        "superposition": 34603008
      },
      {
        "holder": 2,
        "order": 11,
        "support": [
          [
            0,
            2,
            5
          ],
          [
            1,
            1,
            5
          ],
          [
            0,
            1,
            10
          ],
          [
            1,
            2,
            10
          ]
        ],
        "worlds": [
          [
            [
              9,
              4,
              1
            ],
            [
              12,
              0,
              1
            ]
          ],
          [
            [
              9,
              4,
              1
            ],
            [
              12,
              1,
              1
            ]
          ],
          [
            [
              9,
              5,
              1
            ],
            [
              12,
              0,
              1
            ]
          ],
          [
            [
              9,
              5,
              1
            ],
            [
              12,
              1,
              1
            ]
          ]
        ]
      },
      {
        "holder": 2,
        "order": 12,
        "superposition": 33
      }
    ],
    "turn_count": 10,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          0,
          0,
          1,
          1,
          0,
          0
        ],
        "clue_value": 5,
        "focus_slot": 5,
        "giver": 1,
        "inverted": false,
        "react_order": 2,
        "reacter": 0,
        "receiver": 2,
        "receiver_hand": [
          17,
          14,
          13,
          11,
          10
        ],
        "rlocks": false,
        "turn": 8
      }
    ]
  },
  "game_id": 10544,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 5,
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
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 2,
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
          13,
          14
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
        "order": 9,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 0,
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
        "kind": "C",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 4,
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
        "giver": 1,
        "kind": "C",
        "list": [
          11,
          14
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
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 2,
        "suit": 5,
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
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
        "kind": "R",
        "list": [
          17
        ],
        "t": "clue",
        "target": 2,
        "value": 5
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
        "order": 11,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
      null,
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
        4,
        4
      ],
      [
        3,
        5
      ],
      [
        5,
        3
      ],
      [
        5,
        1
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
        1,
        1
      ],
      [
        2,
        4
      ],
      [
        3,
        4
      ],
      [
        0,
        1
      ],
      null,
      [
        5,
        2
      ],
      null,
      [
        2,
        1
      ]
    ],
    "names": [
      "yagami_green",
      "yagami_black",
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
      "variant_name": "Throw It in a Hole & Pink (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Pink (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T18:20:13.117",
  "turn": 10
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_EQ(game.meta[2].status, hanabi::CardStatus::CALLED_TO_DISCARD);
  EXPECT_EQ(game.common.thoughts[2].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{5, 1}))
      << "the finesse's connector, the i1 black played -- not the bucket's {b3}";
  const auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  ASSERT_NE(discard, nullptr) << "the i1 is already down: discharge it";
  EXPECT_EQ(discard->target, 2);
}
