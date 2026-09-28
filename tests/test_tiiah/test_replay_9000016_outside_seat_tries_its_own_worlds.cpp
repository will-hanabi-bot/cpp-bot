// Self-play replay 9000016 (v17_self_play_diagnostics, seed 16; tiiah/CONVENTION.md
// §1e, v17.1.0). Seats: 0 sim-alice, 1 sim-bob (us), 2 sim-cathy.
//
// T8: we blind-play our o16, the b2, without being able to name it. T9: Cathy's
// Blue names Alice's o0, the b3 -- a play on the b2 they both watched go in. We sit
// outside the pair, and on every frame of theirs we could compute the b3 was not
// playable, so until v17.1.0 we read the clue as a MISTAKE while they read a play.
// An outside seat now also tries the worlds of its own hole cards, and the call
// tells it what its o16 was.

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

TEST(SelfPlayReplay9000016, TheOutsideSeatTriesItsOwnWorlds) {
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-bob",
  "ch": "STATE",
  "current_player_index": 1,
  "game_id": 9000016,
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
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 1,
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
        "rank": 5,
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
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          9,
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
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
          3,
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
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
          16,
          7,
          5
        ],
        "t": "clue",
        "target": 1,
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
        "num": 6,
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
        "order": 17,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "num": 7,
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
        "order": 18,
        "p": 1,
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
        "cpi": 2,
        "num": 8,
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
        "value": 3
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
      [
        3,
        3
      ],
      [
        1,
        4
      ],
      [
        1,
        2
      ],
      [
        3,
        1
      ],
      [
        1,
        5
      ],
      null,
      null,
      null,
      [
        0,
        3
      ],
      null,
      [
        0,
        3
      ],
      [
        3,
        4
      ],
      [
        0,
        5
      ],
      [
        1,
        3
      ],
      [
        4,
        5
      ],
      [
        1,
        1
      ],
      null,
      [
        0,
        1
      ],
      null,
      [
        4,
        2
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
    "variant": "Throw It in a Hole (5 Suits)"
  },
  "ts": "2026-09-28T20:29:54.131",
  "turn": 10
}
  )json";
  hanabi::Game game = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  const hanabi::Identity kB3{3, 3};
  EXPECT_EQ(game.meta[0].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "Cathy's Blue calls Alice's b3 at our seat too";
  EXPECT_EQ(game.common.thoughts[0].inferred, hanabi::IdentitySet::single(kB3));
  EXPECT_FALSE(game.meta[16].superposed()) << "and our o16 was the b2";
  EXPECT_EQ(game.state.play_stacks[3], 2);
  EXPECT_EQ(game.state.pairwise_play_stacks[0][3], 2) << "Alice watched our b2";
  EXPECT_EQ(game.state.pairwise_play_stacks[2][3], 2) << "and so did Cathy";
}
