// TIIAH replay 2013616 (tiiah/CONVENTION.md §1.3, §1e, v18.1.0). Throw It in a
// Hole & Brown (4 Suits). Seats: 0 will-bot67, 1 will-bot69 (us), 2 yagami_black.
//
// At T2 we blind-played o6, the r1, from a set of `{r1,g1}`. Later we settled it
// privately as the r1, because both other g1s were in view. At T18 yagami's o23, a
// g1, went into the hole with will-bot67 and us both watching. At will-bot67's
// seat, its row for us replayed our o6 with `{r1,g1}`: the g1 world struck as a
// duplicate, and the row went to red 1. Our copy of that row left o6 out, because
// its private set was empty, so it stayed on red 0. The T19 Green from will-bot67
// to yagami was an ordinary reactive with us reacting. Paired on different rows,
// it had no pairing at our seat, and at T20 we read it as a MISTAKE and discarded
// o15. The giver meant o24, the r2.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (4 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2013616, ReactsAfterThePairSettles) {
  // Reconstruct exactly the Game the live bot saw at turn 20.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2013616,
  "debug": {
    "cards_left": 14,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      0,
      1,
      4
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 14,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 4,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 13,
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
              2,
              1
            ],
            "inferred": 1048567,
            "info_lock": null,
            "order": 21,
            "possible": 1048567,
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
              2
            ],
            "inferred": 1048567,
            "info_lock": null,
            "order": 3,
            "possible": 1048567,
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
            "inferred": 1048567,
            "info_lock": null,
            "order": 2,
            "possible": 1048567,
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
              2
            ],
            "inferred": 1048567,
            "info_lock": null,
            "order": 1,
            "possible": 1048567,
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
            "inferred": 1048567,
            "info_lock": null,
            "order": 0,
            "possible": 1048567,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1048567,
            "info_lock": null,
            "order": 24,
            "possible": 1048567,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 4228,
            "info_lock": null,
            "order": 17,
            "possible": 4228,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 503139,
            "info_lock": null,
            "order": 15,
            "possible": 1044339,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1044339,
            "info_lock": null,
            "order": 8,
            "possible": 1044339,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1044339,
            "info_lock": null,
            "order": 7,
            "possible": 1044339,
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
            "clued": true,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 992,
            "info_lock": null,
            "order": 25,
            "possible": 992,
            "slot": 1,
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
            "inferred": 4100,
            "info_lock": null,
            "order": 20,
            "possible": 4100,
            "slot": 2,
            "status": "CHOP_MOVED",
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
            "inferred": 256,
            "info_lock": null,
            "order": 18,
            "possible": 256,
            "slot": 3,
            "status": "CHOP_MOVED",
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
            "order": 16,
            "possible": 4,
            "slot": 4,
            "status": "CHOP_MOVED",
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
            "inferred": 8,
            "info_lock": null,
            "order": 10,
            "possible": 8,
            "slot": 5,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
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
        "k": "clue",
        "v": "Fix"
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
        "v": "Lock"
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
        "v": "Mistake"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        1,
        1,
        4
      ],
      [
        0,
        0,
        1,
        4
      ],
      [
        0,
        0,
        1,
        4
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          1,
          4
        ],
        "focus_slot": 3,
        "giver": 0,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          25,
          20,
          18,
          16,
          10
        ],
        "turn": 19
      }
    ],
    "play_stacks": [
      1,
      1,
      1,
      4
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 23,
        "superposition": 524321
      }
    ],
    "turn_count": 20,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_play_stacks": [
          0,
          0,
          1,
          4
        ],
        "clue_value": 1,
        "focus_slot": 3,
        "giver": 0,
        "inverted": false,
        "react_order": -1,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          25,
          20,
          18,
          16,
          10
        ],
        "rlocks": false,
        "turn": 19
      }
    ]
  },
  "game_id": 4906,
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
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
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
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 3,
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
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 6,
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
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
          10,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
        "order": 5,
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
        "max": 20,
        "score": 3,
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
        "order": 18,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          10,
          14,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 3,
        "suit": 0,
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
        "failed": false,
        "order": 19,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 8,
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
        "giver": 1,
        "kind": "R",
        "list": [
          13,
          16,
          20
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
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 3,
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
        "kind": "R",
        "list": [
          17
        ],
        "t": "clue",
        "target": 1,
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
        "giver": 1,
        "kind": "C",
        "list": [
          22
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 6,
        "max": 20,
        "score": 4,
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
        "order": 23,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 5,
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
          10,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 5,
        "max": 20,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "order": 9,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 20,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 20,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          18,
          25
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 4,
        "max": 20,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
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
        2,
        2
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
        1
      ],
      [
        2,
        1
      ],
      null,
      null,
      null,
      [
        3,
        4
      ],
      [
        0,
        4
      ],
      [
        3,
        2
      ],
      [
        3,
        1
      ],
      [
        2,
        3
      ],
      [
        0,
        4
      ],
      null,
      [
        0,
        3
      ],
      null,
      [
        1,
        4
      ],
      [
        2,
        1
      ],
      [
        0,
        3
      ],
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
        1
      ],
      null,
      [
        1,
        5
      ]
    ],
    "names": [
      "will-bot67",
      "will-bot69",
      "yagami_black"
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
  "ts": "2026-09-29T16:49:41.332",
  "turn": 20
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // Both seats replay our o6 with the set they share, so our row for will-bot67
  // reaches the same red 1 as will-bot67's row for us.
  const std::vector<int> pair_row{1, 1, 1, 4};
  EXPECT_EQ(game.state.pairwise_play_stacks[0], pair_row)
      << "the pair watched a g1 land after our {r1,g1}, so ours was the r1";
  EXPECT_TRUE(game.meta[6].private_named.contains(hanabi::Identity{0, 1}))
      << "guard: we still hold our own settle of o6 as the r1";

  // With the rows agreeing, the Green has a pairing at our seat too, and the
  // reaction is o24 -- the r2, playable on the true red 1.
  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "the Green is a reactive to answer, not a mistake";
  EXPECT_EQ(play->target, 24);
}
