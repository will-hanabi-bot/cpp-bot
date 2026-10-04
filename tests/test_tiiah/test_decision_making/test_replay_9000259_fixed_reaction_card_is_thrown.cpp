// Self-play seed 259 T5 (game 9000259; tiiah/CONVENTION.md §1c, v20.12.0, the user's
// ruling). Bob owed Cathy a reaction on his o7 and deferred at T2. Cathy then played
// a b1 into the hole, so o7, a b1, was dead, and Alice's T4 Green fixed it. A fixed
// reaction card is thrown at once: Cathy, who cannot name o7 and is waiting on the
// Green, learns from the discard that it was the fix, not a reverse reactive. Until
// v20.12.0 Bob clued instead and later played an unrelated r1, which Cathy took for
// his standing call.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay9000259, FixedReactionCardIsThrown) {
  // Reconstruct exactly the Game the live bot saw at turn 5.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-bob",
  "ch": "STATE",
  "current_player_index": 1,
  "debug": {
    "cards_left": 34,
    "clue_tokens": 5,
    "common_play_stacks": [
      0,
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
              0,
              2
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 4,
            "possible": 32538623,
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
              3
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 3,
            "possible": 32538623,
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
              4
            ],
            "inferred": 1015808,
            "info_lock": null,
            "order": 2,
            "possible": 1015808,
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
              2
            ],
            "inferred": 65536,
            "info_lock": null,
            "order": 1,
            "possible": 1015808,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              2
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 0,
            "possible": 32538623,
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
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31744,
            "info_lock": null,
            "order": 9,
            "possible": 31744,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33522687,
            "info_lock": null,
            "order": 8,
            "possible": 33522687,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32768,
            "info_lock": 1081377,
            "order": 7,
            "possible": 33522687,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33522687,
            "info_lock": null,
            "order": 6,
            "possible": 33522687,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33522687,
            "info_lock": null,
            "order": 5,
            "possible": 33522687,
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
              3,
              2
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
              1,
              1
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 14,
            "possible": 32538623,
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
              5
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 13,
            "possible": 32538623,
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
              4
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
            "clued": false,
            "focused": false,
            "id": [
              1,
              1
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 11,
            "possible": 32538623,
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
        "k": "clue",
        "v": "Fix"
      }
    ],
    "pairwise_play_stacks": [
      [
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
        0
      ],
      [
        0,
        0,
        0,
        1,
        0
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          0,
          0,
          0
        ],
        "focus_slot": 4,
        "giver": 0,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          14,
          13,
          12,
          11,
          10
        ],
        "turn": 1
      }
    ],
    "play_stacks": [
      0,
      0,
      0,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 5,
    "waiting": []
  },
  "game_id": 9000259,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 0,
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
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 1,
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
        "rank": 5,
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
          12,
          10
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
        "kind": "C",
        "list": [
          2,
          1
        ],
        "t": "clue",
        "target": 0,
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
        "suit": 3,
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
        "giver": 0,
        "kind": "C",
        "list": [
          9
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
        "cpi": 1,
        "num": 4,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        1,
        2
      ],
      [
        3,
        2
      ],
      [
        3,
        4
      ],
      [
        2,
        3
      ],
      [
        0,
        2
      ],
      null,
      null,
      null,
      null,
      null,
      [
        3,
        1
      ],
      [
        1,
        1
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
        1
      ],
      [
        3,
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
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-04T18:59:05.518",
  "turn": 5
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  const auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  ASSERT_TRUE(discard) << "the fixed reaction card, thrown at once";
  EXPECT_EQ(discard->target, 7);
}
