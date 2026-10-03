// Replay 2018535 T5-T8 (tiiah/CONVENTION.md §1d, the receiver world fallback,
// v20.5.0). will-bot67 threw o9 `{r2,ra2}` into the hole unnamed at T5 (it was the
// m2). yagami's T6 Green to will-bot67 was a reactive: will-bot69's T7 reaction
// into will-bot67's o8. On the frame the giver and will-bot67 share rainbow is on 1,
// so the m3 is one away and the shared stamp read a bluff. In the world where o9 was
// the m2 it plays: o8 is the m3, called, and the hole collapses. Step 1 reads it,
// before any other one-away: the reacter's y1 names the rainbow bucket (a colour
// clue, one bucket down), and the m3 plays through the m2 the hole card may have
// been -- the user's "finesse path". At T8 will-bot67 still gives a stable play to yagami's
// chop (rung 3.1, HIGH), which the decision ladder ranks above its call; what this
// pins is the reading.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2018535, ReceiverFinesseThroughAHoleCard) {
  // Reconstruct exactly the Game the live bot saw at turn 8.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2018535,
  "debug": {
    "cards_left": 31,
    "clue_tokens": 5,
    "common_play_stacks": [
      0,
      0,
      0,
      0,
      1
    ],
    "current_player_index": 1,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 18,
            "possible": 33554431,
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
              2
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 16,
            "possible": 33554431,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 4,
            "possible": 33554431,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 3,
            "possible": 33554431,
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
              2
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 2,
            "possible": 33554431,
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
            "id": null,
            "inferred": 1016831,
            "info_lock": null,
            "order": 17,
            "possible": 1016831,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 918428,
            "info_lock": null,
            "order": 15,
            "possible": 951229,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4194304,
            "info_lock": null,
            "order": 8,
            "possible": 29388800,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 29388800,
            "info_lock": null,
            "order": 7,
            "possible": 29388800,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 29388800,
            "info_lock": null,
            "order": 6,
            "possible": 29388800,
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
              4
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 14,
            "possible": 33554431,
            "slot": 1,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 13,
            "possible": 33554431,
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
              5
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 12,
            "possible": 33554431,
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
              3
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 11,
            "possible": 33554431,
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
              3
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 10,
            "possible": 33554431,
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
        0,
        0,
        1
      ],
      [
        0,
        0,
        0,
        0,
        1
      ],
      [
        0,
        1,
        0,
        0,
        1
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      1,
      0,
      0,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 1,
        "superposition": 33
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 33793
      },
      {
        "holder": 1,
        "order": 9,
        "superposition": 2097154,
        "support": [
          [
            0,
            2,
            1
          ],
          [
            1,
            1,
            7
          ],
          [
            0,
            1,
            6
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
          ]
        ]
      }
    ],
    "turn_count": 8,
    "waiting": []
  },
  "game_id": 10817,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "rank": 1,
        "suit": 2,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 1,
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
        "giver": 2,
        "kind": "R",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "rank": 2,
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
        "order": 9,
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
        "giver": 2,
        "kind": "C",
        "list": [
          6,
          7,
          8
        ],
        "t": "clue",
        "target": 1,
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
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 4,
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        4,
        1
      ],
      [
        1,
        1
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
        2,
        1
      ],
      null,
      null,
      null,
      null,
      null,
      [
        0,
        3
      ],
      [
        3,
        3
      ],
      [
        1,
        5
      ],
      [
        3,
        1
      ],
      [
        1,
        4
      ],
      null,
      [
        1,
        2
      ],
      null,
      [
        4,
        2
      ]
    ],
    "names": [
      "will-bot69",
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
      "variant_name": "Throw It in a Hole & Rainbow (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T22:40:21.132",
  "turn": 8
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_EQ(game.common.thoughts[8].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{4, 3}))
      << "o8 is the m3";
  EXPECT_EQ(game.meta[8].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(game.state.play_stacks[4], 2) << "o9 was the m2";
  EXPECT_EQ(game.state.common_play_stacks[4], 2) << "for every seat";
  (void)action;
}
