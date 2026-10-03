// Replay 2018517 T19-T21 (tiiah/CONVENTION.md §1d, the receiver world fallback,
// v20.5.0). will-bot67 had thrown o21 `{r2,y1}` into the hole unnamed (it was the
// r2). yagami's 3 was a reactive: will-bot69's m3 into will-bot67's slot 1, o23,
// the r3. On the frame the giver and will-bot67 share red was 1, so no 3 played and
// the shared stamp read a bluff; will-bot67 discarded at T21. In the world where
// o21 was the r2 the r3 plays: o21 collapses to `{r2}` and o23 is called.

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

// Variant: Throw It in a Hole & Rainbow (5 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2018517, ReceiverWorldFallbackCollapsesHole) {
  // Reconstruct exactly the Game the live bot saw at turn 21.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2018517,
  "debug": {
    "cards_left": 25,
    "clue_tokens": 1,
    "common_play_stacks": [
      1,
      0,
      1,
      1,
      3
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 6,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 7,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 13,
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
              0,
              1
            ],
            "inferred": 33521663,
            "info_lock": null,
            "order": 20,
            "possible": 33521663,
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
              5
            ],
            "inferred": 984030,
            "info_lock": null,
            "order": 4,
            "possible": 984030,
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
              4
            ],
            "inferred": 984030,
            "info_lock": null,
            "order": 3,
            "possible": 984030,
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
            "inferred": 984030,
            "info_lock": null,
            "order": 2,
            "possible": 984030,
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
              4
            ],
            "inferred": 984030,
            "info_lock": null,
            "order": 0,
            "possible": 984030,
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
              0,
              2
            ],
            "inferred": 33521663,
            "info_lock": null,
            "order": 24,
            "possible": 33521663,
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
              4
            ],
            "inferred": 33521663,
            "info_lock": null,
            "order": 22,
            "possible": 33521663,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              4
            ],
            "inferred": 8659208,
            "info_lock": null,
            "order": 9,
            "possible": 8659208,
            "slot": 3,
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
            "inferred": 17318416,
            "info_lock": null,
            "order": 8,
            "possible": 17318416,
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
            "inferred": 5379237,
            "info_lock": null,
            "order": 5,
            "possible": 5379237,
            "slot": 5,
            "status": "CALLED_TO_DISCARD",
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
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 135172,
            "info_lock": null,
            "order": 23,
            "possible": 4329604,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 135296,
            "info_lock": null,
            "order": 18,
            "possible": 135296,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4224,
            "info_lock": null,
            "order": 12,
            "possible": 4224,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28512,
            "info_lock": null,
            "order": 11,
            "possible": 28512,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28512,
            "info_lock": null,
            "order": 10,
            "possible": 28512,
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
        "v": "Discard"
      },
      {
        "k": "clue",
        "v": "Play"
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
        "k": "play",
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
        1,
        0,
        1,
        1,
        3
      ],
      [
        1,
        0,
        1,
        1,
        3
      ],
      [
        1,
        0,
        1,
        1,
        3
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      0,
      1,
      1,
      3
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 21,
        "superposition": 34
      }
    ],
    "turn_count": 21,
    "waiting": []
  },
  "game_id": 10800,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 3,
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
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 4,
        "suit": 1,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 5
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
        "kind": "C",
        "list": [
          13,
          14
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
        "value": 2
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
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
        "failed": false,
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
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
        "kind": "R",
        "list": [
          15
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
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
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
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
        "order": 18,
        "p": 2,
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
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          16
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 2,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 25,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
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
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          12,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 21,
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
        "clues": 2,
        "max": 25,
        "score": 6,
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
          12,
          18,
          23
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 1,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        3,
        4
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
        3,
        4
      ],
      [
        1,
        5
      ],
      [
        0,
        3
      ],
      [
        3,
        1
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
        1,
        4
      ],
      null,
      null,
      null,
      [
        4,
        1
      ],
      [
        3,
        1
      ],
      [
        4,
        1
      ],
      [
        4,
        2
      ],
      [
        4,
        3
      ],
      null,
      [
        0,
        1
      ],
      [
        0,
        1
      ],
      null,
      [
        0,
        4
      ],
      null,
      [
        0,
        2
      ]
    ],
    "names": [
      "yagami_black",
      "will-bot69",
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
      "variant_name": "Throw It in a Hole & Rainbow (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T22:19:59.298",
  "turn": 21
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_EQ(game.meta[23].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_EQ(game.common.thoughts[23].inferred,
            hanabi::IdentitySet::single(hanabi::Identity{0, 3}))
      << "the r3, in the world where o21 was the r2";
  // o21 collapses to the r2: red goes to 2 in our belief and in the shared view.
  EXPECT_EQ(game.state.play_stacks[0], 2) << "red is on 2 now";
  EXPECT_EQ(game.state.common_play_stacks[0], 2) << "for every seat";
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_TRUE(play) << "the r3, not a discard";
  EXPECT_EQ(play->target, 23);
}
