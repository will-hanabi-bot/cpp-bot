// A required play with ONE card still in the deck.
//
// Turn 25, "Odds and Evens & Rainbow (3 Suits)". Stacks [3,5,4] at 12 of 15,
// ZERO clues, one strike, one card left. Seats: us (p0), yagami (p1),
// will-bot69 (p2).
//
// will-bot69 holds BOTH remaining playables -- the other r4 (stamped CTP,
// reading {r4, ra5}) and the critical ra5 -- but gets a single turn, and with
// no clues there is no way to stall round for a second one. Their r4 is dead
// whatever they choose. yagami's whole hand is trash. So our copy is the only
// r4 that can ever be played, and playing it is what makes the rest work:
//
//   us   play slot 4 -> r4          (draws the last card, opening the round)
//   p1   all trash, discards
//   p2   plays the ra5              rainbow to 5
//   us   play slot 5 -> r5          15/15
//
// Our slot 4 (order 1) IS the r4 and slot 5 (order 0) IS the r5, so the line is
// real, not hopeful. The bot discarded a trash b4 and the game ended at 12.
//
// Rule 0b already asked the right question and picked the right card, but was
// called only at `cards_left == 0` and bailed on an unset `endgame_turns`. Rule
// 0c is the same question one card earlier, with a narrower candidate test to
// carry the weaker signal: the card must be CLUED and everything it could be
// that is not already trash must be the one required identity. Slot 4 reads
// {r2,r4,ra2,ra4} and the stacks kill all but the r4. Slot 1 has the identical
// reading but is unclued, which is exactly why it is not the answer.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/action.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Rainbow (3 Suits). 3 players, our_player_index=0.

TEST(EndgameReplay1972670, RequiredPlayWithOneCardLeft) {
  // Reconstruct exactly the Game the live bot saw at turn 25.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1972670,
  "debug": {
    "cards_left": 1,
    "clue_tokens": 0,
    "current_player_index": 0,
    "discards": [
      {
        "order": 23,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 1,
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
            "id": null,
            "inferred": 10250,
            "info_lock": null,
            "order": 24,
            "possible": 10250,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 256,
            "info_lock": null,
            "order": 20,
            "possible": 256,
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
              1
            ],
            "inferred": 32,
            "info_lock": null,
            "order": 2,
            "possible": 32,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 10250,
            "info_lock": null,
            "order": 1,
            "possible": 10250,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 21525,
            "info_lock": null,
            "order": 0,
            "possible": 21525,
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
            "clued": true,
            "focused": false,
            "id": [
              2,
              4
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 27,
            "possible": 31744,
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
              3
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 25,
            "possible": 31744,
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
            "inferred": 31,
            "info_lock": null,
            "order": 21,
            "possible": 31,
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
            "inferred": 27648,
            "info_lock": null,
            "order": 17,
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
              0,
              1
            ],
            "inferred": 31,
            "info_lock": null,
            "order": 8,
            "possible": 31,
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
            "clued": true,
            "focused": true,
            "id": [
              0,
              4
            ],
            "inferred": 16392,
            "info_lock": 16392,
            "order": 28,
            "possible": 31775,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 23,
            "info_lock": null,
            "order": 26,
            "possible": 31,
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
            "inferred": 128,
            "info_lock": null,
            "order": 18,
            "possible": 160,
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
              5
            ],
            "inferred": 20480,
            "info_lock": null,
            "order": 16,
            "possible": 21504,
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
            "inferred": 160,
            "info_lock": null,
            "order": 10,
            "possible": 160,
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
        "v": "Stall"
      },
      {
        "k": "clue",
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Stall"
      }
    ],
    "play_stacks": [
      3,
      5,
      4
    ],
    "strikes": 1,
    "turn_count": 25,
    "waiting": []
  },
  "game_id": 4737,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 1,
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
        "rank": 4,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          11,
          12,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 7,
        "max": 15,
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
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 15,
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
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 15,
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
          13,
          16
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
        "max": 15,
        "score": 2,
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
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
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
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
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
          10,
          13,
          16,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 15,
        "score": 4,
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
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 5,
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
          5,
          9,
          17,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 15,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          1,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 3,
        "max": 15,
        "score": 7,
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
        "suit": 0,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 15,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          0,
          2,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 2,
        "max": 15,
        "score": 8,
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
        "rank": 4,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 15,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 15,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "num": 1,
        "order": 19,
        "t": "strike",
        "turn": 16
      },
      {
        "failed": true,
        "order": 19,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 15,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 15,
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
        "kind": "C",
        "list": [
          10,
          13,
          16,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 2,
        "max": 15,
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
        "suit": 2,
        "t": "play"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 15,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
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
        "order": 28,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 15,
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
          17,
          25,
          27
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 2,
        "max": 15,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          16,
          26,
          28
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 1,
        "max": 15,
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
          0,
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 0,
        "max": 15,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      null,
      null,
      [
        1,
        1
      ],
      [
        0,
        3
      ],
      [
        0,
        2
      ],
      [
        2,
        4
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
        0,
        1
      ],
      [
        1,
        3
      ],
      [
        1,
        1
      ],
      [
        2,
        2
      ],
      [
        1,
        4
      ],
      [
        1,
        5
      ],
      [
        0,
        1
      ],
      [
        2,
        1
      ],
      [
        2,
        5
      ],
      [
        2,
        2
      ],
      [
        1,
        3
      ],
      [
        2,
        1
      ],
      null,
      [
        0,
        1
      ],
      [
        2,
        3
      ],
      [
        1,
        2
      ],
      null,
      [
        2,
        3
      ],
      [
        0,
        3
      ],
      [
        2,
        4
      ],
      [
        0,
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
      "variant_name": "Odds and Evens & Rainbow (3 Suits)"
    },
    "our_player_index": 0,
    "reactive_overrides": [
      {
        "clue_value": 1,
        "even": true,
        "kind": "R",
        "reactive_value": 5
      },
      {
        "clue_value": 2,
        "even": true,
        "kind": "R",
        "reactive_value": 2
      }
    ],
    "rlocks": true,
    "variant": "Odds and Evens & Rainbow (3 Suits)",
    "zcs_turn": 24
  },
  "ts": "2026-08-25T20:40:15.871",
  "turn": 25
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  const hanabi::State& s = game.state;
  ASSERT_EQ(s.cards_left, 1) << "guard: rule 0c's window";
  ASSERT_EQ(s.clue_tokens, 0)
      << "guard: no clues, so nobody can stall round for a second will-bot69 "
         "turn -- which is what kills their copy of the r4";

  // Slot 4's readings minus trash are exactly {r4}: the narrow candidate test.
  const hanabi::IdentitySet live = game.me().thoughts[1].possibilities().filter(
      [&s](hanabi::Identity i) { return !s.is_basic_trash(i); });
  ASSERT_EQ(live.length(), 1);
  EXPECT_EQ(live.head(), (hanabi::Identity{0, 4}));

  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "expected the r4, got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 1)
      << "slot 4 -- the leftmost CLUED card that could be the required r4";
}
