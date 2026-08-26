// An expendable card on an INVERTED suit can be named as a reactive target --
// the receiver sheds it by pressing PLAY (a pitch), not DISCARD.
//
// Replay 1974257 T30, "Pink-Ones & Orange (6 Suits)", stacks r1 y1 g1 b1 p3 o1.
// yagami clues GREEN to will-bot67: anchor 3, and colour is the odd bucket, so
// exactly one play and the two seats press opposite buttons.
//
// Target selection runs playable-then-trash, leftmost-first. will-bot67 has no
// playable card at all (o3, o3, g5, y5, o4), so the first trash target is the
// LEFTMOST of his two orange 3s -- a same-hand dupe -- at slot 1.
// calc_slot(3, 1, 5) = 2, so will-bot69 reacts on slot 2.
//
// He sheds an orange by PITCHING it, which is the Play button; odd parity
// opposes, so will-bot69 presses DISCARD. Order 26 is a y3 and his slot 5 is
// the other y3, so that throw costs nothing either.
//
// Before v10.6.0 `dc_candidates` excluded inverted cards from the target pool
// outright -- correct about the Discard button, blind to the other one -- so
// every expendable card will-bot67 had was invisible, the pool came back EMPTY,
// the clue read as a MISTAKE, and phase 2 fell to rung 12 and threw the chop.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Pink-Ones & Orange (6 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1974257, AnInvertedTrashTargetIsNamedAndPitched) {
  // Reconstruct exactly the Game the live bot saw at turn 30.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1974257,
  "debug": {
    "cards_left": 30,
    "clue_tokens": 1,
    "current_player_index": 2,
    "discards": [
      {
        "order": 8,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 4,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 10,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 0,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 27,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 5,
        "rank": 1,
        "suit": 5
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
              5,
              3
            ],
            "inferred": 1073709567,
            "info_lock": null,
            "order": 29,
            "possible": 1073709567,
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
              3
            ],
            "inferred": 688521366,
            "info_lock": null,
            "order": 23,
            "possible": 761987286,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              5
            ],
            "inferred": 16384,
            "info_lock": null,
            "order": 16,
            "possible": 16384,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 512,
            "info_lock": null,
            "order": 3,
            "possible": 512,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              4
            ],
            "inferred": 268697608,
            "info_lock": null,
            "order": 2,
            "possible": 277086216,
            "slot": 5,
            "status": "CHOP_MOVED",
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
              2,
              2
            ],
            "inferred": 900540762,
            "info_lock": null,
            "order": 20,
            "possible": 900540762,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              3
            ],
            "inferred": 4329604,
            "info_lock": null,
            "order": 15,
            "possible": 4329604,
            "slot": 2,
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
            "inferred": 10824010,
            "info_lock": null,
            "order": 9,
            "possible": 10824010,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              2,
              3
            ],
            "inferred": 4329604,
            "info_lock": null,
            "order": 7,
            "possible": 4329604,
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
              2
            ],
            "inferred": 10824010,
            "info_lock": null,
            "order": 6,
            "possible": 10824010,
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
            "inferred": 1073724927,
            "info_lock": null,
            "order": 28,
            "possible": 1073724927,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073724927,
            "info_lock": null,
            "order": 26,
            "possible": 1073724927,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073724927,
            "info_lock": null,
            "order": 22,
            "possible": 1073724927,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 15360,
            "info_lock": null,
            "order": 13,
            "possible": 15360,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 480,
            "info_lock": null,
            "order": 12,
            "possible": 480,
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
      5,
      5
    ],
    "move_history": [
      {
        "k": "clue",
        "v": "Reactive"
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Reactive"
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
        "k": "clue",
        "v": "Reactive"
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
        "k": "clue",
        "v": "Lock"
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
        "k": "clue",
        "v": "Reactive"
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
    "play_stacks": [
      1,
      1,
      1,
      1,
      3,
      1
    ],
    "strikes": 0,
    "turn_count": 30,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_value": 2,
        "focus_slot": 3,
        "giver": 1,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          29,
          23,
          16,
          3,
          2
        ],
        "rlocks": false,
        "turn": 29
      }
    ]
  },
  "game_id": 141,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 1,
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
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 2,
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
          11,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
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
        "kind": "R",
        "list": [
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          1,
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 7,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          12
        ],
        "t": "clue",
        "target": 2,
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
        "num": 7,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          3,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 3,
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
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          17,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
        "score": 2,
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
          2,
          4,
          19
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
        "max": 30,
        "score": 2,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 3,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 4,
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
        "score": 3,
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
          7,
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16,
          19
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 24,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          2,
          25
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 2,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 26,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 28,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        3,
        4
      ],
      [
        5,
        1
      ],
      [
        5,
        4
      ],
      [
        1,
        5
      ],
      [
        1,
        4
      ],
      [
        5,
        1
      ],
      [
        4,
        2
      ],
      [
        2,
        3
      ],
      [
        0,
        4
      ],
      [
        2,
        2
      ],
      [
        3,
        1
      ],
      [
        2,
        1
      ],
      null,
      null,
      [
        3,
        1
      ],
      [
        4,
        3
      ],
      [
        2,
        5
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
        4
      ],
      [
        2,
        2
      ],
      [
        4,
        3
      ],
      null,
      [
        5,
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
      null,
      [
        4,
        1
      ],
      null,
      [
        5,
        3
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
      "variant_name": "Pink-Ones & Orange (6 Suits)"
    },
    "our_player_index": 2,
    "reactive_overrides": [
      {
        "clue_value": 5,
        "even": true,
        "kind": "C",
        "reactive_value": 1
      }
    ],
    "rlocks": false,
    "variant": "Pink-Ones & Orange (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T23:13:03.707",
  "turn": 30
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards ---------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 2) << "we are will-bot69, the reacter";
  ASSERT_EQ(s.play_stacks[5], 1) << "orange on 1, so neither o3 is playable";
  ASSERT_EQ(s.deck[29].id(), (hanabi::Identity{5, 3}))
      << "will-bot67 slot 1 is an o3";
  ASSERT_EQ(s.deck[23].id(), (hanabi::Identity{5, 3}))
      << "and slot 2 is the other one -- a same-hand dupe, so the leftmost is "
         "expendable";
  ASSERT_EQ(s.hands[2][1], 26) << "order 26 is OUR slot 2";
  ASSERT_EQ(s.hands[2][0], 28) << "order 28 is our chop, what the bug threw";

  // The clue must have been READ. Before the fix it was a MISTAKE, which is
  // why nothing was stamped and the ladder fell through to rung 12.
  ASSERT_FALSE(game.waiting.empty()) << "the reactive connection must survive";
  EXPECT_EQ(game.waiting.front().react_order, 26)
      << "the reading pairs will-bot67's leftmost o3 with our slot 2";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformDiscard>(action))
      << "odd parity opposes the receiver's PLAY, so we press Discard -- got "
      << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformDiscard>(action).target, 26)
      << "slot 2, the slot the anchor pairs with his slot 1";
  EXPECT_NE(std::get<hanabi::PerformDiscard>(action).target, 28)
      << "order 28 is the chop -- throwing it is the rung-12 fallback the "
         "MISTAKE reading produced";
}
