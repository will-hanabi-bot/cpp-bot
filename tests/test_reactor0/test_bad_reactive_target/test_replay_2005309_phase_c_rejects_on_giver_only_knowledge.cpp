// Replay 2005309 turn 33 (reactor0, Dual-Color & Orange: Tangerine t, Violet v,
// Jade j, Orange o). will-bot69 is Alice, will-bot67 Bob (the reacter),
// yagami_black Cathy (the receiver). Stacks t2 v5 j4 o3.
//
// Rank 2 to yagami_black is a reactive rank clue, anchor 2. Phase A finds her
// no playable; Phase B's only one-away card is her t4 at slot 4, whose pairing
// is Bob's slot 3, which common knowledge says cannot be the t3. Phase C then
// walks her trash: her leftmost is the o1 at slot 1, pairing with Bob's slot 1
// (calc_slot(2,1,5) = 1). An inverted dc-target swaps the reacter onto Play, so
// the call is a blind play -- and Bob's slot 1 is a v4 the giver can see is
// neither playable nor a connector, which is a giver-only REJECT.
//
// Until v15.3.0 Phase C treated that REJECT as a retarget and walked on to her
// slot 2 (the v1), pairing with Bob's slot 5 -- his known Orange 4, whose
// discard call is a chuck that stacks. So the giver read a safe "Bob plays,
// Cathy discards" reactive discard, `predicts_a_strike` had nothing to veto,
// and rung 2 gave it. Bob, who cannot see his own v4, stayed on the slot 1
// pairing, played it at T34 and struck. §1g: shared knowledge retargets,
// giver-only knowledge rejects.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Dual-Color & Orange (4 Suits). 3 players, our_player_index=2.

