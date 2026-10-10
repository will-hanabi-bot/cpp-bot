// Self-play TIIAH & Black (6 Suits), game 9000118, sim-alice's seat: human
// diagnostic human_vs_bot_diagnostics/9000118.md T7-T8.
// sim-cathy's T6 4 to sim-bob was a reactive with sim-alice reacting; the giver and
// the reacter read sim-alice's o3 as the b1, and that call struck b1 from sim-bob's
// hole card o8 `{r1,g1,b1,p1,k1}` -- at every seat but sim-bob's, who decodes the
// reacter's call only once she acts. At T7 sim-bob's ASCR read the b1 world he still
// allowed. A reacter's call no longer narrows the RECEIVER's hole cards (v23.15.0), so
// sim-alice's model of o8 keeps the b1, and the 2 to sim-cathy that sim-bob would
// answer with the wrong slot is not a clue she can give.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Black (6 Suits). 3 players, our_player_index=0.

TEST(TiiahSelfPlay9000118, ReacterCallSparesReceiversHole) {
  // Reconstruct exactly the Game the live bot saw at turn 7.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-alice",
  "ch": "STATE",
  "current_player_index": 0,
  "debug": {
    "cards_left": 38,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      1,
      0,
      0,
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
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 4,
            "possible": 1073740831,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 32768,
            "info_lock": 34636802,
            "order": 3,
            "possible": 1073740831,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 2,
            "possible": 1073740831,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 0,
            "possible": 1073740831,
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
              5,
              1
            ],
            "inferred": 796647159,
            "info_lock": null,
            "order": 16,
            "possible": 796647159,
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
              3
            ],
            "inferred": 762010326,
            "info_lock": null,
            "order": 9,
            "possible": 762010326,
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
              3
            ],
            "inferred": 762010326,
            "info_lock": null,
            "order": 7,
            "possible": 762010326,
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
              4
            ],
            "inferred": 277094664,
            "info_lock": null,
            "order": 6,
            "possible": 277094664,
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
            "inferred": 762010326,
            "info_lock": null,
            "order": 5,
            "possible": 762010326,
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
              2,
              5
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 14,
            "possible": 935194491,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              3
            ],
            "inferred": 138547332,
            "info_lock": null,
            "order": 13,
            "possible": 138547332,
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
              4
            ],
            "inferred": 11906411,
            "info_lock": null,
            "order": 12,
            "possible": 935194491,
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
              5
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 11,
            "possible": 935194491,
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
              2
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 10,
            "possible": 935194491,
            "slot": 5,
            "status": "NONE",
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
        "v": "Play"
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
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        1,
        0,
        0,
        0,
        0
      ],
      [
        0,
        1,
        0,
        0,
        0,
        0
      ],
      [
        1,
        1,
        0,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          1,
          0,
          0,
          0,
          0
        ],
        "focus_slot": 4,
        "giver": 2,
        "reacter": 0,
        "receiver": 1,
        "receiver_hand": [
          16,
          9,
          7,
          6,
          5
        ],
        "turn": 6
      }
    ],
    "play_stacks": [
      1,
      1,
      0,
      0,
      0,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 8,
        "superposition": 34604033
      }
    ],
    "turn_count": 7,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          0,
          1,
          0,
          0,
          0,
          0
        ],
        "clue_value": 4,
        "focus_slot": 4,
        "giver": 2,
        "inverted": false,
        "react_order": 3,
        "reacter": 0,
        "receiver": 1,
        "receiver_hand": [
          16,
          9,
          7,
          6,
          5
        ],
        "rlocks": false,
        "turn": 6
      }
    ]
  },
  "game_id": 9000118,
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
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          8
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
          13
        ],
        "t": "clue",
        "target": 2,
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
        "giver": 2,
        "kind": "C",
        "list": [
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 1,
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
        "order": 8,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 5,
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
        "num": 5,
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
        "value": 4
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      null,
      [
        1,
        1
      ],
      null,
      null,
      null,
      [
        0,
        2
      ],
      [
        4,
        4
      ],
      [
        4,
        3
      ],
      [
        0,
        1
      ],
      [
        0,
        3
      ],
      [
        3,
        2
      ],
      [
        3,
        5
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
        2,
        5
      ],
      null,
      [
        5,
        1
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
      "variant_name": "Throw It in a Hole & Black (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Black (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-10T00:51:02.626",
  "turn": 7
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::TIIAH);
  ASSERT_TRUE(game.meta[8].superposed());
  EXPECT_TRUE(game.meta[8].superposition.contains(hanabi::Identity(3, 1)))
      << "sim-bob's o8 may still be the b1";
  EXPECT_EQ(game.meta[3].status, hanabi::CardStatus::CALLED_TO_PLAY) << "the reacter's b1";
  hanabi::PerformAction action = game.take_action();
  EXPECT_FALSE(std::holds_alternative<hanabi::PerformRank>(action) &&
               std::get<hanabi::PerformRank>(action).target == 2 &&
               std::get<hanabi::PerformRank>(action).value == 2)
      << "not the 2 to sim-cathy";
}
