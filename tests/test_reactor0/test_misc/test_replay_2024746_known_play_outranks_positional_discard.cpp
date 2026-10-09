// Replay 2024746 T56-T57 (reactor0, v23.7.0, the user's ruling), will-bot69's seat.
// The deck is empty. Noah_R threw his slot 5 at T56; will-bot69 knew its o21 was
// the p5 and will-bot67 its o3 the r5. A play the team can see is certain always
// comes first, so with both of the remaining plays known the discard speaks to
// nobody. Until v23.7.0 will-bot69 read it as naming its slot 5 and played that --
// a dead g3 -- instead of the p5.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: No Variant. 3 players, our_player_index=2.

TEST(MiscReplay2024746, KnownPlayOutranksPositionalDiscard) {
  // Reconstruct exactly the Game the live bot saw at turn 57.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 2024746,
  "debug": {
    "cards_left": 0,
    "clue_tokens": 4,
    "current_player_index": 2,
    "discards": [
      {
        "order": 31,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 18,
        "rank": 2,
        "suit": 0
      },
      {
        "order": 16,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 12,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 40,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 35,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 25,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 13,
        "rank": 4,
        "suit": 2
      },
      {
        "order": 28,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 20,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 15,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 33,
        "rank": 2,
        "suit": 4
      },
      {
        "order": 7,
        "rank": 4,
        "suit": 4
      }
    ],
    "endgame_turns": 2,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              2
            ],
            "inferred": 5347493,
            "info_lock": null,
            "order": 49,
            "possible": 5347493,
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
            "inferred": 1148961,
            "info_lock": null,
            "order": 43,
            "possible": 1148961,
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
            "inferred": 8,
            "info_lock": 8,
            "order": 29,
            "possible": 8,
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
            "inferred": 4198532,
            "info_lock": null,
            "order": 26,
            "possible": 4198532,
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
            "inferred": 16,
            "info_lock": null,
            "order": 3,
            "possible": 16,
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
              0,
              1
            ],
            "inferred": 5347493,
            "info_lock": null,
            "order": 41,
            "possible": 5347493,
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
            "inferred": 5347493,
            "info_lock": null,
            "order": 38,
            "possible": 5347493,
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
              1
            ],
            "inferred": 1148961,
            "info_lock": null,
            "order": 32,
            "possible": 1148961,
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
              1
            ],
            "inferred": 1148961,
            "info_lock": null,
            "order": 30,
            "possible": 1148961,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "Noah_R",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 104613,
            "info_lock": null,
            "order": 46,
            "possible": 104613,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 104613,
            "info_lock": null,
            "order": 44,
            "possible": 104613,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 5242880,
            "info_lock": null,
            "order": 39,
            "possible": 5242880,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              5
            ],
            "inferred": 16777216,
            "info_lock": 16777216,
            "order": 21,
            "possible": 16777216,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4096,
            "info_lock": null,
            "order": 11,
            "possible": 6144,
            "slot": 5,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
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
        "k": "clue",
        "v": "Stall"
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
        "k": "clue",
        "v": "Play"
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
        "k": "play",
        "v": "None"
      },
      {
        "k": "discard",
        "v": "None"
      }
    ],
    "pending_reactions": [
      {
        "clue_play_stacks": [
          3,
          5,
          5,
          4,
          4
        ],
        "focus_slot": 5,
        "giver": 1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          47,
          43,
          29,
          26,
          3
        ],
        "turn": 50
      }
    ],
    "play_stacks": [
      4,
      5,
      5,
      5,
      4
    ],
    "strikes": 0,
    "turn_count": 57,
    "waiting": []
  },
  "game_id": 2953,
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
        "rank": 2,
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
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 2,
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
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
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
        "kind": "R",
        "list": [
          8
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
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
        "max": 25,
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
          5,
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 6,
        "max": 25,
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
        "suit": 1,
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
        "kind": "C",
        "list": [
          1,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 5,
        "max": 25,
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
        "suit": 3,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 6,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
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
        "kind": "C",
        "list": [
          11,
          13,
          17
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
        "score": 3,
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
        "rank": 1,
        "suit": 2,
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
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 4,
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
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 1,
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
        "clues": 5,
        "max": 25,
        "score": 5,
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
          11,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 22,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
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
          23
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
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
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 24,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 1,
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
        "clues": 4,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 20,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 8,
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
          11,
          13,
          25
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 8,
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
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 28,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 9,
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
          27
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 9,
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
        "rank": 4,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 30,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 11,
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
          9,
          22,
          28
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 29,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
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
        "max": 25,
        "score": 13,
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
        "rank": 1,
        "suit": 0,
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
        "max": 25,
        "score": 13,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 13,
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
          9,
          28
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 2,
        "max": 25,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 2,
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
        "clues": 2,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 36,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 37,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          21,
          37
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 2,
        "max": 25,
        "score": 15,
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
        "suit": 1,
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
        "clues": 2,
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 3,
        "suit": 0,
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
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 35,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 40,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 28,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 41,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 33,
        "p": 2,
        "rank": 2,
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
        "clues": 5,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 40,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 17,
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
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 5,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
        "t": "turn"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 4,
        "suit": 4,
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
        "clues": 5,
        "max": 25,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 19,
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
          29
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "order": 37,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 46,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 48,
        "t": "turn"
      },
      {
        "order": 45,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 47,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          3,
          47
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 5,
        "max": 25,
        "score": 21,
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
          29
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 51,
        "t": "turn"
      },
      {
        "order": 47,
        "p": 0,
        "rank": 5,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 48,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 52,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          21,
          39
        ],
        "t": "clue",
        "target": 2,
        "value": 4
      },
      {
        "clues": 4,
        "max": 25,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 53,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          29,
          48
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 54,
        "t": "turn"
      },
      {
        "order": 48,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 49,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 55,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 56,
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
        0,
        2
      ],
      [
        1,
        1
      ],
      [
        0,
        5
      ],
      [
        2,
        4
      ],
      [
        3,
        3
      ],
      [
        3,
        2
      ],
      [
        4,
        4
      ],
      [
        4,
        1
      ],
      [
        4,
        3
      ],
      [
        3,
        1
      ],
      null,
      [
        1,
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
        4,
        1
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
        0,
        2
      ],
      [
        0,
        3
      ],
      [
        3,
        4
      ],
      [
        4,
        5
      ],
      [
        2,
        3
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
        1
      ],
      [
        0,
        3
      ],
      [
        2,
        2
      ],
      [
        3,
        3
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
        0,
        1
      ],
      [
        3,
        1
      ],
      [
        4,
        2
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
        3
      ],
      [
        1,
        5
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
        1
      ],
      [
        4,
        4
      ],
      [
        3,
        2
      ],
      null,
      [
        2,
        5
      ],
      null,
      [
        3,
        5
      ],
      [
        0,
        4
      ],
      [
        2,
        2
      ]
    ],
    "names": [
      "will-bot67",
      "Noah_R",
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
      "variant_name": "No Variant"
    },
    "our_player_index": 2,
    "rlocks": true,
    "variant": "No Variant",
    "zcs_turn": -1
  },
  "ts": "2026-10-09T06:38:23.203",
  "turn": 57
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  EXPECT_FALSE(game.meta[11].positional_play) << "the discard names nothing";
  hanabi::PerformAction action = game.take_action();
  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action));
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 21) << "the known p5";
}
