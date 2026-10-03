// Replay 2018435 T11 (reactor0/CONVENTION.md §1b priority 5; tiiah/CONVENTION.md
// §1e; v20.4.0). Will-bot69, locked, could give Yellow to yagami_black. Yellow
// re-touches o14, a clued ra1, and newly touches o13, the y1.
//  - A stable colour clue focuses its NEWLY touched cards first, so Yellow calls
//    o13, not the rainbow o14.
//  - The call is no longer widened by the giver's own hole cards, but black's own
//    o12 `{r1,y1,g2,g3}` still widens it to `{y1,y2}`. Black cannot name its card,
//    so Yellow cannot be given, and Green is the better clue (the user's ruling).
// Until v20.4.0 Yellow named o14, Green was widened by will-bot69's own hole card,
// and will-bot69 locked black with a 3.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (6 Suits). 3 players, our_player_index=1.

TEST(TiiahReplay2018435, YellowFocusesNewlyTouchedCard) {
  // Reconstruct exactly the Game the live bot saw at turn 11.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 2018435,
  "debug": {
    "cards_left": 40,
    "clue_tokens": 5,
    "common_play_stacks": [
      0,
      0,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 1,
    "discards": [
      {
        "order": 3,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 3,
        "suit": 1
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
              0,
              4
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 18,
            "possible": 1048575,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              5,
              5
            ],
            "inferred": 553648128,
            "info_lock": null,
            "order": 4,
            "possible": 553648128,
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
              2
            ],
            "inferred": 507375,
            "info_lock": null,
            "order": 2,
            "possible": 507375,
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
            "inferred": 507375,
            "info_lock": null,
            "order": 1,
            "possible": 507375,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              1
            ],
            "inferred": 1048576,
            "info_lock": 34603008,
            "order": 0,
            "possible": 519045120,
            "slot": 5,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 796647159,
            "info_lock": null,
            "order": 19,
            "possible": 796647159,
            "slot": 1,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 762010326,
            "info_lock": null,
            "order": 9,
            "possible": 762010326,
            "slot": 2,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 762010326,
            "info_lock": null,
            "order": 8,
            "possible": 762010326,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 277094664,
            "info_lock": null,
            "order": 7,
            "possible": 277094664,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 277094664,
            "info_lock": null,
            "order": 6,
            "possible": 277094664,
            "slot": 5,
            "status": "CHOP_MOVED",
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
            "id": [
              0,
              1
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 17,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              1
            ],
            "inferred": 1041203200,
            "info_lock": null,
            "order": 14,
            "possible": 1041203200,
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
            "clued": false,
            "focused": false,
            "id": [
              4,
              3
            ],
            "inferred": 32538623,
            "info_lock": null,
            "order": 11,
            "possible": 32538623,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 1041203200,
            "info_lock": null,
            "order": 10,
            "possible": 1041203200,
            "slot": 5,
            "status": "NONE",
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
        "k": "play",
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
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Lock"
      }
    ],
    "pairwise_play_stacks": [
      [
        1,
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
        0,
        0,
        0
      ],
      [
        0,
        0,
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
      0,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 5,
        "superposition": 33588256
      },
      {
        "holder": 2,
        "order": 12,
        "superposition": 6177
      },
      {
        "holder": 1,
        "order": 15,
        "superposition": 101376,
        "support": [
          [
            2,
            1,
            13
          ],
          [
            3,
            1,
            11
          ],
          [
            2,
            2,
            2
          ],
          [
            3,
            2,
            4
          ]
        ],
        "worlds": [
          [
            [
              5,
              1,
              1
            ]
          ],
          [
            [
              5,
              2,
              1
            ]
          ],
          [
            [
              5,
              3,
              1
            ]
          ],
          [
            [
              5,
              5,
              1
            ]
          ]
        ]
      }
    ],
    "turn_count": 11,
    "waiting": []
  },
  "game_id": 10695,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 5,
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
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 7,
        "max": 30,
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
        "max": 30,
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
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 6,
        "max": 30,
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
          10,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 5,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
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
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
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
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 1,
        "rank": 3,
        "suit": 1,
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
        "clues": 7,
        "max": 30,
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
        "kind": "C",
        "list": [
          0,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
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
          6,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        4,
        1
      ],
      [
        2,
        3
      ],
      [
        0,
        2
      ],
      [
        0,
        2
      ],
      [
        5,
        5
      ],
      null,
      null,
      null,
      null,
      null,
      [
        5,
        3
      ],
      [
        4,
        3
      ],
      [
        0,
        1
      ],
      [
        1,
        1
      ],
      [
        5,
        1
      ],
      null,
      [
        1,
        3
      ],
      [
        0,
        1
      ],
      [
        0,
        4
      ],
      null
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
      "variant_name": "Throw It in a Hole & Rainbow (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T20:30:17.352",
  "turn": 11
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  // Yellow, as black would read it: o13 called, as exactly the y1.
  const auto& black = game.state.hands[2];
  hanabi::ClueAction yellow{1, 2,
                            game.state.clue_touched(black, hanabi::ClueKind::COLOUR, 1),
                            hanabi::BaseClue{hanabi::ClueKind::COLOUR, 1}};
  const hanabi::Game hypo = game.simulate(hanabi::Action{yellow});
  EXPECT_EQ(hypo.meta[13].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the newly touched o13 is the focus";
  EXPECT_NE(hypo.meta[14].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "not the re-touched rainbow o14";
  // {y1,y2}: not widened by will-bot69's own hole card, but by black's OWN o12
  // `{r1,y1,g2,g3}`, which black cannot name -- in the world where it was the y1,
  // will-bot69 would see yellow on 1. Black cannot name its card, so Yellow is not
  // given (§2c), and Green is the better clue (the user's ruling).
  EXPECT_EQ(hypo.common.thoughts[13].inferred,
            hanabi::IdentitySet::single(hanabi::Identity{1, 1})
                .add(hanabi::Identity{1, 2}));

  const auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_TRUE(colour) << "a colour stable play, not the 3 lock";
  EXPECT_EQ(colour->target, 2);
  EXPECT_EQ(colour->value, 2) << "Green";
}
