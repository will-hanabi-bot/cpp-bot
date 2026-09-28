// Self-play replay 9000008 (v17_self_play_diagnostics, seed 8; tiiah/CONVENTION.md
// §1d, v17.1.0). Seats: 0 sim-alice, 1 sim-bob, 2 sim-cathy (us).
//
// T31: Alice's Red to us, an ordinary reactive; T32: Bob answers with his o7, the
// g3. We are the receiver, so we read his card at reaction time -- on the frame the
// giver and the reacter share, which for us is the shared view floored across every
// seat's hole cards. By then o7 is itself in the hole, and until v17.1.0 the floor
// counted it: the frame read green 3, the bucket reading came out `{g4}`, o7 was
// settled for the team as the g4, and our row for Alice went to green 4 with green
// really on 3 while the shared view stayed on 2. The frame now leaves the card being
// read out of its worlds.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

TEST(SelfPlayReplay9000008, TheReacterFrameLeavesItsCardOut) {
  const char* kSnapshotJson = R"json(
{
  "bot": "sim-cathy",
  "ch": "STATE",
  "current_player_index": 2,
  "debug": {
    "cards_left": 13,
    "clue_tokens": 6,
    "common_play_stacks": [
      2,
      1,
      3,
      5,
      3
    ],
    "current_player_index": 2,
    "discards": [
      {
        "order": 26,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 32,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 24,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 0,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 30,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 17,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 25,
        "rank": 3,
        "suit": 4
      },
      {
        "order": 29,
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
              1,
              1
            ],
            "inferred": 28833789,
            "info_lock": null,
            "order": 33,
            "possible": 28833789,
            "slot": 1,
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
            "inferred": 480,
            "info_lock": null,
            "order": 20,
            "possible": 480,
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
            "inferred": 480,
            "info_lock": null,
            "order": 18,
            "possible": 480,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              5
            ],
            "inferred": 16777232,
            "info_lock": null,
            "order": 4,
            "possible": 16777232,
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
              4
            ],
            "inferred": 13312,
            "info_lock": null,
            "order": 1,
            "possible": 13312,
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
              4
            ],
            "inferred": 28833789,
            "info_lock": null,
            "order": 36,
            "possible": 28833789,
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
              1
            ],
            "inferred": 28833789,
            "info_lock": null,
            "order": 34,
            "possible": 28833789,
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
              1
            ],
            "inferred": 28833789,
            "info_lock": null,
            "order": 27,
            "possible": 28833789,
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
              4
            ],
            "inferred": 26674112,
            "info_lock": null,
            "order": 9,
            "possible": 28804064,
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
              4
            ],
            "inferred": 26674112,
            "info_lock": null,
            "order": 8,
            "possible": 28804064,
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
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 4,
            "info_lock": null,
            "order": 35,
            "possible": 29,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 20177920,
            "info_lock": null,
            "order": 28,
            "possible": 28832768,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 20177920,
            "info_lock": null,
            "order": 19,
            "possible": 28832768,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 20112384,
            "info_lock": null,
            "order": 14,
            "possible": 28832768,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 960,
            "info_lock": null,
            "order": 12,
            "possible": 992,
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
      }
    ],
    "pairwise_play_stacks": [
      [
        2,
        1,
        3,
        5,
        3
      ],
      [
        2,
        1,
        3,
        5,
        3
      ],
      [
        2,
        0,
        3,
        4,
        3
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      2,
      1,
      3,
      5,
      3
    ],
    "strikes": 0,
    "turn_count": 33,
    "waiting": []
  },
  "game_id": 9000008,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "rank": 3,
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
        "rank": 4,
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
          10
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
        "order": 6,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 4,
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
          7,
          5
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 2,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 2,
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
        "cpi": 1,
        "num": 4,
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
        "order": 17,
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
        "num": 5,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          3,
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
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
        "num": 7,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          12
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
        "cpi": 2,
        "num": 8,
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
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
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
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 1,
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
        "order": 22,
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
        "num": 12,
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
        "clues": 4,
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
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 2,
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
        "order": 24,
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
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 0
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
        "order": 23,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 25,
        "p": 1,
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
        "cpi": 2,
        "num": 17,
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
        "clues": 2,
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
        "failed": false,
        "order": 0,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 0,
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
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
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
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 24,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 28,
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
        "num": 22,
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
        "order": 29,
        "p": 1,
        "rank": 4,
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
        "num": 23,
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
        "order": 30,
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
        "num": 24,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 1,
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
        "num": 25,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 29,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 32,
        "p": 1,
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
        "cpi": 2,
        "num": 26,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          31,
          20,
          18
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
        "num": 27,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 33,
        "p": 0,
        "rank": 1,
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
        "num": 28,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 32,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 1,
        "rank": 1,
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
        "num": 29,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 30,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 35,
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
        "num": 30,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          35
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 6,
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
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 4,
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
        "num": 32,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      [
        1,
        4
      ],
      [
        2,
        4
      ],
      [
        4,
        2
      ],
      [
        2,
        2
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
        4,
        1
      ],
      [
        2,
        3
      ],
      [
        1,
        4
      ],
      [
        4,
        4
      ],
      [
        0,
        1
      ],
      [
        3,
        5
      ],
      null,
      null,
      null,
      [
        3,
        4
      ],
      [
        3,
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
      null,
      [
        1,
        3
      ],
      [
        4,
        3
      ],
      null,
      [
        0,
        2
      ],
      [
        1,
        2
      ],
      [
        4,
        3
      ],
      [
        0,
        2
      ],
      [
        2,
        1
      ],
      null,
      [
        4,
        4
      ],
      [
        2,
        2
      ],
      [
        1,
        1
      ],
      [
        1,
        1
      ],
      [
        1,
        1
      ],
      [
        3,
        1
      ],
      null,
      [
        0,
        4
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-28T20:29:53.249",
  "turn": 33
}
  )json";
  hanabi::Game game = hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
  const hanabi::Identity kG3{2, 3};
  EXPECT_TRUE(game.meta[7].named_in_hole.contains(kG3)) << "Bob's o7 is the g3, not the g4";
  EXPECT_EQ(game.state.common_play_stacks[2], 3);
  EXPECT_EQ(game.state.pairwise_play_stacks[0][2], 3) << "our row for Alice: green 3, not 4";
  EXPECT_EQ(game.state.pairwise_play_stacks[1][2], 3);
}
