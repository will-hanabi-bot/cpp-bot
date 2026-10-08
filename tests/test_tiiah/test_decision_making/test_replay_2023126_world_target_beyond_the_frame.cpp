// TIIAH replay 2023126 T13-T14, yagami_green (tiiah/CONVENTION.md §1e; v22.6.0,
// the user's report). Throw It in a Hole & Omni (5 Suits; buckets {r,y} {g,b} {o}).
// Seats: 0 yagami_black (human), 1 yagami_green (us, the reacter), 2 yagami_blue.
//
// We could not name our own hole cards o6 `{r1,o2}` and o8 `{r2,y2}` (the r1 and the
// r2), so on the frame red was 0 and blue's o12, the r3, two away: the walk offered
// no target at all. By ASCR the r3 is at most one away in every world where o6 was
// the r1, so it is the target (the b4 is not: no world has the b2 and b3 down).
// black's Red asks for our o18: the connector r2 where red is on 1 (o8 the y2), the
// bucket's g2 or b2 where the r2 is down already. o18 reads `{r2,g2,b2}`, o6
// collapses to the r1, o8 stays open, and we play o18 rather than discard our chop
// (we discarded o19 live).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Omni (5 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2023126, WorldTargetBeyondTheFrame) {
  // Reconstruct exactly the Game the live bot saw at turn 14.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2023126,
  "debug": {
    "cards_left": 29,
    "clue_tokens": 1,
    "common_play_stacks": [
      0,
      1,
      1,
      1,
      1
    ],
    "current_player_index": 1,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              2
            ],
            "inferred": 64,
            "info_lock": 2097216,
            "order": 17,
            "possible": 32506848,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 1047583,
            "info_lock": null,
            "order": 4,
            "possible": 1047583,
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
              3
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 2,
            "possible": 1047583,
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
              4
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 1,
            "possible": 1047583,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              4
            ],
            "inferred": 32506848,
            "info_lock": null,
            "order": 0,
            "possible": 32506848,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 19,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
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
            "id": null,
            "inferred": 1014750,
            "info_lock": null,
            "order": 15,
            "possible": 1014750,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1014750,
            "info_lock": null,
            "order": 9,
            "possible": 1014750,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 32539681,
            "info_lock": null,
            "order": 5,
            "possible": 32539681,
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
              3,
              4
            ],
            "inferred": 1048544,
            "info_lock": null,
            "order": 20,
            "possible": 1048544,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              1
            ],
            "inferred": 1024,
            "info_lock": 1024,
            "order": 16,
            "possible": 27648,
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
              5
            ],
            "inferred": 590368,
            "info_lock": null,
            "order": 13,
            "possible": 623200,
            "slot": 3,
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
            "inferred": 4,
            "info_lock": null,
            "order": 12,
            "possible": 4,
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
              5
            ],
            "inferred": 19,
            "info_lock": null,
            "order": 10,
            "possible": 19,
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
        "k": "play",
        "v": "None"
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
        "k": "clue",
        "v": "Play"
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
        "v": "Mistake"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        1,
        1,
        1,
        1
      ],
      [
        0,
        1,
        1,
        1,
        1
      ],
      [
        0,
        1,
        1,
        1,
        1
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          1,
          1,
          1,
          1
        ],
        "focus_slot": 1,
        "giver": 0,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          20,
          16,
          13,
          12,
          10
        ],
        "turn": 13
      }
    ],
    "play_stacks": [
      0,
      1,
      1,
      1,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 6,
        "superposition": 2097153
      },
      {
        "holder": 1,
        "order": 8,
        "superposition": 66,
        "support": [
          [
            0,
            2,
            1
          ],
          [
            0,
            1,
            2
          ]
        ],
        "worlds": [
          [
            [
              7,
              2,
              1
            ],
            [
              6,
              0,
              1
            ]
          ],
          [
            [
              7,
              2,
              1
            ],
            [
              6,
              4,
              2
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
          1,
          1,
          1,
          1
        ],
        "clue_value": 0,
        "focus_slot": 1,
        "giver": 0,
        "inverted": false,
        "react_order": -1,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          20,
          16,
          13,
          12,
          10
        ],
        "rlocks": false,
        "turn": 13
      }
    ]
  },
  "game_id": 1078,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 3,
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
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
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
        "order": 7,
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
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5,
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "giver": 1,
        "kind": "C",
        "list": [
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 2,
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
        "order": 17,
        "p": 0,
        "rank": 2,
        "suit": 1,
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
        "num": 7,
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
        "order": 18,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          17
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
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
        "clues": 2,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
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
        "order": 19,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "num": 11,
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
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 3,
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
        "num": 12,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 1,
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
        4,
        4
      ],
      [
        2,
        4
      ],
      [
        3,
        3
      ],
      [
        1,
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
        0,
        5
      ],
      [
        3,
        1
      ],
      [
        0,
        3
      ],
      [
        3,
        5
      ],
      [
        4,
        1
      ],
      null,
      [
        2,
        1
      ],
      [
        1,
        2
      ],
      null,
      null,
      [
        3,
        4
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
      "variant_name": "Throw It in a Hole & Omni (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Omni (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-08T01:44:54.214",
  "turn": 14
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  EXPECT_EQ(game.waiting.empty() ? -1 : game.waiting.front().receiver_target_order, 12)
      << "blue's r3 is the target";
  hanabi::IdentitySet want = hanabi::IdentitySet::empty();
  for (const char* c : {"r2", "g2", "b2"}) want = want.add(game.state.expand_short(c));
  EXPECT_EQ(game.common.thoughts[18].inferred, want) << "connector or bucket playable";
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action)) << "not the chop discard";
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 18);
}
