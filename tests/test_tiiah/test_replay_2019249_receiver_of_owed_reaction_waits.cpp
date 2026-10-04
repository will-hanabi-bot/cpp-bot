// Replay 2019249 T4-T6 (tiiah/CONVENTION.md §1c, v20.12.0, the user's ruling).
// will-bot67's Yellow to yagami at T4 was a reverse reactive: will-bot69's o15 into
// yagami's o9, the r2 -- leftmost playable once yagami's owed T1 reaction, the r1
// (o7), has played. will-bot69 had been that T1 reaction's receiver, so it could
// not name yagami's o7, and read the Yellow as a stable MISTAKE. It now waits: when
// yagami plays o7 at T5, it rewinds to T4 with o7 counted as a standing call, reads
// the reverse reactive, and is called on o15 as will-bot67 reads it.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (5 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2019249, ReceiverOfAnOwedReactionWaitsThenReads) {
  // Reconstruct exactly the Game the live bot saw at turn 6.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2019249,
  "debug": {
    "cards_left": 33,
    "clue_tokens": 5,
    "common_play_stacks": [
      1,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 2,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              1
            ],
            "inferred": 24895223,
            "info_lock": null,
            "order": 4,
            "possible": 24895223,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              4
            ],
            "inferred": 8659208,
            "info_lock": null,
            "order": 3,
            "possible": 8659208,
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
            "inferred": 7576807,
            "info_lock": null,
            "order": 2,
            "possible": 24895223,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
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
            "inferred": 24895223,
            "info_lock": null,
            "order": 1,
            "possible": 24895223,
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
              1
            ],
            "inferred": 24895223,
            "info_lock": null,
            "order": 0,
            "possible": 24895223,
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
            "id": [
              3,
              2
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 16,
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
              0,
              2
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 9,
            "possible": 1047583,
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
            "inferred": 1048608,
            "info_lock": 1048608,
            "order": 8,
            "possible": 32506848,
            "slot": 3,
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
            "order": 6,
            "possible": 1047583,
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
              1
            ],
            "inferred": 1047583,
            "info_lock": null,
            "order": 5,
            "possible": 1047583,
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
            "id": null,
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
            "id": null,
            "inferred": 32472030,
            "info_lock": null,
            "order": 14,
            "possible": 32472030,
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
            "order": 13,
            "possible": 32472030,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1082401,
            "info_lock": null,
            "order": 11,
            "possible": 1082401,
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
            "order": 10,
            "possible": 32472030,
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
        "k": "clue",
        "v": "Discard"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Mistake"
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
        0,
        0,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      0,
      0,
      0,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 2,
        "order": 12,
        "superposition": 1082400
      }
    ],
    "turn_count": 6,
    "waiting": []
  },
  "game_id": 11753,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 0,
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
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "giver": 0,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        2,
        1
      ],
      [
        0,
        1
      ],
      [
        0,
        1
      ],
      [
        3,
        4
      ],
      [
        4,
        1
      ],
      [
        3,
        1
      ],
      [
        3,
        3
      ],
      [
        0,
        1
      ],
      [
        1,
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
      null,
      [
        3,
        2
      ]
    ],
    "names": [
      "will-bot67",
      "yagami_black",
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
      "variant_name": "Throw It in a Hole & Rainbow (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-04T17:48:54.963",
  "turn": 6
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  using hanabi::CardStatus;
  using hanabi::Identity;
  ASSERT_EQ(game.meta[15].status, CardStatus::CALLED_TO_PLAY)
      << "the reverse reactive's reacter card";
  EXPECT_EQ(game.common.thoughts[15].inferred, hanabi::IdentitySet::empty()
                                                   .add(Identity{2, 1})
                                                   .add(Identity{2, 2})
                                                   .add(Identity{3, 1})
                                                   .add(Identity{3, 2}));
  ASSERT_EQ(game.deferred_reads.size(), 1u);
  EXPECT_EQ(game.deferred_reads[0].turn, 4);
  EXPECT_EQ(game.deferred_reads[0].standing, 7) << "yagami's o7, played at T5";
  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_TRUE(play) << "o15, not o11";
  EXPECT_EQ(play->target, 15);
}
