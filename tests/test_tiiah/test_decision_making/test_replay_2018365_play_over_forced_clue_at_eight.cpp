// Human diagnostic 2018365 T10 (v18_human_vs_bot_diagnostics/2018365.md). At 8
// tokens green, still holding the called y1, gave Blue on black's already-clued b3
// (rung 4), a clue that saved nothing while black's chop was a same-hand dupe. At 8
// tokens section 4 opens only when Alice has no known play, so green plays the y1.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Null (6 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay2018365, PlayOverForcedClueAtEight) {
  // Reconstruct exactly the Game the live bot saw at turn 10.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2018365,
  "debug": {
    "cards_left": 40,
    "clue_tokens": 8,
    "common_play_stacks": [
      0,
      0,
      0,
      0,
      1,
      0
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 7,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 4,
        "rank": 3,
        "suit": 4
      },
      {
        "order": 5,
        "rank": 1,
        "suit": 5
      },
      {
        "order": 15,
        "rank": 4,
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
            "id": null,
            "inferred": 1073740831,
            "info_lock": null,
            "order": 16,
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
            "order": 3,
            "possible": 1073740831,
            "slot": 2,
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
            "order": 2,
            "possible": 1073740831,
            "slot": 3,
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
            "order": 1,
            "possible": 1073740831,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 32,
            "info_lock": 32,
            "order": 0,
            "possible": 992,
            "slot": 5,
            "status": "CALLED_TO_PLAY",
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
              4
            ],
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
            "id": [
              1,
              4
            ],
            "inferred": 1071577021,
            "info_lock": null,
            "order": 17,
            "possible": 1071577021,
            "slot": 2,
            "status": "CHOP_MOVED",
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
            "inferred": 1067247417,
            "info_lock": null,
            "order": 9,
            "possible": 1067247417,
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
              2
            ],
            "inferred": 2164802,
            "info_lock": null,
            "order": 8,
            "possible": 2164802,
            "slot": 4,
            "status": "CHOP_MOVED",
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
            "inferred": 4329604,
            "info_lock": null,
            "order": 6,
            "possible": 4329604,
            "slot": 5,
            "status": "CHOP_MOVED",
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
              3,
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
            "clued": false,
            "focused": false,
            "id": [
              2,
              4
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 14,
            "possible": 1041235967,
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
              2
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 13,
            "possible": 1041235967,
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
              5
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 11,
            "possible": 1041235967,
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
              5
            ],
            "inferred": 1041235967,
            "info_lock": null,
            "order": 10,
            "possible": 1041235967,
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Lock"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        0,
        0,
        0,
        0,
        0
      ],
      [
        0,
        0,
        0,
        0,
        1,
        0
      ],
      [
        0,
        0,
        0,
        0,
        1,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      0,
      0,
      0,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 10,
    "waiting": []
  },
  "game_id": 10601,
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
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
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
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          6,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 3
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
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
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
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 0,
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
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 1,
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
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 7,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
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
        "value": 2
      },
      {
        "clues": 6,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 4,
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
        "num": 8,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 1,
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
      null,
      null,
      null,
      null,
      [
        4,
        3
      ],
      [
        5,
        1
      ],
      [
        3,
        3
      ],
      [
        0,
        3
      ],
      [
        0,
        2
      ],
      [
        0,
        4
      ],
      [
        0,
        5
      ],
      [
        2,
        5
      ],
      [
        4,
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
        5,
        4
      ],
      null,
      [
        1,
        4
      ],
      [
        0,
        4
      ],
      [
        3,
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
      "variant_name": "Throw It in a Hole & Null (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T18:56:03.941",
  "turn": 10
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_TRUE(play) << "the standing y1, not a section-4 clue at 8 tokens";
  EXPECT_EQ(play->target, 0) << "o0, the called y1";
}
