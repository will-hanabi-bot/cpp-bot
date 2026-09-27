// Rule 6's collapse is SHARED, and it corrects the frozen clue frame
// (tiiah/CONVENTION.md 1e rule 6 and 1d, v16.21.0).
//
// TIIAH replay 2011133. will-bot67 threw order 13 -- a b1 -- into the hole at T6 not
// knowing what it was. At T16 the human clued Blue on its order 14, a b3, and at T17
// will-bot69 answered by blind-playing the b2. Rule 6 did its job: the b2 only lands in
// the world where order 13 was the b1, so order 13 collapsed and will-bot67's own blue
// went 0 -> 2.
//
// Everything downstream still failed, because the collapse was PRIVATE. Every view a
// clue is read against stayed on blue 0, and the frame frozen on the T16 clue stayed
// there too -- so the promise on order 14 came out {b2}, already played, and
// `stamp_receiver_call`'s Rule 5 correctly dropped the whole call as stale. Order 14
// read {b2,b3,b4,b5} with no call for the rest of the game; will-bot67 discarded its
// chop at T18, threw the other b2 away at T21, and sat on the b3 through a second
// identical Blue clue at T28.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (5 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2011133, RuleSixIsShared) {
  // Reconstruct exactly the Game the live bot saw at turn 18.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2011133,
  "debug": {
    "cards_left": 25,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      2,
      1,
      0,
      1
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 2,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 15,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 21,
        "rank": 1,
        "suit": 4
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              3
            ],
            "inferred": 4329604,
            "info_lock": null,
            "order": 22,
            "possible": 4329604,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              4
            ],
            "inferred": 8659208,
            "info_lock": null,
            "order": 20,
            "possible": 8659208,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 8659208,
            "info_lock": null,
            "order": 4,
            "possible": 8659208,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              4
            ],
            "inferred": 8659208,
            "info_lock": null,
            "order": 3,
            "possible": 8659208,
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
            "inferred": 3247203,
            "info_lock": null,
            "order": 1,
            "possible": 20565619,
            "slot": 5,
            "status": "CALLED_TO_DISCARD",
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
              4,
              3
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 24,
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
              3,
              4
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 18,
            "possible": 33554431,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              3
            ],
            "inferred": 960,
            "info_lock": null,
            "order": 9,
            "possible": 960,
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
              5
            ],
            "inferred": 32440350,
            "info_lock": null,
            "order": 7,
            "possible": 32440350,
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
              5
            ],
            "inferred": 32440350,
            "info_lock": null,
            "order": 5,
            "possible": 32440350,
            "slot": 5,
            "status": "NONE",
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
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32538623,
            "info_lock": null,
            "order": 19,
            "possible": 32538623,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32538623,
            "info_lock": null,
            "order": 17,
            "possible": 32538623,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 983040,
            "info_lock": null,
            "order": 14,
            "possible": 983040,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 31488990,
            "info_lock": null,
            "order": 12,
            "possible": 31488990,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 31488990,
            "info_lock": null,
            "order": 11,
            "possible": 31488990,
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
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Play"
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
        "k": "discard",
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
        2,
        1,
        0,
        1
      ],
      [
        0,
        2,
        1,
        0,
        1
      ],
      [
        0,
        2,
        1,
        0,
        1
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      2,
      2,
      2,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 23,
        "superposition": 34816
      }
    ],
    "turn_count": 18,
    "waiting": []
  },
  "game_id": 1812,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 2,
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
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 5,
        "suit": 3,
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
        "rank": 5,
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
        "rank": 3,
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
        "kind": "C",
        "list": [
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
          13
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
        "giver": 2,
        "kind": "C",
        "list": [
          8,
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 3,
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
          16
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
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
        "order": 18,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
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
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
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
        "order": 21,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          3,
          4,
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 21,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
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
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        1,
        1
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
        4
      ],
      [
        3,
        5
      ],
      [
        2,
        1
      ],
      [
        0,
        5
      ],
      [
        1,
        2
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
        3
      ],
      [
        4,
        1
      ],
      null,
      [
        3,
        4
      ],
      null,
      [
        0,
        4
      ],
      [
        4,
        1
      ],
      [
        3,
        3
      ],
      [
        3,
        2
      ],
      [
        4,
        3
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-27T16:37:47.745",
  "turn": 18
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  // 1. The collapse itself, which already worked -- pinned so the rest has a floor.
  EXPECT_FALSE(game.meta[13].superposed())
      << "the b2 only lands in the world where order 13 was the b1";
  const std::vector<int> belief{0, 2, 2, 2, 1};
  EXPECT_EQ(game.state.play_stacks, belief) << "our own blue is on 2: our b1, then the b2";

  // 2. What v16.21.0 adds: the conclusion is one every seat reaches, so the SHARED view
  //    moves with our belief. It read [0,2,1,0,1] -- blue 0 -- before.
  EXPECT_EQ(game.state.common_play_stacks[3], 1)
      << "the b1 is common knowledge now: they all watched the card, they all hold its "
         "candidate set, and never-presume-a-strike is the convention";
  for (size_t p = 0; p < game.state.pairwise_play_stacks.size(); ++p) {
    EXPECT_GE(game.state.pairwise_play_stacks[p][3], 1)
        << "every row learns it too, row " << p;
  }

  // 3. And the frozen frame is corrected, which is what saves the promise. Without it
  //    the T16 frame keeps blue 0, the reacter's advance adds only +1 -- the collapse
  //    is already inside `prev` -- and the promise reads {b2}.
  const int b3_order = 14;
  EXPECT_EQ(game.meta[b3_order].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the call survives Rule 5 instead of being dropped as a stale reading";
  EXPECT_EQ(game.common.thoughts[b3_order].possibilities(),
            hanabi::IdentitySet::single(hanabi::Identity{3, 3}))
      << "and it names the b3 exactly -- which is what the card is";

  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "it discarded its chop here before";
  EXPECT_EQ(play->target, b3_order);
}
