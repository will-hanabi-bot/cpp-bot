// The same defect on the other branches, and it cost a critical card.
//
// Turn 42, "Pink-Ones & Dark Orange (6 Suits)", stacks [5,3,5,3,2,1].
// will-bot67 holds a playable p3 in slot 4 (order 19) -- purple is on 2 -- and
// is the reacter of a pending reactive clue, so playing it names Neema's slot.
//
// It could not see the p3: slot 4 read `inferred = {y2, b2}` against
// `possible = {y2, b2, b3, p2, p3, d2, d3}`. The p3 had been eliminated across
// two reactions -- the T19 finesse (reacter played the r4, receiver the r5,
// both red) and the T22 reaction -- by negatives that never checked whether the
// reacter's paired slot could have supplied the reading they assumed.
//
// Holding no playable it could see, the bot chucked its urgent CTD instead.
// That card was a **Dark Orange 4**: an inverted suit, so Discard is a play
// attempt, and with the dark stack on 1 it struck. Max score fell to 28.
//
// Both halves are fixed by the same change, so this test asserts the play.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/action.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Pink-Ones & Dark Orange (6 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1970589, ElimLeavesThePlayableItCannotRuleOut) {
  // Reconstruct exactly the Game the live bot saw at turn 42.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1970589,
  "debug": {
    "cards_left": 14,
    "clue_tokens": 2,
    "current_player_index": 2,
    "discards": [
      {
        "order": 25,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 23,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 8,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 32,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 0,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 27,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 16,
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
            "focused": false,
            "id": [
              3,
              1
            ],
            "inferred": 278168873,
            "info_lock": null,
            "order": 40,
            "possible": 278168873,
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
            "inferred": 278168873,
            "info_lock": null,
            "order": 37,
            "possible": 278168873,
            "slot": 2,
            "status": "NONE",
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
            "inferred": 278168873,
            "info_lock": null,
            "order": 35,
            "possible": 278168873,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              5
            ],
            "inferred": 761987650,
            "info_lock": null,
            "order": 22,
            "possible": 761987650,
            "slot": 4,
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
            "inferred": 277086464,
            "info_lock": null,
            "order": 3,
            "possible": 277086464,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "Neema",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              1,
              1
            ],
            "inferred": 1040160619,
            "info_lock": null,
            "order": 38,
            "possible": 1040160619,
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
            "inferred": 138547200,
            "info_lock": null,
            "order": 28,
            "possible": 138547200,
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
            "inferred": 69271618,
            "info_lock": null,
            "order": 26,
            "possible": 69271618,
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
              5
            ],
            "inferred": 554172928,
            "info_lock": null,
            "order": 15,
            "possible": 554172928,
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
              3
            ],
            "inferred": 134352896,
            "info_lock": null,
            "order": 6,
            "possible": 134352896,
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
            "inferred": 1040160619,
            "info_lock": null,
            "order": 39,
            "possible": 1040160619,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1040160619,
            "info_lock": null,
            "order": 30,
            "possible": 1040160619,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 9471017,
            "info_lock": null,
            "order": 29,
            "possible": 278168873,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 65600,
            "info_lock": null,
            "order": 19,
            "possible": 207814720,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 554172928,
            "info_lock": null,
            "order": 11,
            "possible": 554172928,
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
        "v": "Play"
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
        "k": "discard",
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
        "v": "Stall"
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
        "v": "Reactive"
      }
    ],
    "play_stacks": [
      5,
      3,
      5,
      3,
      2,
      1
    ],
    "strikes": 0,
    "turn_count": 42,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_value": 4,
        "focus_slot": 4,
        "giver": 1,
        "inverted": false,
        "react_order": 29,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          40,
          37,
          35,
          22,
          3
        ],
        "rlocks": true,
        "turn": 41
      }
    ]
  },
  "game_id": 2164,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
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
        "rank": 4,
        "suit": 2,
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
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 5,
        "suit": 3,
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
        "kind": "C",
        "list": [
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 0
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
        "kind": "R",
        "list": [
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
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 5,
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
        "clues": 6,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
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
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 0,
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
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 5,
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
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 17,
        "p": 2,
        "rank": 1,
        "suit": 3,
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
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
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
          10,
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
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
          15,
          16
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 2,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 0,
        "rank": 3,
        "suit": 1,
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
        "num": 16,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 25,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          10,
          11,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 1,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 27,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
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
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
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
        "value": 4
      },
      {
        "clues": 1,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          29
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 0,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          2,
          31
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 33,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 32,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          26,
          34
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 35,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 34,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 1,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 37,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 38,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 39,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          3,
          35,
          37,
          40
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 2,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        2,
        4
      ],
      [
        0,
        1
      ],
      [
        2,
        4
      ],
      [
        3,
        4
      ],
      [
        1,
        2
      ],
      [
        0,
        4
      ],
      [
        2,
        3
      ],
      [
        3,
        2
      ],
      [
        1,
        4
      ],
      [
        4,
        1
      ],
      [
        0,
        5
      ],
      null,
      [
        2,
        5
      ],
      [
        0,
        2
      ],
      [
        5,
        1
      ],
      [
        3,
        5
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
        1,
        1
      ],
      null,
      [
        2,
        1
      ],
      [
        0,
        3
      ],
      [
        1,
        5
      ],
      [
        1,
        3
      ],
      [
        1,
        3
      ],
      [
        0,
        3
      ],
      [
        1,
        2
      ],
      [
        3,
        1
      ],
      [
        5,
        3
      ],
      null,
      null,
      [
        2,
        2
      ],
      [
        2,
        2
      ],
      [
        2,
        3
      ],
      [
        4,
        2
      ],
      [
        0,
        4
      ],
      [
        3,
        3
      ],
      [
        2,
        1
      ],
      [
        1,
        1
      ],
      null,
      [
        3,
        1
      ]
    ],
    "names": [
      "Neema",
      "yagami_black",
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
      "variant_name": "Pink-Ones & Dark Orange (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Pink-Ones & Dark Orange (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-24T05:31:18.908",
  "turn": 42
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  EXPECT_TRUE(game.common.thoughts[19].inferred.contains(hanabi::Identity{4, 3}))
      << "the p3 must survive the two reactions' negatives";

  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "expected the p3 play, got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 19)
      << "slot 4 -- purple is on 2, so the p3 is playable, and playing it is "
         "also this seat's reaction";
}
