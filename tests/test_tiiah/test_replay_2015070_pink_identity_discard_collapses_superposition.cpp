// A pink identity clue names the card, and its discard then settles our hole
// (reactor0 CONVENTION.md §1c priority 0; tiiah/CONVENTION.md §1e rule 7; v19.0.0).
//
// TIIAH replay 2015070, Throw It in a Hole & Pink (6 Suits). Seats: 0 barakeel,
// 1 will-bot67 (us), 2 yagami_black.
//   T5  we threw our o5 into the hole unnamed: it was the i1, superposed {p1,i1}.
//   T6  yagami's 4 touched barakeel's o3 (the i1) as a referential discard. Its
//       pink promise narrowed o3 to {i4}; under v19.0.0 a referential discard
//       makes no pink promise.
//   T18 yagami's 1 re-touched only o3, now known pink, with pink on 0 globally:
//       a pink identity clue. o3 is the i1. It read as a STALL before.
//   T19 barakeel discarded o3. A named playable discarded was already played,
//       so our o5 was the i1, and every pink stack goes to 1.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Pink (6 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2015070, PinkIdentityDiscardCollapsesSuperposition) {
  // Reconstruct exactly the Game the live bot saw at turn 20.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2015070,
  "debug": {
    "cards_left": 35,
    "clue_tokens": 4,
    "common_play_stacks": [
      3,
      0,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 14,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 15,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 2,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 17,
        "rank": 4,
        "suit": 4
      },
      {
        "order": 3,
        "rank": 1,
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
              5,
              1
            ],
            "inferred": 1056964589,
            "info_lock": null,
            "order": 24,
            "possible": 1056964589,
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
              2
            ],
            "inferred": 15694796,
            "info_lock": null,
            "order": 21,
            "possible": 15694796,
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
            "inferred": 15694784,
            "info_lock": null,
            "order": 18,
            "possible": 15694784,
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
              2
            ],
            "inferred": 7035584,
            "info_lock": null,
            "order": 16,
            "possible": 7035584,
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
              3
            ],
            "inferred": 7035584,
            "info_lock": null,
            "order": 1,
            "possible": 7035584,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "barakeel",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 3145728,
            "info_lock": 1048576,
            "order": 22,
            "possible": 15728640,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
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
            "order": 19,
            "possible": 16777216,
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
              5
            ],
            "inferred": 16,
            "info_lock": null,
            "order": 9,
            "possible": 16,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1007174144,
            "info_lock": null,
            "order": 7,
            "possible": 1040728576,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 14680064,
            "info_lock": null,
            "order": 6,
            "possible": 15728640,
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
            "clued": false,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 14612397,
            "info_lock": null,
            "order": 23,
            "possible": 14612397,
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
              5
            ],
            "inferred": 1042352192,
            "info_lock": null,
            "order": 20,
            "possible": 1042352192,
            "slot": 2,
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
            "inferred": 69273664,
            "info_lock": null,
            "order": 13,
            "possible": 1042352192,
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
              4
            ],
            "inferred": 5678240,
            "info_lock": null,
            "order": 12,
            "possible": 14612384,
            "slot": 4,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 1042352192,
            "info_lock": null,
            "order": 11,
            "possible": 1042352192,
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
        "v": "Mistake"
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
        "v": "Discard"
      },
      {
        "k": "clue",
        "v": "Fix"
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
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "clue",
        "v": "Stall"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        0,
        0,
        1,
        0,
        0
      ],
      [
        3,
        0,
        0,
        0,
        0,
        0
      ],
      [
        3,
        0,
        0,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      0,
      0,
      1,
      0,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 5,
        "superposition": 34603008
      },
      {
        "holder": 2,
        "order": 10,
        "superposition": 33792
      },
      {
        "holder": 1,
        "order": 22,
        "support": [
          [
            4,
            2,
            1
          ],
          [
            4,
            1,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              4,
              1
            ]
          ],
          [
            [
              5,
              5,
              1
            ]
          ]
        ]
      }
    ],
    "turn_count": 20,
    "waiting": []
  },
  "game_id": 6595,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "rank": 2,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          11,
          13,
          14
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
        "giver": 2,
        "kind": "C",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 0
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
        "rank": 2,
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
        "order": 5,
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
        "giver": 2,
        "kind": "R",
        "list": [
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 4
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
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 3,
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
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 4,
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
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
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
          7,
          9,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 5
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
        "kind": "C",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
        "giver": 2,
        "kind": "C",
        "list": [
          0
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
        "cpi": 0,
        "num": 12,
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
        "order": 21,
        "p": 0,
        "rank": 2,
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
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 5,
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
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          6,
          19,
          22
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
        "score": 5,
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
          11,
          13,
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "giver": 2,
        "kind": "R",
        "list": [
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 1,
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
        "cpi": 1,
        "num": 19,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        0,
        3
      ],
      [
        3,
        3
      ],
      [
        2,
        3
      ],
      [
        5,
        1
      ],
      [
        0,
        2
      ],
      null,
      null,
      null,
      null,
      [
        0,
        5
      ],
      [
        3,
        1
      ],
      [
        5,
        3
      ],
      [
        3,
        4
      ],
      [
        5,
        2
      ],
      [
        0,
        2
      ],
      [
        1,
        4
      ],
      [
        1,
        2
      ],
      [
        4,
        4
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
        5,
        5
      ],
      [
        2,
        2
      ],
      null,
      [
        1,
        5
      ],
      [
        5,
        1
      ]
    ],
    "names": [
      "barakeel",
      "will-bot67",
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
      "variant_name": "Throw It in a Hole & Pink (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Pink (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-30T20:21:21.727",
  "turn": 20
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_FALSE(game.meta[5].superposed()) << "our o5 was the i1";
  EXPECT_EQ(game.state.common_play_stacks[5], 1)
      << "the discard is public, and so is the i1 it settles";
  EXPECT_EQ(game.state.play_stacks[5], 1);
  EXPECT_EQ(game.meta[22].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(game.common.thoughts[22].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{4, 1}))
      << "o5 was not the p1, so the call on o22 is the p1, not the p2";
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr);
  EXPECT_EQ(play->target, 22);
}
