// TIIAH replay 2013645 (tiiah/CONVENTION.md §1c, v18.2.0). Throw It in a Hole &
// Brown (4 Suits). Seats: 0 will-bot67, 1 yagami_black, 2 will-bot69 (us).
//
// yagami's T8 Blue called will-bot69's o12 as the b3; its touches still allowed
// b1-b5. At T11 yagami gave will-bot67 a 4. Bob (will-bot69) held a standing play
// and Cathy (will-bot67) did not, so by ROLE INVERSION the 4 was a stable clue.
// Since v18.0.0 the position was read from clue touches alone, so the o12 did not
// count, the 4 was read as an ordinary reactive with will-bot69 reacting, and at
// T12 will-bot69 blind-played its o14 as the reaction. It was an r3, and struck.
//
// T12, at will-bot69's seat: the 4 is stable, and we play the called b3.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/variants/hole.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Brown (4 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2013645, RoleInversionKeepsTheClueStable) {
  // Reconstruct exactly the Game the live bot saw at turn 12.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2013645,
  "debug": {
    "cards_left": 20,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      0,
      2,
      1
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 5,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 0,
        "rank": 1,
        "suit": 3
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 8456,
            "info_lock": null,
            "order": 19,
            "possible": 8456,
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
              3
            ],
            "inferred": 1038005,
            "info_lock": null,
            "order": 18,
            "possible": 1038005,
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
            "inferred": 640,
            "info_lock": null,
            "order": 4,
            "possible": 672,
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
            "inferred": 8192,
            "info_lock": null,
            "order": 3,
            "possible": 8192,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              2
            ],
            "inferred": 2,
            "info_lock": null,
            "order": 1,
            "possible": 2,
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
              1
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 17,
            "possible": 1048575,
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
              5
            ],
            "inferred": 1016831,
            "info_lock": null,
            "order": 9,
            "possible": 1016831,
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
              3
            ],
            "inferred": 1016831,
            "info_lock": null,
            "order": 8,
            "possible": 1016831,
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
              4
            ],
            "inferred": 1016831,
            "info_lock": null,
            "order": 7,
            "possible": 1016831,
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
              2
            ],
            "inferred": 1016831,
            "info_lock": null,
            "order": 6,
            "possible": 1016831,
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
            "inferred": 1016831,
            "info_lock": null,
            "order": 15,
            "possible": 1016831,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 65536,
            "info_lock": 65569,
            "order": 14,
            "possible": 1016831,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1016831,
            "info_lock": null,
            "order": 13,
            "possible": 1016831,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 4096,
            "info_lock": 4096,
            "order": 12,
            "possible": 31744,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31744,
            "info_lock": null,
            "order": 11,
            "possible": 31744,
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
        "v": "Play"
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
        "v": "Discard"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Reactive"
      }
    ],
    "pairwise_play_stacks": [
      [
        0,
        0,
        2,
        1
      ],
      [
        0,
        0,
        2,
        1
      ],
      [
        0,
        0,
        2,
        1
      ]
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          1,
          1
        ],
        "focus_slot": 4,
        "giver": 1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          19,
          18,
          4,
          3,
          1
        ],
        "turn": 11
      }
    ],
    "play_stacks": [
      0,
      0,
      2,
      1
    ],
    "strikes": 0,
    "turn_count": 12,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          0,
          0,
          1,
          1
        ],
        "clue_value": 4,
        "focus_slot": 4,
        "giver": 1,
        "inverted": false,
        "react_order": 14,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          19,
          18,
          4,
          3,
          1
        ],
        "rlocks": false,
        "turn": 11
      }
    ]
  },
  "game_id": 4947,
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
        "rank": 2,
        "suit": 0,
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
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 5,
        "suit": 3,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "giver": 1,
        "kind": "C",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 20,
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
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
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
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
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
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 6,
        "max": 20,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 3,
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
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 20,
        "score": 3,
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
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 20,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 20,
        "score": 3,
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
          3,
          19
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
        "max": 20,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
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
        0,
        2
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
        2,
        1
      ],
      [
        0,
        2
      ],
      [
        3,
        4
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
        3,
        1
      ],
      null,
      null,
      null,
      null,
      null,
      [
        2,
        2
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
        1,
        4
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
      "variant_name": "Throw It in a Hole & Brown (4 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Brown (4 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-29T17:45:14.148",
  "turn": 12
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // Our o12 is the standing play that inverts the roles: called as the b3 by
  // yagami's T8 Blue, although its touches still allow b1-b5.
  EXPECT_EQ(game.meta[12].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_FALSE(hanabi::reactor::variants::has_known_play(game, 2))
      << "guard: the touches alone do not name it playable (v18.0.0's test)";

  // So the 4 to will-bot67 was stable: nothing of ours is called as its reaction.
  EXPECT_NE(game.meta[14].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "o14 (an r3) is not the reaction to a reactive that was never given";
  EXPECT_TRUE(game.waiting.empty()) << "and no reactive is waiting on us";

  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "we play the b3 we were called to play";
  EXPECT_EQ(play->target, 12);
}
