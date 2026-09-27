// A named playable that a partner DISCARDS was already played -- by us, into the
// hole (tiiah/CONVENTION.md 1e rule 7, v16.22.0). There is no gentleman's discard
// in Throw It in a Hole.
//
// TIIAH replay 2011319. will-bot69 threw order 5 (a p1) and order 9 (a p2) into
// the hole without naming them: order 5 read {g1,b1,p1}, order 9 {r2,p2}. At T8
// our Purple clue called yagami's order 14 as p1. At T12 yagami threw it away --
// correctly, she could see the p1 and p2 in the hole. Read against purple 0, the
// p1 looked playable, so the shared gentleman's-discard reading found no p1 in
// any hand, fell back on "then it is in mine", and pinned our only unknown card,
// order 16, to {p1}. At T14 we played it: a b2, a strike.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/card.h"

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2011319, DiscardedPlayableWasAlreadyPlayed) {
  // Reconstruct exactly the Game the live bot saw at turn 14.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2011319,
  "debug": {
    "cards_left": 28,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      1,
      0,
      0,
      0
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 3,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 14,
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
              2,
              4
            ],
            "inferred": 33423359,
            "info_lock": null,
            "order": 19,
            "possible": 33423359,
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
            "inferred": 32538623,
            "info_lock": null,
            "order": 18,
            "possible": 32538623,
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
              3
            ],
            "inferred": 4198532,
            "info_lock": null,
            "order": 2,
            "possible": 4198532,
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
              2
            ],
            "inferred": 28340091,
            "info_lock": null,
            "order": 1,
            "possible": 28340091,
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
            "order": 0,
            "possible": 131072,
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
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31,
            "info_lock": null,
            "order": 20,
            "possible": 31,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1048576,
            "info_lock": null,
            "order": 16,
            "possible": 33423328,
            "slot": 2,
            "status": "GENTLEMANS_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 960,
            "info_lock": null,
            "order": 8,
            "possible": 960,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32339968,
            "info_lock": null,
            "order": 7,
            "possible": 32339968,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32339968,
            "info_lock": null,
            "order": 6,
            "possible": 32339968,
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
            "inferred": 33423359,
            "info_lock": null,
            "order": 21,
            "possible": 33423359,
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
            "inferred": 917503,
            "info_lock": null,
            "order": 17,
            "possible": 917503,
            "slot": 2,
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
            "inferred": 917503,
            "info_lock": null,
            "order": 12,
            "possible": 917503,
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
            "inferred": 32505856,
            "info_lock": null,
            "order": 11,
            "possible": 32505856,
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
              2
            ],
            "inferred": 917503,
            "info_lock": null,
            "order": 10,
            "possible": 917503,
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
      5
    ],
    "move_history": [
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
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Fix"
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
        "v": "GentlemansDiscard"
      },
      {
        "k": "clue",
        "v": "Reactive"
      }
    ],
    "pairwise_play_stacks": [
      [
        1,
        1,
        0,
        0,
        0
      ],
      [
        1,
        2,
        0,
        0,
        0
      ],
      [
        1,
        2,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          1,
          1,
          0,
          0,
          0
        ],
        "focus_slot": 1,
        "giver": 0,
        "reacter": 2,
        "receiver": 1,
        "receiver_hand": [
          20,
          16,
          8,
          7,
          6
        ],
        "turn": 13
      }
    ],
    "play_stacks": [
      1,
      2,
      0,
      0,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 4,
        "superposition": 33856
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 1082368
      },
      {
        "holder": 1,
        "order": 9,
        "superposition": 2097154,
        "support": [
          [
            4,
            1,
            3
          ],
          [
            4,
            2,
            4
          ]
        ],
        "worlds": [
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
        "holder": 2,
        "order": 15,
        "superposition": 65
      }
    ],
    "turn_count": 14,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_play_stacks": [
          1,
          1,
          0,
          0,
          0
        ],
        "clue_value": 0,
        "focus_slot": 1,
        "giver": 0,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 1,
        "receiver_hand": [
          20,
          16,
          8,
          7,
          6
        ],
        "rlocks": false,
        "turn": 13
      }
    ]
  },
  "game_id": 2040,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 4,
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
        "rank": 3,
        "suit": 3,
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
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          0,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
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
        "giver": 0,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 1,
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
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "order": 15,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 4,
        "suit": 1,
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
        "order": 4,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 2,
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
        "kind": "C",
        "list": [
          11,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
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
        "order": 9,
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
        "clues": 4,
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
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "giver": 0,
        "kind": "C",
        "list": [
          20
        ],
        "t": "clue",
        "target": 1,
        "value": 0
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        3,
        3
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
        3,
        3
      ],
      [
        1,
        2
      ],
      null,
      null,
      null,
      null,
      null,
      [
        0,
        2
      ],
      [
        4,
        5
      ],
      [
        1,
        2
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
        0,
        1
      ],
      null,
      [
        1,
        4
      ],
      [
        2,
        2
      ],
      [
        2,
        4
      ],
      null,
      [
        1,
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-27T19:10:50.642",
  "turn": 14
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  // The discard settles our hole: only the world where order 5 was the p1 has the
  // p1 already played, and then order 9 lands only as the p2.
  EXPECT_FALSE(game.meta[5].superposed()) << "order 5 was the p1 yagami could see";
  EXPECT_FALSE(game.meta[9].superposed()) << "and order 9 the p2 on top of it";
  EXPECT_EQ(game.state.play_stacks[4], 2) << "our purple is on 2";
  EXPECT_EQ(game.state.common_play_stacks[4], 2)
      << "shared: the discard is public and the p1 was common knowledge";

  // No gentleman's discard: order 16 learned nothing from the p1 going.
  EXPECT_NE(game.meta[16].status, hanabi::CardStatus::GENTLEMANS_DISCARD);
  EXPECT_FALSE(game.common.thoughts[16].inferred ==
               hanabi::IdentitySet::single(hanabi::Identity{4, 1}));

  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr);
  EXPECT_NE(play->target, 16) << "order 16 was a b2; it struck here before";
  EXPECT_EQ(play->target, 20) << "the Red clue's r2, on red 1";
}
