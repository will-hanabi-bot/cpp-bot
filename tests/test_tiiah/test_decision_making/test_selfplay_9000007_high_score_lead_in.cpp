// The HIGH SCORE PHASE LEAD-IN (v23.26.0, the user's ruling): once plays reach 2/3 of
// the maximum, a play clue revealing an unknown playable of Bob's -- or of Cathy's,
// when Bob is loaded -- that leads into at least two cards in Bob's and Cathy's
// hands (and Alice's own, where she names it), or straight into a critical one, is
// HIGH, and priority 1b gives it after a reactive play clue.
// - Self-play Dark Null 9000007 (v23.22) T48, human_vs_bot_diagnostics/9000007_23_22.md:
//   sim-alice holds her called d5, so it is sim-bob's cards that count; his b3 leads
//   into sim-alice's critical b4 and b5, his r3 into his own r4 and r5. The 3 reveals
//   both. v23.25.0 chucked (every clue LOW, the MEDIUM bar at two tokens).
// - Self-play Dark Null 9000081 (v23.22) T51, human_vs_bot_diagnostics/9000081_23_22.md:
//   sim-alice's b2 leads straight into sim-cathy's own critical b3, so Blue to
//   sim-alice is a HIGH lead-in. (The 1 to sim-bob, a reactive play, still comes
//   first: test_selfplay_9000081_both_name_their_card_late.cpp.)

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/state_eval.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// T48, sim-cathy's seat.
// Variant: Throw It in a Hole & Dark Null (6 Suits). 3 players, our_player_index=2.
TEST(TiiahSelfPlay9000007, ThreeToSimBobLeadsIntoTheOtherHands) {
  // Reconstruct exactly the Game the live bot saw at turn 48.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-cathy",
  "ch": "STATE",
  "current_player_index": 2,
  "debug": {
    "cards_left": 8,
    "clue_tokens": 2,
    "common_play_stacks": [
      2,
      5,
      5,
      2,
      5,
      4
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 32,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 27,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 19,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 25,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 26,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 15,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 14,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 30,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 17,
        "rank": 1,
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
              3,
              4
            ],
            "inferred": 552578207,
            "info_lock": null,
            "order": 45,
            "possible": 552578207,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": [
              5,
              5
            ],
            "inferred": 536870912,
            "info_lock": null,
            "order": 41,
            "possible": 547728395,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 4325376,
            "info_lock": null,
            "order": 37,
            "possible": 4325380,
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
            "inferred": 524304,
            "info_lock": null,
            "order": 28,
            "possible": 524304,
            "slot": 4,
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
            "inferred": 4096,
            "info_lock": null,
            "order": 20,
            "possible": 4096,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-alice",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              0,
              3
            ],
            "inferred": 552578207,
            "info_lock": null,
            "order": 46,
            "possible": 552578207,
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
            "inferred": 552578207,
            "info_lock": null,
            "order": 35,
            "possible": 552578207,
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
            "inferred": 8658952,
            "info_lock": null,
            "order": 29,
            "possible": 8658952,
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
            "inferred": 537627671,
            "info_lock": null,
            "order": 21,
            "possible": 537627671,
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
              3
            ],
            "inferred": 753664,
            "info_lock": null,
            "order": 5,
            "possible": 753664,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-bob",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 552578207,
            "info_lock": null,
            "order": 44,
            "possible": 552578207,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 552578207,
            "info_lock": null,
            "order": 42,
            "possible": 552578207,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 552578207,
            "info_lock": null,
            "order": 40,
            "possible": 552578207,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 543884437,
            "info_lock": null,
            "order": 34,
            "possible": 543919255,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 3072,
            "info_lock": null,
            "order": 10,
            "possible": 3072,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-cathy",
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
        "v": "Reveal"
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
        "k": "play",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        2,
        5,
        5,
        2,
        5,
        4
      ],
      [
        2,
        5,
        5,
        2,
        5,
        4
      ],
      [
        2,
        5,
        4,
        1,
        5,
        4
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      2,
      5,
      5,
      2,
      5,
      4
    ],
    "strikes": 0,
    "turn_count": 48,
    "waiting": []
  },
  "game_id": 9000007,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 5,
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
        "order": 9,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "giver": 2,
        "kind": "C",
        "list": [
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
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
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
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
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
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
        "order": 21,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
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
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
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
        "order": 25,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          24,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          23,
          13,
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": -1,
        "suit": -1,
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
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
          29
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 31,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          31,
          19
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 33,
        "p": 1,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 32,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "order": 33,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 35,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
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
        "order": 36,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 30,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          24
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 38,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 39,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          39,
          20,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 36,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 40,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "order": 39,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 41,
        "p": 0,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
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
          37,
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "order": 38,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 42,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
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
        "order": 43,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          28
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
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
        "order": 44,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 19,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 46,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        5,
        2
      ],
      [
        5,
        1
      ],
      [
        4,
        1
      ],
      [
        1,
        2
      ],
      [
        4,
        5
      ],
      [
        3,
        3
      ],
      [
        5,
        3
      ],
      [
        4,
        3
      ],
      [
        4,
        4
      ],
      [
        1,
        1
      ],
      null,
      [
        1,
        4
      ],
      [
        0,
        2
      ],
      [
        2,
        5
      ],
      [
        3,
        4
      ],
      [
        3,
        3
      ],
      [
        2,
        4
      ],
      [
        4,
        1
      ],
      [
        4,
        2
      ],
      [
        1,
        2
      ],
      [
        2,
        3
      ],
      [
        0,
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
        1,
        5
      ],
      [
        1,
        4
      ],
      [
        2,
        1
      ],
      [
        1,
        1
      ],
      [
        3,
        5
      ],
      [
        0,
        4
      ],
      [
        4,
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
        5,
        4
      ],
      null,
      [
        0,
        1
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
        1
      ],
      [
        2,
        3
      ],
      null,
      [
        5,
        5
      ],
      null,
      [
        3,
        2
      ],
      null,
      [
        3,
        4
      ],
      [
        0,
        3
      ]
    ],
    "names": [
      "sim-alice",
      "sim-bob",
      "sim-cathy"
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
      "variant_name": "Throw It in a Hole & Dark Null (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Dark Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-10T20:27:39.233",
  "turn": 48
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "a rank clue, not the chuck";
  EXPECT_EQ(rank->target, 1) << "to sim-bob";
  EXPECT_EQ(rank->value, 3);
}

