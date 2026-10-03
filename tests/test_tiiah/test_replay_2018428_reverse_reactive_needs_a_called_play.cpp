// Replay 2018428 T16-T18 (tiiah/CONVENTION.md §1c, v20.3.0). Black's T7 1 had called
// will-bot69's o15 and only touched o5, `{r1,y1,g1,b1,p1,ra1}`. At T16 black's 1
// re-touched o5 and touched the new o23. o5 was never called, and the hole could hold
// any of its 1s (no good touch on ancillary cards), so will-bot69 held no standing
// play: the clue was stable, not a reverse reactive. And will-bot69's T17 play of the
// newly clued o23 would have withdrawn a reverse reactive anyway. will-bot67 used to
// blind-play its o11 as a g2 here, into a strike.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole & Rainbow (6 Suits). 3 players, our_player_index=2.

TEST(TiiahReplay2018428, ReverseReactiveNeedsACalledPlay) {
  // Reconstruct exactly the Game the live bot saw at turn 18.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2018428,
  "debug": {
    "cards_left": 35,
    "clue_tokens": 4,
    "common_play_stacks": [
      0,
      0,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 20,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 16,
        "rank": 3,
        "suit": 4
      },
      {
        "order": 4,
        "rank": 3,
        "suit": 5
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
              4,
              2
            ],
            "inferred": 35651584,
            "info_lock": 35651584,
            "order": 22,
            "possible": 1072693248,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 1072693248,
            "info_lock": null,
            "order": 3,
            "possible": 1072693248,
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
            "inferred": 1048575,
            "info_lock": null,
            "order": 2,
            "possible": 1048575,
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
            "inferred": 1048575,
            "info_lock": null,
            "order": 1,
            "possible": 1048575,
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
              3
            ],
            "inferred": 1048575,
            "info_lock": null,
            "order": 0,
            "possible": 1048575,
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
            "id": [
              0,
              1
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 24,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              4
            ],
            "inferred": 1039104990,
            "info_lock": null,
            "order": 17,
            "possible": 1039104990,
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
              3
            ],
            "inferred": 1039104990,
            "info_lock": null,
            "order": 7,
            "possible": 1039104990,
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
              4
            ],
            "inferred": 1039104990,
            "info_lock": null,
            "order": 6,
            "possible": 1039104990,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              1
            ],
            "inferred": 34636833,
            "info_lock": null,
            "order": 5,
            "possible": 34636833,
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 21,
            "possible": 1073741823,
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
            "order": 19,
            "possible": 32538623,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 8397064,
            "info_lock": null,
            "order": 18,
            "possible": 8397064,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 24141559,
            "info_lock": null,
            "order": 12,
            "possible": 24141559,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 2048,
            "info_lock": 2099233,
            "order": 11,
            "possible": 24141559,
            "slot": 5,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
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
        0,
        1,
        2,
        1,
        1
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
    "pending_reactions": [
      {
        "clue_play_stacks": [
          0,
          0,
          0,
          0,
          0,
          0
        ],
        "focus_slot": 1,
        "giver": 0,
        "reacter": 2,
        "receiver": 1,
        "receiver_hand": [
          23,
          17,
          7,
          6,
          5
        ],
        "turn": 16
      }
    ],
    "play_stacks": [
      0,
      0,
      1,
      2,
      1,
      1
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 8,
        "superposition": 33792
      },
      {
        "holder": 1,
        "order": 9,
        "superposition": 99328,
        "support": [
          [
            2,
            2,
            1
          ],
          [
            3,
            1,
            1
          ],
          [
            2,
            1,
            2
          ],
          [
            3,
            2,
            2
          ]
        ],
        "worlds": [
          [
            [
              8,
              2,
              1
            ]
          ],
          [
            [
              8,
              3,
              1
            ]
          ]
        ]
      },
      {
        "holder": 2,
        "order": 13,
        "superposition": 33
      },
      {
        "holder": 2,
        "order": 14,
        "superposition": 66,
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
              13,
              0,
              1
            ]
          ],
          [
            [
              13,
              1,
              1
            ]
          ]
        ]
      },
      {
        "holder": 1,
        "order": 15,
        "superposition": 34636833
      },
      {
        "holder": 1,
        "order": 23,
        "superposition": 34636833
      }
    ],
    "turn_count": 18,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_play_stacks": [
          0,
          0,
          0,
          0,
          0,
          0
        ],
        "clue_value": 1,
        "focus_slot": 1,
        "giver": 0,
        "inverted": false,
        "react_order": 11,
        "reacter": 2,
        "receiver": 1,
        "receiver_hand": [
          23,
          17,
          7,
          6,
          5
        ],
        "rlocks": false,
        "turn": 16
      }
    ]
  },
  "game_id": 10686,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 0,
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
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
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
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 8,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
        "order": 13,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
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
        "kind": "C",
        "list": [
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 6,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
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
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
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
        "order": 18,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 4,
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
          5,
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
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
        "kind": "R",
        "list": [
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 4,
        "max": 30,
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
        "order": 16,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
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
        "kind": "C",
        "list": [
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 4,
        "max": 30,
        "score": 4,
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
        "order": 20,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
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
        "order": 21,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
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
        "order": 4,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
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
        "order": 20,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
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
        "kind": "C",
        "list": [
          3,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
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
        "kind": "R",
        "list": [
          5,
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 30,
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
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
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
        0,
        3
      ],
      [
        2,
        2
      ],
      [
        0,
        1
      ],
      [
        5,
        3
      ],
      [
        5,
        3
      ],
      [
        1,
        1
      ],
      [
        0,
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
        3,
        1
      ],
      null,
      null,
      null,
      null,
      null,
      [
        4,
        1
      ],
      [
        4,
        3
      ],
      [
        5,
        4
      ],
      null,
      null,
      [
        4,
        1
      ],
      null,
      [
        4,
        2
      ],
      [
        5,
        1
      ],
      [
        0,
        1
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
      "variant_name": "Throw It in a Hole & Rainbow (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Rainbow (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-03T20:16:15.301",
  "turn": 18
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  EXPECT_NE(game.meta[11].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "o11 was never called: black's 1 was no reverse reactive";
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  EXPECT_FALSE(play && play->target == 11) << "no blind play of o11";
}
