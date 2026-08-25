// The endgame decides WHETHER to clue; the convention decides WHICH.
//
// Turn 59, "Odds and Evens & Dark Prism (6 Suits)". Alice = will-bot67 (p1),
// Bob = yagami (p2), Cathy = will-bot69 (p0). Stacks [4,5,4,5,5,5], 28 of 30,
// one card left, three clues, one strike. Both missing cards are visible:
// **r5 in Bob's slot 4, g5 in Cathy's slot 1**. Alice has no playable, so the
// turn is a clue.
//
// The right clue is PURPLE TO CATHY. A colour clue to Cathy is reactive, and
// under Odds and Evens colour is the EVEN parity -- a double play. Purple's
// colour value is 5, and the pairing is `react_slot + target_slot = anchor
// (mod hand size)`: Bob's r5 sits in slot 4, Cathy's g5 in slot 1, and
// 4 + 1 = 5. Both lay, 28 becomes 30.
//
// What happened instead: the solver returned COLOUR RED TO BOB, touching his
// slot 2 (r1) and slot 4 (r5). Reactor0's stable-colour rule names the
// LEFTMOST touched card that could be playable -- slot 2 -- so Bob played the
// r1, took strike 2, and the game ended 29 with the r5 still in hand.
//
// Neither half of the right answer was new. `read_clue` already classifies
// purple-to-Cathy as REACTIVE_PLAY, and `predicts_a_strike` already rejects
// red-to-Bob. They simply never ran: the endgame fork returns before reactor0's
// candidate pool is built, and the TIMING for this turn shows
// `reactor0.analyse_clues calls = 0`.
//
// The solver cannot tell the two apart -- `find_all_clues` ranks with REACTOR's
// scorer even here, `clueless_winnable` prices every clue as a dummy token
// burn, and a partner whose call would strike is modelled as free to ignore it.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/action.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Dark Prism (6 Suits). 3 players, our_player_index=1.

