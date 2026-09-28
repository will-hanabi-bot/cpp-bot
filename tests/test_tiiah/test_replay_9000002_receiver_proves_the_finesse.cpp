// Self-play replay 9000002 (v17_self_play_diagnostics, seed 2; tiiah/CONVENTION.md
// §1d, v17.1.0). Seats: 0 sim-alice, 1 sim-bob (us), 2 sim-cathy.
//
// T6: Cathy's Red to us names our o7, the r2 -- one away, so the pairing is a
// finesse and Alice, the reacter, reads her card as the connector alone. T7: she
// plays it, the r1. We are the receiver, and cannot see our own card; until
// v17.1.0 we read hers as the whole bucket `{r1,y2}`, so our shared view and our
// row for Alice stayed on red 0 while theirs moved to red 1. Our red card cannot
// be any card of the bucket half (the purple bucket), so the finesse is proven
// and her card is the r1 for the team.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

TEST(SelfPlayReplay9000002, TheReceiverProvesTheFinesse) {
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-bob",
  "ch": "STATE",
  "current_player_index": 1,
  "debug": {
    "cards_left": 31,
    "clue_tokens": 7,
    "common_play_stacks": [
      1,
      1,
      0,
      0,
      0
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 4,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 5,
        "rank": 3,
        "suit": 2
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
              4,
              3
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
              2,
              5
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
              1,
              1
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 2,
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
              2,
              1
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 1,
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
              1
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 0,
            "possible": 33554431,
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
            "id": null,
            "inferred": 33554400,
            "info_lock": null,
            "order": 17,
            "possible": 33554400,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24895200,
            "info_lock": null,
            "order": 9,
            "possible": 24895200,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24895200,
            "info_lock": null,
            "order": 8,
            "possible": 24895200,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 2,
            "info_lock": null,
            "order": 7,
            "possible": 23,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 8659200,
            "info_lock": null,
            "order": 6,
            "possible": 8659200,
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
              1,
              3
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 15,
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
              4,
              3
            ],
            "inferred": 33553439,
            "info_lock": null,
            "order": 13,
            "possible": 33553439,
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
            "inferred": 33553439,
            "info_lock": null,
            "order": 12,
            "possible": 33553439,
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
              4
            ],
            "inferred": 33553439,
            "info_lock": null,
            "order": 11,
            "possible": 33553439,
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
            "inferred": 33553439,
            "info_lock": null,
            "order": 10,
            "possible": 33553439,
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
        1,
        0,
        0,
        0
      ],
      [
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
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      1,
      0,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 8,
    "waiting": []
  },
  "game_id": 9000002,
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
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 4,
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
        "rank": 3,
        "suit": 4,
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
        "kind": "R",
        "list": [
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
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
        "kind": "C",
        "list": [
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
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
        0,
        1
      ],
      [
        2,
        1
      ],
      [
        1,
        1
      ],
      [
        0,
        1
      ],
      [
        2,
        1
      ],
      [
        2,
        3
      ],
      null,
      null,
      null,
      null,
      [
        2,
        4
      ],
      [
        4,
        4
      ],
      [
        3,
        4
      ],
      [
        4,
        3
      ],
      [
        1,
        1
      ],
      [
        1,
        3
      ],
      [
        2,
        5
      ],
      null,
      [
        4,
        3
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-28T20:29:52.861",
  "turn": 8
}
  )json";
  hanabi::Game game = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  const hanabi::Identity kR1{0, 1};
  EXPECT_FALSE(game.meta[3].superposed()) << "Alice's o3 is named, not left {r1,y2}";
  EXPECT_TRUE(game.meta[3].named_in_hole.contains(kR1));
  EXPECT_EQ(game.state.common_play_stacks[0], 1) << "the whole team can name the r1";
  EXPECT_EQ(game.state.pairwise_play_stacks[0][0], 1) << "and so can our row for Alice";
}
