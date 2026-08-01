// Replay 1942330 turn 33 (reactor0; hanab.live shows it as #34). Bob's chop
// is a playable Navy 2, but a copy of it sits in Cathy's hand, so
// `at_risk_chop` reports it as NOT endangered and tier conditions H1/N1 stay
// silent. With H2/H3 and N2/N3 also inapplicable, the v2.2.0 pace-clue tier
// gate rated EVERY clue LOW and returned a flat -1.0 for all 13 of them; the
// argmax then picked the first candidate in enumeration order, which was a
// LOCK. CONVENTION.md §2a condition N5 fixes this: a playable, non-duped
// chop on Bob makes any clue at least MEDIUM.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Extremely Ambiguous & Muddy Rainbow (6 Suits). 3 players, our_player_index=2.

TEST(DecisionMaking1942330, T33PrefersPlayClueOverLockWhenBobsChopIsPlayable) {
  // Reconstruct exactly the Game the live bot saw at turn 33.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1942330,
  "debug": {
    "cards_left": 25,
    "clue_tokens": 3,
    "current_player_index": 2,
    "discards": [
      {
        "order": 31,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 9,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 8,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 15,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 1,
        "rank": 4,
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
              4,
              2
            ],
            "inferred": 1073725434,
            "info_lock": null,
            "order": 33,
            "possible": 1073725434,
            "slot": 1,
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
            "inferred": 1073725434,
            "info_lock": null,
            "order": 27,
            "possible": 1073725434,
            "slot": 2,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              5
            ],
            "inferred": 1036919760,
            "info_lock": null,
            "order": 21,
            "possible": 1073725434,
            "slot": 3,
            "status": "CHOP_MOVED",
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
            "inferred": 8659200,
            "info_lock": null,
            "order": 4,
            "possible": 8659208,
            "slot": 4,
            "status": "CHOP_MOVED",
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
            "inferred": 8659200,
            "info_lock": null,
            "order": 2,
            "possible": 8659208,
            "slot": 5,
            "status": "CHOP_MOVED",
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
              4,
              2
            ],
            "inferred": 1073725434,
            "info_lock": null,
            "order": 34,
            "possible": 1073725434,
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
              3
            ],
            "inferred": 1073725434,
            "info_lock": null,
            "order": 29,
            "possible": 1073725434,
            "slot": 2,
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
            "inferred": 1073725434,
            "info_lock": null,
            "order": 23,
            "possible": 1073725434,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              1
            ],
            "inferred": 1073725434,
            "info_lock": null,
            "order": 19,
            "possible": 1073725434,
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
              2
            ],
            "inferred": 1034756952,
            "info_lock": null,
            "order": 6,
            "possible": 1068313434,
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
            "inferred": 1073725434,
            "info_lock": null,
            "order": 32,
            "possible": 1073725434,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 17302016,
            "info_lock": null,
            "order": 30,
            "possible": 17302032,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 919965130,
            "info_lock": null,
            "order": 28,
            "possible": 1056423402,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 886418882,
            "info_lock": null,
            "order": 22,
            "possible": 1056423402,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 881143114,
            "info_lock": null,
            "order": 14,
            "possible": 1052093802,
            "slot": 5,
            "status": "CHOP_MOVED",
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
        "k": "clue",
        "v": "Lock"
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
        "k": "play",
        "v": "None"
      },
      {
        "k": "play",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "Sarcastic"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "play_stacks": [
      4,
      0,
      5,
      2,
      1,
      2
    ],
    "strikes": 0,
    "turn_count": 33,
    "waiting": []
  },
  "game_id": 3284,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 4,
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
        "rank": 4,
        "suit": 3,
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
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "rank": 3,
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
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 6,
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
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 5,
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
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 15,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
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
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 0,
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
        "giver": 2,
        "kind": "R",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 4,
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
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 17,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
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
          2,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 3,
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
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 2,
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
        "clues": 3,
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
        "kind": "R",
        "list": [
          10,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 2,
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
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 20,
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
        "num": 12,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 21,
        "p": 0,
        "rank": 5,
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
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          1,
          2,
          4,
          15,
          21
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 1,
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
        "order": 18,
        "p": 2,
        "rank": 2,
        "suit": 3,
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
        "clues": 1,
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
          10,
          11,
          14,
          20,
          22
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 0,
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
        "failed": false,
        "order": 16,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 1,
        "suit": 5,
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
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          11,
          14,
          22,
          24
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 0,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 4,
        "suit": 0,
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
        "clues": 1,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 10,
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
          1,
          2,
          4,
          21,
          27
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 1,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 5,
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
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          14,
          22,
          26,
          28
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 0,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
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
        "order": 29,
        "p": 1,
        "rank": 3,
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
        "num": 26,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
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
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          26,
          30
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 0,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 31,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 0,
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
        "order": 26,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 32,
        "p": 2,
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
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 4,
        "suit": 5,
        "t": "discard"
      },
      {
        "order": 33,
        "p": 0,
        "rank": 2,
        "suit": 4,
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
        "order": 31,
        "p": 1,
        "rank": 1,
        "suit": 0,
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
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        0,
        1
      ],
      [
        5,
        4
      ],
      [
        4,
        4
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
        3,
        1
      ],
      [
        1,
        2
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
        0,
        3
      ],
      [
        2,
        3
      ],
      [
        5,
        2
      ],
      [
        2,
        2
      ],
      [
        0,
        3
      ],
      null,
      [
        2,
        1
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
        3,
        2
      ],
      [
        5,
        1
      ],
      [
        5,
        1
      ],
      [
        4,
        5
      ],
      null,
      [
        4,
        3
      ],
      [
        0,
        4
      ],
      [
        2,
        4
      ],
      [
        2,
        5
      ],
      [
        4,
        3
      ],
      null,
      [
        1,
        3
      ],
      null,
      [
        0,
        1
      ],
      null,
      [
        4,
        2
      ],
      [
        4,
        2
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
      "variant_name": "Extremely Ambiguous & Muddy Rainbow (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Extremely Ambiguous & Muddy Rainbow (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-01T01:27:50.870",
  "turn": 33
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::REACTOR0)
      << "the snapshot must replay under reactor0";

  const hanabi::State& s = game.state;
  const int alice = s.our_player_index;        // will-bot67
  const int bob = s.next_player_index(alice);  // yagami_black
  ASSERT_EQ(alice, 2);
  ASSERT_EQ(bob, 0);

  // The facts N5 keys on: Bob's chop is playable and not duplicated in his
  // own hand. (It IS duplicated in Cathy's hand, which is exactly why H1/N1
  // cannot fire here and why N5 has to be weaker than `at_risk_chop`.)
  auto chop = game.chop(bob);
  ASSERT_TRUE(chop.has_value());
  auto chop_id = s.deck[*chop].id();
  ASSERT_TRUE(chop_id.has_value());
  EXPECT_TRUE(s.is_playable(*chop_id))
      << "Bob's chop is Navy 2 on a Navy stack of 1";
  ASSERT_GE(s.pace(), 3);
  ASSERT_LE(s.clue_tokens, 3) << "guard: the pace-clue tier gate must be live";

  int blue = -1;
  for (size_t i = 0; i < s.variant->clue_colour_names.size(); ++i) {
    if (s.variant->clue_colour_names[i] == "Blue") blue = static_cast<int>(i);
  }
  ASSERT_GE(blue, 0) << "every EA suit in this variant clues as Blue";

  hanabi::PerformAction action = game.take_action();

  // The bug: rank 2 is not a direct play here (Aqua 2 is useful and
  // unplayable, so priority 1 fails), and it touches Bob's only unclued slot
  // — his lock slot — so it reads as a LOCK. It won only because the tier
  // gate had flattened every clue to -1.0 and the argmax returned the first
  // candidate in enumeration order (ranks before colours, lowest rank first).
  auto* rank = std::get_if<hanabi::PerformRank>(&action);
  EXPECT_FALSE(rank != nullptr && rank->target == bob && rank->value == 2)
      << "rank 2 to yagami_black locks him for no gain";

  // The fix: Blue is a stable direct play clue calling Navy 2.
  auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(colour, nullptr) << "expected the Blue colour clue to yagami_black";
  EXPECT_EQ(colour->target, bob);
  EXPECT_EQ(colour->value, blue);
}
