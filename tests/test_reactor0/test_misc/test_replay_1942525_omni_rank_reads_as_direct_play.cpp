// Replay 1942525 turn 53 (reactor0, "Extremely Ambiguous & Dark Omni").
// Dark Omni is an OMNI suit, so every rank clue touches it at every rank.
// stable_rank classified the rank over the variant-wide touch set, so
// Dark Omni 4 and 5 (useful, unplayable at a stack of 2) made playable_rank
// false and priority 1 never fired — the clue degraded to a referential
// discard and will-bot67 chucked its chop. The classification now runs over
// what the TOUCHED CARDS can actually be, where Dark Omni 4 is already
// eliminated. See CONVENTION.md §1c.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Extremely Ambiguous & Dark Omni (6 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1942525, OmniRankFourReadsAsDirectPlay) {
  // Reconstruct exactly the Game the live bot saw at turn 53.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1942525,
  "debug": {
    "cards_left": 6,
    "clue_tokens": 3,
    "current_player_index": 1,
    "discards": [
      {
        "order": 30,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 13,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 44,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 41,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 18,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 29,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 32,
        "rank": 2,
        "suit": 3
      },
      {
        "order": 37,
        "rank": 4,
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
              4,
              2
            ],
            "inferred": 6711502,
            "info_lock": null,
            "order": 47,
            "possible": 6711502,
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
              4
            ],
            "inferred": 4612236,
            "info_lock": null,
            "order": 45,
            "possible": 4612236,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              1
            ],
            "inferred": 1082401,
            "info_lock": null,
            "order": 38,
            "possible": 1082401,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              5,
              5
            ],
            "inferred": 939524096,
            "info_lock": null,
            "order": 22,
            "possible": 939524096,
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
            "inferred": 939524096,
            "info_lock": null,
            "order": 4,
            "possible": 939524096,
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
            "id": null,
            "inferred": 7523559,
            "info_lock": null,
            "order": 48,
            "possible": 7523559,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 7523559,
            "info_lock": null,
            "order": 39,
            "possible": 7523559,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 5293221,
            "info_lock": null,
            "order": 24,
            "possible": 5424293,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 270344,
            "info_lock": null,
            "order": 21,
            "possible": 270344,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 270344,
            "info_lock": null,
            "order": 9,
            "possible": 270344,
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
              0,
              2
            ],
            "inferred": 947317999,
            "info_lock": null,
            "order": 46,
            "possible": 947317999,
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
              1
            ],
            "inferred": 947317999,
            "info_lock": null,
            "order": 42,
            "possible": 947317999,
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
            "inferred": 943849604,
            "info_lock": null,
            "order": 31,
            "possible": 943849604,
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
              1
            ],
            "inferred": 1369121,
            "info_lock": null,
            "order": 27,
            "possible": 1369129,
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
            "inferred": 4325508,
            "info_lock": null,
            "order": 11,
            "possible": 4325508,
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
        "v": "Lock"
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
        "k": "clue",
        "v": "Stall"
      }
    ],
    "play_stacks": [
      5,
      5,
      3,
      5,
      5,
      2
    ],
    "strikes": 0,
    "turn_count": 53,
    "waiting": []
  },
  "game_id": 3499,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
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
        "rank": 3,
        "suit": 2,
        "t": "draw"
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 3,
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
          7
        ],
        "t": "clue",
        "target": 1,
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
        "order": 3,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "giver": 1,
        "kind": "R",
        "list": [
          4,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 2,
        "suit": 4,
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
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 4,
        "suit": 2,
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
        "giver": 1,
        "kind": "R",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 17,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "clues": 4,
        "max": 30,
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
        "kind": "R",
        "list": [
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
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
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 5,
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
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 9,
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
          1,
          4,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 2,
        "max": 30,
        "score": 9,
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
          5,
          7,
          9,
          21,
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 1,
        "max": 30,
        "score": 9,
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
        "rank": 2,
        "suit": 3,
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
        "clues": 1,
        "max": 30,
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
        "order": 19,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 1,
        "suit": 5,
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
        "failed": false,
        "order": 18,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "giver": 1,
        "kind": "C",
        "list": [
          1,
          2,
          4,
          22,
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 27,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "giver": 0,
        "kind": "R",
        "list": [
          25
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 0,
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
          7
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
        "order": 26,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 1,
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
        "num": 25,
        "t": "turn"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 3,
        "suit": 1,
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
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 31,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 29,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 0,
        "rank": 2,
        "suit": 3,
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
        "num": 28,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 30,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 33,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          7,
          9,
          21
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 35,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          9,
          21,
          24,
          33,
          35
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 1,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 32,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 36,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 37,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 18,
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
          1,
          4,
          22,
          34,
          36
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 18,
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
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 19,
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
        "rank": 4,
        "suit": 4,
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
        "clues": 3,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 0,
        "kind": "R",
        "list": [
          11,
          14,
          31
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 35,
        "p": 1,
        "rank": 4,
        "suit": 4,
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
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "order": 40,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 41,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 44,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 22,
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
          4,
          22,
          34,
          38,
          43
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 4,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          4,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 3,
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
        "order": 14,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 46,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 47,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 44,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 48,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 50,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          4,
          22,
          38
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 4,
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
        "giver": 0,
        "kind": "R",
        "list": [
          9,
          21
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 3,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 52,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        1,
        2
      ],
      [
        3,
        5
      ],
      [
        1,
        4
      ],
      [
        1,
        1
      ],
      [
        5,
        4
      ],
      [
        0,
        4
      ],
      [
        3,
        1
      ],
      [
        5,
        2
      ],
      [
        2,
        1
      ],
      null,
      [
        4,
        1
      ],
      [
        0,
        3
      ],
      [
        0,
        3
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
        0,
        2
      ],
      [
        0,
        1
      ],
      [
        4,
        2
      ],
      [
        2,
        4
      ],
      [
        1,
        4
      ],
      [
        4,
        3
      ],
      null,
      [
        5,
        5
      ],
      [
        3,
        2
      ],
      null,
      [
        5,
        1
      ],
      [
        0,
        5
      ],
      [
        2,
        1
      ],
      [
        1,
        3
      ],
      [
        3,
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
        3,
        2
      ],
      [
        1,
        5
      ],
      [
        4,
        5
      ],
      [
        4,
        4
      ],
      [
        3,
        3
      ],
      [
        4,
        4
      ],
      [
        4,
        1
      ],
      null,
      [
        3,
        4
      ],
      [
        2,
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
        4,
        2
      ],
      null
    ],
    "names": [
      "yagami_black",
      "will-bot67",
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
      "variant_name": "Extremely Ambiguous & Dark Omni (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Extremely Ambiguous & Dark Omni (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-01T03:27:08.147",
  "turn": 53
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::REACTOR0);

  const hanabi::State& s = game.state;
  const int me = s.our_player_index;  // will-bot67
  ASSERT_EQ(me, 1);

  // Slot 4 is the card the rank-4 clue promised. Its empathy has already
  // ruled out Dark Omni 4 (that copy is visible in yagami_black's hand), so
  // the only *useful* identity it can hold is Sky 4 — playable on a Sky
  // stack of 3. Ice 4 and Berry 4 are trash at stacks of 5.
  const int slot4 = s.hands[me][3];
  const hanabi::Thought& t = game.common.thoughts[slot4];
  int useful = 0, useful_playable = 0;
  for (hanabi::Identity id : (t.inferred.non_empty() ? t.inferred : t.possible)) {
    if (s.is_basic_trash(id)) continue;
    ++useful;
    if (s.is_playable(id)) ++useful_playable;
  }
  EXPECT_GT(useful, 0);
  EXPECT_EQ(useful, useful_playable)
      << "every useful identity slot 4 can hold must be playable — that is "
         "what makes the rank-4 clue a direct play clue";

  hanabi::PerformAction action = game.take_action();

  // Before the fix the classification scanned the whole variant touch set,
  // which in an omni variant includes Dark Omni at every rank; one
  // useful-unplayable omni rank made `playable_rank` false, priority 1 never
  // fired, and the clue degraded to a referential discard.
  auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  EXPECT_FALSE(discard != nullptr && discard->target == s.hands[me][0])
      << "discarding the chop was the bug";

  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "expected a play";
  EXPECT_EQ(play->target, slot4) << "the left 4 (slot 4) is the promised card";
}