TEST(BadReactiveTarget2005309, PhaseCRejectsOnGiverOnlyKnowledge) {
  // Reconstruct exactly the Game the live bot saw at turn 33.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2005309,
  "debug": {
    "cards_left": 5,
    "clue_tokens": 3,
    "current_player_index": 2,
    "discards": [
      {
        "order": 1,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 14,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 10,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 27,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 32,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 26,
        "rank": 2,
        "suit": 3
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
              1,
              4
            ],
            "inferred": 970047,
            "info_lock": null,
            "order": 34,
            "possible": 970047,
            "slot": 1,
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
            "inferred": 707637,
            "info_lock": null,
            "order": 30,
            "possible": 707639,
            "slot": 2,
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
            "inferred": 707584,
            "info_lock": null,
            "order": 19,
            "possible": 707584,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              5
            ],
            "inferred": 540672,
            "info_lock": null,
            "order": 16,
            "possible": 540672,
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
            "inferred": 262144,
            "info_lock": null,
            "order": 3,
            "possible": 262144,
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
            "inferred": 970047,
            "info_lock": null,
            "order": 28,
            "possible": 970047,
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
            "inferred": 970047,
            "info_lock": null,
            "order": 25,
            "possible": 970047,
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
            "inferred": 64,
            "info_lock": null,
            "order": 18,
            "possible": 64,
            "slot": 3,
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
            "inferred": 16412,
            "info_lock": null,
            "order": 7,
            "possible": 16412,
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
              5
            ],
            "inferred": 16412,
            "info_lock": null,
            "order": 6,
            "possible": 16412,
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
            "inferred": 838971,
            "info_lock": null,
            "order": 33,
            "possible": 838971,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 131076,
            "info_lock": null,
            "order": 31,
            "possible": 131076,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 17721,
            "info_lock": null,
            "order": 24,
            "possible": 17721,
            "slot": 3,
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
            "inferred": 4,
            "info_lock": null,
            "order": 23,
            "possible": 4,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 25,
            "info_lock": null,
            "order": 11,
            "possible": 25,
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
        "v": "Stall"
      }
    ],
    "pending_reactions": [],
    "play_stacks": [
      2,
      5,
      4,
      3
    ],
    "strikes": 0,
    "turn_count": 33,
    "waiting": []
  },
  "game_id": 4284,
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
        "rank": 1,
        "suit": 1,
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
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 2,
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
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
          0,
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 0
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
        "failed": false,
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
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
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 5,
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
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
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
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 5,
        "max": 20,
        "score": 3,
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
          11,
          14,
          15
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 4,
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
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 20,
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
        "kind": "R",
        "list": [
          18
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 20,
        "score": 4,
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
        "order": 19,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 20,
        "score": 5,
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
        "order": 20,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 20,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
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
        "max": 20,
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
        "rank": 2,
        "suit": 3,
        "t": "discard"
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
        "max": 20,
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
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 3,
        "max": 20,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 2,
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
        "clues": 3,
        "max": 20,
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
          6,
          7,
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 2,
        "max": 20,
        "score": 8,
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
          14,
          15
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 1,
        "max": 20,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 4,
        "suit": 1,
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
        "max": 20,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 0,
        "max": 20,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 20,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 3,
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
        "clues": 1,
        "max": 20,
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
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 20,
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
        "suit": 2,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 20,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 3,
        "suit": 1,
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
        "clues": 3,
        "max": 20,
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
        "order": 27,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 20,
        "score": 12,
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
          3,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 3,
        "max": 20,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "order": 26,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 31,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 20,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
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
        "order": 32,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 20,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
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
        "value": 2
      },
      {
        "clues": 3,
        "max": 20,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 33,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 20,
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
        "order": 32,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 20,
        "score": 14,
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
          23,
          31
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 3,
        "max": 20,
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
        1,
        2
      ],
      [
        1,
        1
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
        3,
        2
      ],
      [
        0,
        1
      ],
      [
        0,
        5
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
        1,
        3
      ],
      [
        2,
        1
      ],
      null,
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
        3
      ],
      [
        1,
        4
      ],
      [
        2,
        5
      ],
      [
        1,
        1
      ],
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
        5
      ],
      [
        2,
        2
      ],
      [
        2,
        4
      ],
      [
        0,
        3
      ],
      null,
      [
        1,
        1
      ],
      [
        3,
        2
      ],
      [
        2,
        3
      ],
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
        2
      ],
      null,
      [
        2,
        4
      ],
      null,
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
      "variant_name": "Dual-Color & Orange (4 Suits)"
    },
    "our_player_index": 2,
    "rlocks": false,
    "variant": "Dual-Color & Orange (4 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-21T21:49:56.573",
  "turn": 33
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.convention, hanabi::Convention::REACTOR0)
      << "the snapshot must replay under reactor0";

  const hanabi::State& s = game.state;
  const int alice = s.our_player_index;         // will-bot69
  const int bob = s.next_player_index(alice);   // will-bot67
  const int cathy = s.next_player_index(bob);   // yagami_black
  ASSERT_EQ(alice, 2);
  ASSERT_EQ(bob, 0);
  ASSERT_EQ(cathy, 1);

  // The pairing Phase C must reject: Cathy's leftmost trash is the inverted o1,
  // and its react slot holds a Violet 4 with Violet finished.
  ASSERT_EQ(s.deck[s.hands[cathy][0]].id(), (hanabi::Identity{3, 1}));
  ASSERT_EQ(s.deck[s.hands[bob][0]].id(), (hanabi::Identity{1, 4}));
  ASSERT_FALSE(s.is_playable(hanabi::Identity{1, 4}));
  // ...and the pairing it used to walk on to: her v1, against Bob's known o4.
  ASSERT_EQ(s.deck[s.hands[cathy][1]].id(), (hanabi::Identity{1, 1}));
  ASSERT_EQ(s.deck[s.hands[bob][4]].id(), (hanabi::Identity{3, 4}));

  // The clue itself is unreadable from the giver's seat, which is what keeps
  // `analyse_clues` from ever offering it.
  auto touched = s.clue_touched(s.hands[cathy], hanabi::ClueKind::RANK, 2);
  ASSERT_FALSE(touched.empty()) << "guard: rank 2 is a legal clue here";
  hanabi::Action clue{hanabi::ClueAction{alice, cathy, std::move(touched),
                                         hanabi::BaseClue{hanabi::ClueKind::RANK, 2}}};
  hanabi::Game hypo = game.simulate(clue);
  auto move = hypo.last_move();
  ASSERT_TRUE(move.has_value());
  ASSERT_TRUE(std::holds_alternative<hanabi::ClueInterp>(*move));
  EXPECT_EQ(std::get<hanabi::ClueInterp>(*move), hanabi::ClueInterp::MISTAKE)
      << "the reacter would blind-play the v4, so the clue must not read";

  // ...so the bot gives something else. (It gives Red to yagami_black, which is
  // colour mode 2 on the same o1: Bob chucks his known o4 for a point and she
  // pitches the o1.)
  hanabi::PerformAction action = game.take_action();
  auto* rank = std::get_if<hanabi::PerformRank>(&action);
  EXPECT_FALSE(rank != nullptr && rank->target == cathy && rank->value == 2)
      << "rank 2 to yagami_black makes will-bot67 blind-play a Violet 4";
}
