// Self-play replay 9000031 (v17_self_play_diagnostics, seed 31; tiiah/CONVENTION.md
// §1d, §1.3, v17.2.0). Seats: 0 sim-alice, 1 sim-bob (us), 2 sim-cathy.
//
// T2: our Rank 4 to Alice is a reactive: Cathy reacts, Alice receives. T3: Cathy
// plays her o10, the g1. We and Cathy name it exactly -- we saw the target the walk
// paired it with -- but Alice cannot: her reading of it is the bucket, `{g1,b1}`.
// Until v17.2.0 we and Cathy moved the shared view to green 1 and Alice did not,
// and the three seats' shared views never agreed again. The card is now the team's
// only as far as the receiver can name it: we keep the g1 privately, the shared view
// carries `{g1,b1}` for it, and our rows take it -- Alice and Cathy each watched it or
// knew it -- while Cathy's row for Alice does not.

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

TEST(SelfPlayReplay9000031, TheReacterCardIsNamedAsFarAsTheReceiverCan) {
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-bob",
  "ch": "STATE",
  "current_player_index": 1,
  "game_id": 9000031,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
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
        "rank": 1,
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
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 0,
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
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
          3
        ],
        "t": "clue",
        "target": 0,
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
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 2,
        "suit": 0,
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        2,
        5
      ],
      [
        2,
        2
      ],
      [
        0,
        3
      ],
      [
        0,
        4
      ],
      [
        1,
        3
      ],
      null,
      null,
      null,
      null,
      null,
      [
        2,
        1
      ],
      [
        4,
        4
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
        2,
        4
      ],
      [
        0,
        2
      ],
      [
        3,
        4
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
  "ts": "2026-09-28T21:24:10.872",
  "turn": 4
}
  )json";
  hanabi::Game game = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  EXPECT_EQ(game.state.common_play_stacks[2], 0) << "the team cannot name the g1 yet";
  EXPECT_TRUE(game.meta[10].named_in_hole.is_empty());
  EXPECT_TRUE(game.meta[10].shared_left.contains(hanabi::Identity{2, 1}));
  EXPECT_TRUE(game.meta[10].shared_left.contains(hanabi::Identity{3, 1}));
  EXPECT_EQ(game.state.pairwise_play_stacks[0][2], 1) << "Alice watched it land";
  EXPECT_EQ(game.state.pairwise_play_stacks[2][2], 1) << "Cathy knew what she played";
}
