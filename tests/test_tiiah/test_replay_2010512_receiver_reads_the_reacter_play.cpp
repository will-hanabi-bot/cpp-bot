// The receiver reads what the REACTER played (tiiah/CONVENTION.md 1d, v16.19.0).
//
// TIIAH replay 2010512. yagami answered two rank-1 reactives by playing its `p1` and
// then its `p2`. Bucket 2 is `{p}` alone, so "a playable card of bucket 2" is a single
// identity each time, and will-bot69 -- the giver, which ran the target walk -- resolved
// both. will-bot67 is the RECEIVER, and the receiver returns before the walk because it
// cannot see its own hand: in its game the reacter's card was never narrowed at all. So
// it kept two 20- and 24-candidate superpositions, its shared stacks read purple 0
// instead of 2, and its row for yagami never moved off [0,0,0,1,0].
//
// At T13 the human gave a reactive yellow naming will-bot69's `y2`, which is playable
// only once yellow is on 1. Against that row it read one away, so the pairing came out a
// FINESSE demanding a `y1` the reacter could not hold; the walk found no pairing at all
// and the clue was recorded as a MISTAKE. will-bot67 then gave a stable clue while
// will-bot69 went on waiting for its reaction.
//
// Generated from the live log at turn 14; `apply_snapshot` replays the action history, so
// the T3 and T9 reactives and the T13 clue are all interpreted by the build under test.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2010512, ReceiverReadsTheReacterPlay) {
  // Reconstruct exactly the Game the live bot saw at turn 14.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2010512,
  "debug": {
    "cards_left": 27,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      0,
      0,
      1,
      0
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 17,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 12,
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
              3,
              1
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 20,
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
            "clued": false,
            "focused": false,
            "id": [
              0,
              5
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 16,
            "possible": 32538623,
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
              3
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 4,
            "possible": 32538623,
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
            "inferred": 32538623,
            "info_lock": null,
            "order": 1,
            "possible": 32538623,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_light",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 21,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1081344,
            "info_lock": null,
            "order": 19,
            "possible": 1082401,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32472030,
            "info_lock": null,
            "order": 9,
            "possible": 32472030,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32472030,
            "info_lock": null,
            "order": 8,
            "possible": 32472030,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32472030,
            "info_lock": null,
            "order": 6,
            "possible": 32472030,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              4
            ],
            "inferred": 33553439,
            "info_lock": null,
            "order": 22,
            "possible": 33553439,
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
              4
            ],
            "inferred": 992,
            "info_lock": null,
            "order": 14,
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
              1,
              2
            ],
            "inferred": 992,
            "info_lock": null,
            "order": 13,
            "possible": 992,
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
              2
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 11,
            "possible": 31744,
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
            "inferred": 33521695,
            "info_lock": null,
            "order": 10,
            "possible": 33521695,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
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
        "k": "clue",
        "v": "Mistake"
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
        1,
        2
      ],
      [
        1,
        1,
        0,
        1,
        2
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          0,
          1,
          0
        ],
        "focus_slot": 2,
        "giver": 0,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          22,
          14,
          13,
          11,
          10
        ],
        "turn": 13
      }
    ],
    "play_stacks": [
      0,
      0,
      0,
      1,
      2
    ],
    "strikes": 1,
    "superpositions": [
      {
        "holder": 0,
        "order": 2,
        "superposition": 32538623
      },
      {
        "holder": 0,
        "order": 3,
        "superposition": 33521663
      },
      {
        "holder": 1,
        "order": 5,
        "superposition": 33
      },
      {
        "holder": 1,
        "order": 7,
        "superposition": 33,
        "support": [
          [
            0,
            2,
            1
          ],
          [
            1,
            1,
            1
          ],
          [
            0,
            1,
            2
          ],
          [
            1,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 15,
        "superposition": 4194370,
        "support": [
          [
            0,
            2,
            7
          ],
          [
            1,
            1,
            1
          ],
          [
            1,
            2,
            14
          ],
          [
            0,
            1,
            8
          ]
        ],
        "worlds": [
          [
            [
              5,
              0,
              1
            ],
            [
              7,
              0,
              1
            ]
          ],
          [
            [
              5,
              0,
              1
            ],
            [
              7,
              1,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              7,
              0,
              1
            ]
          ],
          [
            [
              5,
              1,
              1
            ],
            [
              7,
              1,
              1
            ]
          ]
        ]
      }
    ],
    "turn_count": 14,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_play_stacks": [
          0,
          0,
          0,
          1,
          0
        ],
        "clue_value": 1,
        "focus_slot": 2,
        "giver": 0,
        "inverted": false,
        "react_order": -1,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          22,
          14,
          13,
          11,
          10
        ],
        "rlocks": false,
        "turn": 13
      }
    ]
  },
  "game_id": 1081,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
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
        "suit": 3,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
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
        "order": 5,
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
        "max": 25,
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
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 6,
        "max": 25,
        "score": 1,
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
        "order": 16,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
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
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 3,
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
        "value": 3
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
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
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
        "giver": 2,
        "kind": "R",
        "list": [
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "order": 15,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "num": 1,
        "order": 12,
        "t": "strike",
        "turn": 11
      },
      {
        "failed": true,
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 4,
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
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          13,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
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
      null,
      [
        3,
        4
      ],
      [
        2,
        2
      ],
      [
        4,
        1
      ],
      [
        1,
        2
      ],
      [
        1,
        4
      ],
      null,
      [
        0,
        5
      ],
      [
        2,
        4
      ],
      [
        2,
        4
      ],
      null,
      [
        3,
        1
      ],
      null,
      [
        4,
        4
      ]
    ],
    "names": [
      "yagami_light",
      "will-bot67",
      "will-bot69"
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
  "ts": "2026-09-27T00:28:43.457",
  "turn": 14
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // 1. The two reactive blind plays are no longer superpositions at all: each read as
  //    one identity, so `note_hidden_action` took its singleton branch.
  EXPECT_FALSE(game.meta[3].superposed())
      << "yagami's T4 play: bucket 2 on [0,0,0,0,0] is the p1 and nothing else";
  EXPECT_FALSE(game.meta[2].superposed())
      << "and its T10 play: bucket 2 on [0,0,0,1,1] is the p2";

  // 2. Which is what puts purple on 2 in the view every seat shares -- the thing a
  //    superposed play cannot do. Red and yellow are on 1 there too as of v16.24.0:
  //    will-bot67's own {r1,y1} pair on orders 5 and 7 leaves (r1,y1) and (y1,r1)
  //    as the only strike-free worlds, every seat can see that, and the shared view
  //    is the minimum across every seat's worlds (§1e).
  const std::vector<int> shared{1, 1, 0, 1, 2};
  EXPECT_EQ(game.state.common_play_stacks, shared)
      << "it read [0,0,0,1,0] before, with purple stuck behind two plays it could not "
         "name on yagami's behalf";

  // 3. And the row the reactive's target walk runs on. Purple comes from the settles
  //    above; red and yellow from will-bot67's own {r1,y1} pair on orders 5 and 7,
  //    whose (r1,r1) and (y1,y1) worlds each strike (1e rule 6, v16.18.0).
  const std::vector<int> row0{1, 1, 0, 1, 2};
  EXPECT_EQ(game.state.pairwise_play_stacks[0], row0)
      << "what will-bot67 knows yagami knows, at the moment yagami clued";
  EXPECT_EQ(game.state.play_stacks, row0)
      << "and its own belief, which 1.3 gives the reacter to read its own card with -- "
         "two cards short, the y2 looks one away and the pairing reads as a finesse";

  // 4. So the pairing is found, and it is the one the report expected: the receiver's
  //    slot 3 (order 13, the y2) against the reacter's slot 4 (order 8).
  ASSERT_FALSE(game.waiting.empty()) << "the connection survived as a readable one";
  EXPECT_EQ(game.waiting.front().receiver_target_order, 13);
  EXPECT_EQ(game.waiting.front().react_order, 8);
  EXPECT_EQ(game.meta[8].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(game.meta[8].urgent);

  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr)
      << "answering the reaction, rather than cluing at a clue the bot no longer "
         "believed it had been given";
  EXPECT_EQ(play->target, 8) << "order 8 is the b2, and blue is on 1";
}
