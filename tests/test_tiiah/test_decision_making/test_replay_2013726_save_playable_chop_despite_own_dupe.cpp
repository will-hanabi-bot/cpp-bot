// TIIAH replay 2013726 (reactor0/DECISION_MAKING.md, the own-dupe filter; v18.7.0).
// Throw It in a Hole & Brown (4 Suits). Seats: 0 yagami_black (human),
// 1 yagami_green (us), 2 yagami_blue.
//
// Human diagnostic v18_human_vs_bot_diagnostics/2013726.md T17: blue's playable g1
// on chop is in danger, blue has no reason to clue, and "a human player would
// surely clue 1 or green here". The own-dupe filter vetoed every such clue, since
// our own hole card o21 is `{r3,g1}`; we played our b3 and blue discarded the g1.
// v16.29.0, before the filter, gave the 1.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (4 Suits). 3 players, our_player_index=1.

TEST(TiiahChopSave2013726, SavePlayableChopDespiteOwnDupe) {
  // Reconstruct exactly the Game the live bot saw at turn 17.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2013726,
  "debug": {
    "cards_left": 15,
    "clue_tokens": 6,
    "common_play_stacks": [
      2,
      0,
      2,
      1
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 14,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 9,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 20,
        "rank": 4,
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
              2,
              1
            ],
            "inferred": 1032191,
            "info_lock": null,
            "order": 18,
            "possible": 1032191,
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
            "inferred": 1032191,
            "info_lock": null,
            "order": 4,
            "possible": 1032191,
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
            "inferred": 1032191,
            "info_lock": null,
            "order": 3,
            "possible": 1032191,
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
            "inferred": 1032191,
            "info_lock": null,
            "order": 2,
            "possible": 1032191,
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
            "inferred": 1032191,
            "info_lock": null,
            "order": 1,
            "possible": 1032191,
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
            "inferred": 1016831,
            "info_lock": null,
            "order": 23,
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
            "inferred": 1016303,
            "info_lock": null,
            "order": 19,
            "possible": 1016303,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              5
            ],
            "inferred": 16384,
            "info_lock": null,
            "order": 15,
            "possible": 16384,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              3
            ],
            "inferred": 4096,
            "info_lock": null,
            "order": 6,
            "possible": 4096,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1016171,
            "info_lock": null,
            "order": 5,
            "possible": 1016171,
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
            "focused": false,
            "id": [
              1,
              1
            ],
            "inferred": 1032191,
            "info_lock": null,
            "order": 24,
            "possible": 1032191,
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
              2
            ],
            "inferred": 16383,
            "info_lock": null,
            "order": 17,
            "possible": 16383,
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
              5
            ],
            "inferred": 1015808,
            "info_lock": null,
            "order": 13,
            "possible": 1015808,
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
            "inferred": 1015808,
            "info_lock": null,
            "order": 12,
            "possible": 1015808,
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
            "inferred": 928,
            "info_lock": null,
            "order": 10,
            "possible": 928,
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
      },
      {
        "k": "clue",
        "v": "Fix"
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
        "k": "discard",
        "v": "None"
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
        "v": "Reveal"
      }
    ],
    "pairwise_play_stacks": [
      [
        2,
        0,
        2,
        1
      ],
      [
        2,
        0,
        2,
        1
      ],
      [
        2,
        0,
        2,
        1
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      2,
      0,
      2,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 11,
        "superposition": 2050
      },
      {
        "holder": 1,
        "order": 21,
        "superposition": 36
      }
    ],
    "turn_count": 17,
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
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 20,
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
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 20,
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
        "order": 16,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
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
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 6,
        "max": 20,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 20,
        "score": 4,
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
          12,
          13,
          22
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 7,
        "max": 20,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 6,
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
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 6,
        "max": 20,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
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
      [
        2,
        3
      ],
      null,
      [
        2,
        1
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
      [
        2,
        5
      ],
      [
        1,
        4
      ],
      [
        0,
        2
      ],
      [
        2,
        1
      ],
      null,
      [
        3,
        4
      ],
      null,
      [
        3,
        1
      ],
      null,
      [
        1,
        1
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
  "ts": "2026-09-29T19:28:11.854",
  "turn": 17
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // Blue's chop is o24, the g1 -- playable -- and blue has nothing else to do.
  // Our own hole card o21 may have been a g1 too, which is what the own-dupe
  // filter vetoes every call on; blue's chop is the one card it spares.
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  const auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_TRUE(rank || colour) << "a save of blue's g1, not our own b3 play";
  EXPECT_EQ(rank ? rank->target : colour->target, 2) << "to blue";
  if (rank) EXPECT_EQ(rank->value, 1) << "a 1";
  if (colour) EXPECT_EQ(colour->value, 1) << "or Green";
}
