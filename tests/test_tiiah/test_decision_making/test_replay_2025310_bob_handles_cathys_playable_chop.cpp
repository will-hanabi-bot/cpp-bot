// Human diagnostic human_vs_bot_diagnostics/2025310.md T3 (v23.11.0, the user's
// ruling), yagami_green's seat. Green holds its called y1 (o12); blue's chop is the
// y2, a good card that plays once green's y1 lands. Black, green's Bob, is expected
// to deal with it rather than throw his own chop, so green does not stop to give
// black a play clue (rung 3, whose high-clue-count arm fired at 7 tokens): it plays
// the y1.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Dark Null (6 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2025310, BobHandlesCathysPlayableChop) {
  // Reconstruct exactly the Game the live bot saw at turn 3.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2025310,
  "debug": {
    "cards_left": 39,
    "clue_tokens": 7,
    "common_play_stacks": [
      0,
      0,
      1,
      0,
      0,
      0
    ],
    "current_player_index": 2,
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 4,
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
              2,
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 3,
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
              4,
              5
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 2,
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
              0,
              1
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 1,
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
              2,
              2
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 0,
            "possible": 1073741823,
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
              2
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
              3,
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 9,
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
              4,
              3
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 8,
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
              1,
              3
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 7,
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
              4,
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
        "name": "yagami_blue",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 14,
            "possible": 1073740831,
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
            "order": 13,
            "possible": 1073740831,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 32,
            "info_lock": null,
            "order": 12,
            "possible": 992,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 11,
            "possible": 1073740831,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
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
        0,
        0,
        0
      ],
      [
        0,
        0,
        1,
        0,
        0,
        0
      ],
      [
        0,
        0,
        1,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      0,
      1,
      0,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 3,
    "waiting": []
  },
  "game_id": 3649,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 4,
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
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
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
        "kind": "C",
        "list": [
          10,
          12
        ],
        "t": "clue",
        "target": 2,
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
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 2,
        "suit": 1,
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        2,
        2
      ],
      [
        0,
        1
      ],
      [
        4,
        5
      ],
      [
        2,
        4
      ],
      [
        4,
        2
      ],
      [
        4,
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
        4,
        3
      ],
      [
        3,
        4
      ],
      null,
      null,
      null,
      null,
      null,
      [
        1,
        2
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
      "variant_name": "Throw It in a Hole & Dark Null (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Dark Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-09T19:19:12.168",
  "turn": 3
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action)) << "not the Red to black";
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 12) << "the known y1";
}