TEST(EndgameReplay1971808, StallClueGoesToTheDoubleReactive) {
  // Reconstruct exactly the Game the live bot saw at turn 59.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1971808,
  "debug": {
    "cards_left": 1,
    "clue_tokens": 3,
    "current_player_index": 1,
    "discards": [
      {
        "order": 20,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 37,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 27,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 46,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 2,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 14,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 26,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 31,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 15,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 21,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 48,
        "rank": 2,
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
              2,
              5
            ],
            "inferred": 13760981,
            "info_lock": null,
            "order": 53,
            "possible": 13760981,
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
              2
            ],
            "inferred": 13760981,
            "info_lock": null,
            "order": 51,
            "possible": 13760981,
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
              3
            ],
            "inferred": 113093,
            "info_lock": null,
            "order": 44,
            "possible": 129493,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              3
            ],
            "inferred": 13631488,
            "info_lock": null,
            "order": 34,
            "possible": 13631488,
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
              3
            ],
            "inferred": 21,
            "info_lock": null,
            "order": 25,
            "possible": 21,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 13730245,
            "info_lock": null,
            "order": 52,
            "possible": 13730261,
            "slot": 1,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 13730261,
            "info_lock": null,
            "order": 49,
            "possible": 13730261,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 448,
            "info_lock": null,
            "order": 45,
            "possible": 448,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 448,
            "info_lock": null,
            "order": 41,
            "possible": 448,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 14336,
            "info_lock": null,
            "order": 39,
            "possible": 30720,
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
              1,
              3
            ],
            "inferred": 13760981,
            "info_lock": null,
            "order": 50,
            "possible": 13760981,
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
              1
            ],
            "inferred": 13760981,
            "info_lock": null,
            "order": 47,
            "possible": 13760981,
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
            "inferred": 13760981,
            "info_lock": null,
            "order": 33,
            "possible": 13760981,
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
            "inferred": 13730261,
            "info_lock": null,
            "order": 11,
            "possible": 13730261,
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
              4
            ],
            "inferred": 13730261,
            "info_lock": null,
            "order": 10,
            "possible": 13730261,
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
        "k": "discard",
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
        "k": "discard",
        "v": "None"
      }
    ],
    "play_stacks": [
      4,
      5,
      4,
      5,
      5,
      5
    ],
    "strikes": 1,
    "turn_count": 59,
    "waiting": []
  },
  "game_id": 3692,
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
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
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
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          5,
          7
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
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
        "kind": "C",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 4,
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
        "num": 4,
        "t": "turn"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 2,
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
        "giver": 2,
        "kind": "R",
        "list": [
          6,
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
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
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 4,
        "suit": 3,
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
        "giver": 2,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "order": 19,
        "p": 1,
        "rank": 2,
        "suit": 1,
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
        "max": 30,
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
          5,
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 4,
        "suit": 2,
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
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 8,
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
        "value": 2
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
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 1,
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
        "giver": 2,
        "kind": "C",
        "list": [
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 1,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
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
        "order": 24,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "giver": 2,
        "kind": "R",
        "list": [
          6,
          8,
          21,
          26
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 0,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          17,
          28
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 0,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "clues": 1,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 30,
        "p": 1,
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
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          6,
          21,
          26,
          30
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 0,
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
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
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
        "order": 30,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 33,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 31,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
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
        "order": 35,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 15,
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
          6,
          26,
          32
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 3,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 36,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "kind": "C",
        "list": [
          25,
          36
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 3,
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
        "order": 36,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 37,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          6,
          32,
          35
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 2,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 41,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          39
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 42,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 43,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          43
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 1,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 44,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "order": 35,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 45,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          41,
          45
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 1,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "order": 42,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 46,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          34,
          38
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 0,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 50,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 47,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 51,
        "t": "turn"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 5,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 48,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 52,
        "t": "turn"
      },
      {
        "order": 43,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 49,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 27,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 53,
        "t": "turn"
      },
      {
        "num": 1,
        "order": 14,
        "t": "strike",
        "turn": 53
      },
      {
        "failed": true,
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 50,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 27,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 54,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 46,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 51,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 27,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 55,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 5,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 52,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 28,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 56,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          39
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 2,
        "max": 30,
        "score": 28,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 57,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 48,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 53,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 28,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 58,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        4,
        4
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
        2,
        3
      ],
      [
        2,
        1
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
        1,
        1
      ],
      [
        4,
        3
      ],
      [
        2,
        2
      ],
      [
        4,
        4
      ],
      [
        0,
        5
      ],
      [
        1,
        3
      ],
      [
        1,
        5
      ],
      [
        2,
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
        5,
        2
      ],
      [
        3,
        2
      ],
      [
        1,
        2
      ],
      [
        0,
        1
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
        5,
        4
      ],
      [
        4,
        2
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
        0,
        4
      ],
      [
        0,
        2
      ],
      [
        5,
        3
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
        3,
        3
      ],
      [
        2,
        2
      ],
      [
        4,
        3
      ],
      [
        3,
        4
      ],
      [
        5,
        1
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
      [
        4,
        5
      ],
      null,
      [
        0,
        3
      ],
      [
        0,
        4
      ],
      [
        2,
        3
      ],
      null,
      [
        1,
        1
      ],
      [
        0,
        1
      ],
      [
        4,
        2
      ],
      null,
      [
        1,
        3
      ],
      [
        3,
        2
      ],
      null,
      [
        2,
        5
      ]
    ],
    "names": [
      "will-bot69",
      "will-bot67",
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
      "variant_name": "Odds and Evens & Dark Prism (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": false,
    "variant": "Odds and Evens & Dark Prism (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-25T04:05:58.811",
  "turn": 59
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  const hanabi::State& s = game.state;
  ASSERT_LE(s.rem_score(), static_cast<int>(s.variant->suits.size()) + 1)
      << "guard: the endgame fork's points half is open";
  ASSERT_LE(s.pace(), s.num_players) << "guard: and its pace half";
  ASSERT_GT(s.clue_tokens, 0) << "guard: a clue is available";

  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformColour>(action))
      << "expected a colour clue, got " << hanabi::to_json(action, 0).dump();
  const auto& c = std::get<hanabi::PerformColour>(action);
  EXPECT_EQ(c.target, 0) << "purple goes to CATHY (will-bot69) -- a reactive "
                            "clue, which under Odds and Evens is the even "
                            "parity and therefore a double play";
  EXPECT_EQ(c.value, 4) << "purple; red (value 0) to Bob names his leftmost "
                           "touched card that could be playable, an r1";
}
