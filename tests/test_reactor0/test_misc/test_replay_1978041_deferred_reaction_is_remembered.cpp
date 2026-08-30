// A deferred reaction survives until the reacter finally answers it.
// Replay 1978041 T7 (reactor0, Alternating Clues & White (3 Suits)).
//
// Seats: will-bot69 (0), yagami_black (1), will-bot67 (2).
//
//   T2  yagami clues rank 3 to will-bot69 -- reacter will-bot67, receiver us
//   T3  will-bot67 DEFERS, giving a reactive clue of its own
//   T4  we play order 1 -- our reacter call from T3's clue, not this one
//   T6  will-bot67 plays its urgent slot 5 (order 10, w1): the deferred answer
//   T7  <-- here
//
// Anchor 3 against react slot 5 gives target slot `3 - 5 = 3 (mod 5)`, which at
// CLUE TIME held our order 2 = r1. Red is still on 0, so it is playable, and a
// draw has since pushed it to slot 4.
//
// Before v12.0.0 `Game::waiting` was cleared by the deferral, so the receiver
// forgot the clue outright and discarded order 15 instead. `pending_reactions`
// is the durable copy that survives it (CONVENTION.md §1d.2).
//
// This game trips neither drop rule: the r1 is still playable at T7 (rule 5),
// and we gave no clue between T2 and T6, so nothing supersedes (rule 6).

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Alternating Clues & White (3 Suits). 3 players, our_player_index=0.

TEST(MiscReplay1978041, ADeferredReactionSurvivesUntilTheReacterAnswers) {
  // Reconstruct exactly the Game the live bot saw at turn 7.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1978041,
  "debug": {
    "cards_left": 12,
    "clue_tokens": 5,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 15,
            "possible": 32767,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28539,
            "info_lock": null,
            "order": 4,
            "possible": 28539,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28539,
            "info_lock": null,
            "order": 3,
            "possible": 28539,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28539,
            "info_lock": null,
            "order": 2,
            "possible": 28539,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4228,
            "info_lock": null,
            "order": 0,
            "possible": 4228,
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
            "id": [
              1,
              5
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 16,
            "possible": 32767,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              1
            ],
            "inferred": 32,
            "info_lock": 32,
            "order": 9,
            "possible": 992,
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
              3
            ],
            "inferred": 30,
            "info_lock": null,
            "order": 7,
            "possible": 31,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              2
            ],
            "inferred": 960,
            "info_lock": null,
            "order": 6,
            "possible": 992,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              0,
              4
            ],
            "inferred": 30,
            "info_lock": null,
            "order": 5,
            "possible": 31,
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
              0,
              1
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 17,
            "possible": 32767,
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
              1
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 14,
            "possible": 32767,
            "slot": 2,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 13,
            "possible": 32767,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 12,
            "possible": 32767,
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
              4
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 11,
            "possible": 32767,
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
      }
    ],
    "play_stacks": [
      0,
      2,
      1
    ],
    "strikes": 0,
    "turn_count": 7,
    "waiting": []
  },
  "game_id": 4622,
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
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 1,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          6,
          8,
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 7,
        "max": 15,
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
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 6,
        "max": 15,
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
          5,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 5,
        "max": 15,
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
        "rank": 1,
        "suit": 1,
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
        "max": 15,
        "score": 1,
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
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
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
        4
      ],
      [
        1,
        2
      ],
      [
        0,
        3
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
        2,
        1
      ],
      [
        2,
        4
      ],
      [
        1,
        3
      ],
      [
        0,
        1
      ],
      [
        1,
        1
      ],
      null,
      [
        1,
        5
      ],
      [
        0,
        1
      ]
    ],
    "names": [
      "will-bot69",
      "yagami_black",
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
      "variant_name": "Alternating Clues & White (3 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Alternating Clues & White (3 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-29T23:27:12.136",
  "turn": 7
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr)
      << "the remembered reaction calls for a PLAY; without it the receiver has "
         "nothing to do and burns a card instead";
  EXPECT_EQ(play->target, 2)
      << "order 2 is the r1 -- clue-time slot 3, now slot 4 after a draw";
}
