// Forced endgame Rule 4 -- two criticals to cash, two turns to cash them in,
// and a partner who cannot play.
//
// Replay 1974303 T44, "Matryoshka & Dark Null (5 Suits)", stacks r4 y5 g4 b4 d4,
// two cards left, one clue.
//
// will-bot69 holds a KNOWN d5 (Dark Null, so the only copy) and a card read
// {r5, g5, b5} -- every member playable, every member critical. will-bot67
// holds b1, r3, r1, y1, y3: all trash, so he cannot play whatever he is told.
//
// The deck therefore drains on schedule and will-bot69 gets exactly two turns
// if he acts now and one if he stalls. He clued rank 2 to Cathy instead and one
// of the two criticals died with the game.
//
// Rule 2 could not cover this: it wants two SINGLETON-critical cards, and the
// {r5, g5, b5} card is read as three identities. What matters is that every one
// of them is playable and critical, which is what `certain_plays` plus a
// criticality filter asks.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Matryoshka & Dark Null (5 Suits). 3 players, our_player_index=1.

TEST(EndgameReplay1974303, TwoCriticalsWithADeadPartnerMustPlay) {
  // Reconstruct exactly the Game the live bot saw at turn 44.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1974303,
  "debug": {
    "cards_left": 2,
    "clue_tokens": 1,
    "current_player_index": 1,
    "discards": [
      {
        "order": 24,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 29,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 11,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 31,
        "rank": 3,
        "suit": 2
      },
      {
        "order": 2,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 21,
        "rank": 2,
        "suit": 3
      },
      {
        "order": 6,
        "rank": 4,
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
              2,
              1
            ],
            "inferred": 716223,
            "info_lock": null,
            "order": 34,
            "possible": 716223,
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
            "inferred": 33825,
            "info_lock": null,
            "order": 32,
            "possible": 33825,
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
              3
            ],
            "inferred": 665884,
            "info_lock": null,
            "order": 30,
            "possible": 682398,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              5
            ],
            "inferred": 540688,
            "info_lock": null,
            "order": 26,
            "possible": 540688,
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
              2
            ],
            "inferred": 2050,
            "info_lock": null,
            "order": 22,
            "possible": 2050,
            "slot": 5,
            "status": "NONE",
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
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 540688,
            "info_lock": 540688,
            "order": 41,
            "possible": 540688,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 175535,
            "info_lock": null,
            "order": 37,
            "possible": 175535,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 175535,
            "info_lock": null,
            "order": 35,
            "possible": 175535,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 8456,
            "info_lock": null,
            "order": 27,
            "possible": 8456,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 16777216,
            "info_lock": null,
            "order": 16,
            "possible": 16777216,
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
            "id": [
              3,
              1
            ],
            "inferred": 716223,
            "info_lock": null,
            "order": 42,
            "possible": 716223,
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
              3
            ],
            "inferred": 141718,
            "info_lock": null,
            "order": 40,
            "possible": 682398,
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
              1
            ],
            "inferred": 33825,
            "info_lock": null,
            "order": 38,
            "possible": 33825,
            "slot": 3,
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
            "inferred": 33825,
            "info_lock": null,
            "order": 33,
            "possible": 33825,
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
              3
            ],
            "inferred": 133250,
            "info_lock": null,
            "order": 25,
            "possible": 133254,
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
      }
    ],
    "play_stacks": [
      4,
      5,
      4,
      4,
      4
    ],
    "strikes": 0,
    "turn_count": 44,
    "waiting": []
  },
  "game_id": 188,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "rank": 1,
        "suit": 0,
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
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 2,
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
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "kind": "R",
        "list": [
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
        "giver": 0,
        "kind": "R",
        "list": [
          11,
          12,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 25,
        "score": 1,
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
        "suit": 3,
        "t": "play"
      },
      {
        "order": 16,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
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
        "order": 14,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 5,
        "suit": 1,
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
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 4,
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
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          12,
          13,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
        "suit": 2,
        "t": "play"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
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
        "kind": "R",
        "list": [
          18,
          22
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
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
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
          11,
          17,
          19,
          21
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 2,
        "max": 25,
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
        "rank": 4,
        "suit": 2,
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
        "max": 25,
        "score": 9,
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
        "rank": 2,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
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
        "order": 26,
        "p": 0,
        "rank": 5,
        "suit": 3,
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
        "num": 19,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 24,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 10,
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
          20,
          27
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 29,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 2,
        "max": 25,
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
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 12,
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
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 31,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 12,
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
          8,
          20,
          27,
          31
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 3,
        "max": 25,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
          32
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 19,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 33,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
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
        "order": 31,
        "p": 1,
        "rank": 3,
        "suit": 2,
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
        "clues": 3,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 36,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
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
        "value": 4
      },
      {
        "clues": 3,
        "max": 25,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 34,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 4,
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
        "max": 25,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "order": 36,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 38,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
        "score": 17,
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
          17
        ],
        "t": "clue",
        "target": 2,
        "value": 5
      },
      {
        "clues": 2,
        "max": 25,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 39,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 40,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
          33,
          38
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 2,
        "max": 25,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "order": 39,
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
        "clues": 2,
        "max": 25,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 41,
        "t": "turn"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 25,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          41
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 1,
        "max": 25,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
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
      [
        1,
        1
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
        3,
        4
      ],
      [
        2,
        4
      ],
      [
        0,
        3
      ],
      [
        3,
        3
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
        3,
        2
      ],
      [
        2,
        3
      ],
      [
        2,
        2
      ],
      [
        3,
        1
      ],
      null,
      [
        1,
        5
      ],
      [
        1,
        2
      ],
      [
        3,
        4
      ],
      [
        1,
        4
      ],
      [
        3,
        2
      ],
      [
        2,
        2
      ],
      [
        0,
        4
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
        5
      ],
      null,
      [
        1,
        3
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
        2,
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
      [
        2,
        1
      ],
      null,
      [
        0,
        2
      ],
      null,
      [
        0,
        1
      ],
      [
        4,
        4
      ],
      [
        0,
        3
      ],
      null,
      [
        3,
        1
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
      "variant_name": "Matryoshka & Dark Null (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": false,
    "variant": "Matryoshka & Dark Null (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T23:55:10.695",
  "turn": 44
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards: each of the rule's three conditions --------------------------
  ASSERT_EQ(s.our_player_index, 1) << "we are will-bot69, the current player";
  ASSERT_EQ(s.current_player_index, 1);
  ASSERT_EQ(s.num_players, 3);
  ASSERT_EQ(s.cards_left, 2) << "condition 1: two cards left";

  // Condition 2: two cards whose EVERY reading is playable and critical.
  for (int o : {41, 16}) {
    const hanabi::IdentitySet live = game.me().thoughts[o].possibilities();
    ASSERT_TRUE(live.non_empty());
    EXPECT_TRUE(live.forall([&](hanabi::Identity i) {
      return s.is_playable(i) && s.is_critical(i);
    })) << "order " << o << " must read as playable-and-critical throughout";
  }
  EXPECT_GT(game.me().thoughts[41].possibilities().length(), 1u)
      << "and order 41 is NOT pinned to one identity -- the clause that puts "
         "this outside Rule 2";

  // Condition 3: the next seat's hand is entirely trash.
  const int bob = s.next_player_index(1);
  for (int o : s.hands[bob]) {
    auto id = s.deck[o].id();
    ASSERT_TRUE(id.has_value()) << "we can see Bob's hand";
    EXPECT_TRUE(s.is_basic_trash(*id))
        << "order " << o << " must be trash -- Bob cannot play at all";
  }

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  EXPECT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "a play is forced -- stalling here costs one of the two criticals. "
         "got " << hanabi::to_json(action, 0).dump();
  if (std::holds_alternative<hanabi::PerformPlay>(action)) {
    const int target = std::get<hanabi::PerformPlay>(action).target;
    EXPECT_TRUE(target == 41 || target == 16)
        << "and it must be one of the two criticals, not a gamble";
  }
}