// T51, sim-cathy's seat: Blue to sim-alice reveals her b2, which leads straight into
// sim-cathy's own critical b3.
// Variant: Throw It in a Hole & Dark Null (6 Suits). 3 players, our_player_index=2.
TEST(TiiahSelfPlay9000081, BlueToSimAliceIsAHighLeadIn) {
  // Reconstruct exactly the Game the live bot saw at turn 51.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-cathy",
  "ch": "STATE",
  "current_player_index": 2,
  "debug": {
    "cards_left": 6,
    "clue_tokens": 3,
    "common_play_stacks": [
      5,
      5,
      5,
      1,
      5,
      2
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 27,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 1,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 42,
        "rank": 3,
        "suit": 0
      },
      {
        "order": 41,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 28,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 44,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 21,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 17,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 38,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 13,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 23,
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
              3,
              2
            ],
            "inferred": 953625961,
            "info_lock": null,
            "order": 45,
            "possible": 953625961,
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
            "inferred": 939994473,
            "info_lock": null,
            "order": 40,
            "possible": 939994473,
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
            "inferred": 939994473,
            "info_lock": null,
            "order": 37,
            "possible": 939994473,
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
            "inferred": 12582912,
            "info_lock": null,
            "order": 32,
            "possible": 12582912,
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
              5
            ],
            "inferred": 524288,
            "info_lock": null,
            "order": 3,
            "possible": 524288,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-alice",
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
            "inferred": 953625961,
            "info_lock": null,
            "order": 48,
            "possible": 953625961,
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
              5
            ],
            "inferred": 952576328,
            "info_lock": null,
            "order": 46,
            "possible": 952576328,
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
            "inferred": 809633856,
            "info_lock": null,
            "order": 39,
            "possible": 943917120,
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
              1
            ],
            "inferred": 1048609,
            "info_lock": null,
            "order": 33,
            "possible": 1048609,
            "slot": 4,
            "status": "NONE",
            "trash": true,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              3
            ],
            "inferred": 943915008,
            "info_lock": null,
            "order": 25,
            "possible": 943915008,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-bob",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 953625961,
            "info_lock": null,
            "order": 47,
            "possible": 953625961,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 949300585,
            "info_lock": null,
            "order": 36,
            "possible": 949300585,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 949300585,
            "info_lock": null,
            "order": 34,
            "possible": 949300585,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 815081481,
            "info_lock": null,
            "order": 26,
            "possible": 949300233,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 131072,
            "info_lock": null,
            "order": 19,
            "possible": 4325376,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-cathy",
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Reveal"
      },
      {
        "k": "play",
        "v": "None"
      }
    ],
    "pairwise_play_stacks": [
      [
        5,
        5,
        5,
        1,
        5,
        2
      ],
      [
        5,
        5,
        5,
        1,
        5,
        2
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
      5,
      5,
      5,
      1,
      5,
      2
    ],
    "strikes": 0,
    "turn_count": 51,
    "waiting": []
  },
  "game_id": 9000081,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 5,
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
          6
        ],
        "t": "clue",
        "target": 1,
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
        "kind": "C",
        "list": [
          2,
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 0
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
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
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
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
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
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
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
        "rank": -1,
        "suit": -1,
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
        "clues": 4,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 15,
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
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
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
        "order": 22,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          20,
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 21,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
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
        "order": 27,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
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
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 27,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          24
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 5,
        "max": 25,
        "score": 0,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
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
        "order": 31,
        "p": 1,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
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
          22,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "order": 24,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
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
        "order": 33,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 28,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 35,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 31,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          35
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 35,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 37,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
          31,
          30,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
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
        "order": 39,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          30
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 0,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 4,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 41,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 42,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 38,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 44,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
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
          43,
          32
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 46,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 44,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 46,
        "p": 1,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 42,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 47,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          33
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 48,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 50,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
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
        3
      ],
      [
        3,
        5
      ],
      [
        5,
        1
      ],
      [
        1,
        2
      ],
      [
        4,
        1
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
        0,
        5
      ],
      [
        0,
        1
      ],
      [
        1,
        4
      ],
      [
        3,
        1
      ],
      [
        4,
        1
      ],
      [
        0,
        2
      ],
      [
        4,
        2
      ],
      [
        1,
        1
      ],
      [
        3,
        3
      ],
      [
        5,
        2
      ],
      null,
      [
        0,
        4
      ],
      [
        3,
        1
      ],
      [
        1,
        5
      ],
      [
        4,
        2
      ],
      [
        4,
        4
      ],
      [
        5,
        3
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
        3
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
      [
        4,
        1
      ],
      null,
      [
        2,
        1
      ],
      null,
      [
        1,
        1
      ],
      [
        3,
        4
      ],
      [
        2,
        2
      ],
      [
        5,
        4
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
        4,
        5
      ],
      [
        3,
        1
      ],
      [
        3,
        2
      ],
      [
        5,
        5
      ],
      null,
      [
        1,
        1
      ]
    ],
    "names": [
      "sim-alice",
      "sim-bob",
      "sim-cathy"
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
      "variant_name": "Throw It in a Hole & Dark Null (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole & Dark Null (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-10-10T20:31:21.465",
  "turn": 51
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;
  std::vector<std::pair<hanabi::PerformAction, hanabi::Action>> clues;
  for (const hanabi::Clue& clue : s.all_valid_clues(0)) {
    if (clue.kind != hanabi::ClueKind::COLOUR || clue.value != 3) continue;
    hanabi::ClueAction act{s.our_player_index, clue.target,
                           s.clue_touched(s.hands[0], clue.kind, clue.value), clue.base()};
    clues.emplace_back(hanabi::PerformAction{hanabi::PerformColour{clue.target, clue.value}},
                       hanabi::Action{act});
  }
  ASSERT_EQ(clues.size(), 1u) << "Blue to sim-alice is legal";
  const auto cands = hanabi::reactor0::analyse_clues(game, clues);
  ASSERT_EQ(cands.size(), 1u);
  EXPECT_EQ(cands[0].lead_in_cards, 1) << "the b2";
  EXPECT_TRUE(cands[0].lead_in_critical) << "its next card, the b3, is critical";
  EXPECT_EQ(cands[0].tier, hanabi::reactor0::ClueTier::HIGH);
}
