// TIIAH replay 2013726 (reactor0/DECISION_MAKING.md rung 2b; v18.8.0). Throw It in
// a Hole & Brown (4 Suits). Seats: 0 yagami_black (human), 1 yagami_green (us),
// 2 yagami_blue.
//
// Human diagnostic v18_human_vs_bot_diagnostics/2013726.md T5, as corrected by the
// reviewer: "the fix does need to be given, and in that situation 2 is actually the
// better clue to give." Blue's o14 is called as `{r1,b2}` and is the r1, already
// down; without a fix blue plays it into a strike. v18.6.0 preferred a colour fix
// (Blue) and was reverted in v18.8.0; this pins the 2.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (4 Suits). 3 players, our_player_index=1.

TEST(TiiahFixDeadCall2013726, TwoFixesBluesDeadCall) {
  // Reconstruct exactly the Game the live bot saw at turn 5.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2013726,
  "debug": {
    "cards_left": 23,
    "clue_tokens": 6,
    "common_play_stacks": [
      0,
      0,
      1,
      0
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
              1,
              4
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 16,
            "possible": 1048575,
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
              4
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 4,
            "possible": 1048575,
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
              4
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 3,
            "possible": 1048575,
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
              3
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 2,
            "possible": 1048575,
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
              1
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 1,
            "possible": 1048575,
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
            "id": null,
            "inferred": 1044347,
            "info_lock": null,
            "order": 15,
            "possible": 1044347,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1044347,
            "info_lock": null,
            "order": 9,
            "possible": 1044347,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 2050,
            "info_lock": null,
            "order": 7,
            "possible": 1044347,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4228,
            "info_lock": null,
            "order": 6,
            "possible": 4228,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1044347,
            "info_lock": null,
            "order": 5,
            "possible": 1044347,
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
            "focused": true,
            "id": [
              0,
              1
            ],
            "inferred": 2049,
            "info_lock": null,
            "order": 14,
            "possible": 1047583,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 1047583,
            "info_lock": null,
            "order": 13,
            "possible": 1047583,
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
              1
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 12,
            "possible": 1047583,
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
              2
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 11,
            "possible": 1047583,
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
              5
            ],
            "inferred": 992,
            "info_lock": null,
            "order": 10,
            "possible": 992,
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
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        0,
        1,
        0
      ],
      [
        0,
        0,
        1,
        0
      ],
      [
        1,
        0,
        1,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      0,
      1,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 0,
        "order": 0,
        "superposition": 33
      }
    ],
    "turn_count": 5,
    "waiting": []
  },
  "game_id": 5051,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 3,
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
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
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
        "value": 1
      },
      {
        "clues": 7,
        "max": 20,
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
        "max": 20,
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
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 6,
        "max": 20,
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
        "max": 20,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
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
        3
      ],
      [
        1,
        4
      ],
      [
        3,
        4
      ],
      null,
      null,
      null,
      [
        2,
        1
      ],
      null,
      [
        1,
        5
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
        3,
        5
      ],
      [
        0,
        1
      ],
      null,
      [
        1,
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
      "variant_name": "Throw It in a Hole & Brown (4 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Brown (4 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-29T19:26:34.343",
  "turn": 5
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // Blue's o14 is called to play as `{r1,b2}` and is the r1, already down on the
  // stacks green and blue share: a dead call, and a fix is on.
  ASSERT_EQ(game.meta[14].status, hanabi::CardStatus::CALLED_TO_PLAY);

  // The fix is given, and it is the 2.
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "the rank 2 fix";
  EXPECT_EQ(rank->target, 2) << "to blue";
  EXPECT_EQ(rank->value, 2);
}
