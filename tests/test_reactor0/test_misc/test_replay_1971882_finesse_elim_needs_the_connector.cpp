// A reaction's negative needs the alternative to have EXISTED.
//
// Turn 42, "Odds and Evens & Dark Prism (6 Suits)". Alice = will-bot69 (p2),
// Bob = will-bot67 (p0), Cathy = yagami (p1). Stacks [3,2,3,3,5,3]. Two cards
// are one clue away from the table: Bob's slot 5 is the r4 and Cathy's slot 1
// is the r5.
//
// COLOUR RED TO CATHY wins both. A colour clue is the even parity under Odds
// and Evens -- a double play -- and Red's colour value is 1, so the pairing
// `react_slot + target_slot = anchor (mod 5)` sends Bob to his slot 5 (the r4)
// and Cathy to her slot 1 (the r5, playable once the r4 lands).
//
// The bot could not see it, because r4 had been struck off Bob's slot 5:
// `inferred = {r2, r5}` against `possible = {r2, r3, r4, r5}`.
//
// That elimination was drawn at turn 19. At T17 yagami clued green to
// will-bot67 (Cathy then); at T18 will-bot69 blind-played a p3; at T19
// will-bot67 played the p4. Same stack, so the reaction read as a FINESSE, and
// the finesse branch strips one-away identities from every passed-over slot.
// Red was on 2, so the r4 was exactly one away.
//
// But that finesse was never available. The anchor was 3, so naming Cathy's
// slot 4 would have required the reacter to blind-play HIS slot
// `calc_slot(3, 4, 5) = 4` -- and will-bot69's slot 4 was clued green, empathy
// {g1, g3, g5, d3}. He could not have held the r3 the reading needs, so "if it
// were the r4, the clue would have named it instead" never applied.
//
// `arm_reaction_elim` now asks that question per slot, at capture time, and
// leaves the r4 alone.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/action.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Dark Prism (6 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1971882, FinesseElimNeedsTheConnectorToHaveExisted) {
  // Reconstruct exactly the Game the live bot saw at turn 42.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1971882,
  "debug": {
    "cards_left": 12,
    "clue_tokens": 5,
    "current_player_index": 2,
    "discards": [
      {
        "order": 39,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 38,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 20,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 37,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 18,
        "rank": 3,
        "suit": 3
      },
      {
        "order": 3,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 21,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 32,
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
              2,
              1
            ],
            "inferred": 809369598,
            "info_lock": null,
            "order": 41,
            "possible": 809369598,
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
              4
            ],
            "inferred": 540017440,
            "info_lock": null,
            "order": 24,
            "possible": 540017632,
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
              5
            ],
            "inferred": 540017440,
            "info_lock": null,
            "order": 22,
            "possible": 540017632,
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
              4
            ],
            "inferred": 25600,
            "info_lock": null,
            "order": 2,
            "possible": 31744,
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
              4
            ],
            "inferred": 18,
            "info_lock": null,
            "order": 1,
            "possible": 30,
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
              5
            ],
            "inferred": 809369598,
            "info_lock": null,
            "order": 42,
            "possible": 809369598,
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
            "inferred": 29696,
            "info_lock": null,
            "order": 27,
            "possible": 31744,
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
            "inferred": 538477236,
            "info_lock": null,
            "order": 7,
            "possible": 538477236,
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
            "inferred": 538477236,
            "info_lock": null,
            "order": 6,
            "possible": 538477236,
            "slot": 4,
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
            "inferred": 538477236,
            "info_lock": null,
            "order": 5,
            "possible": 538477236,
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
            "inferred": 809369598,
            "info_lock": null,
            "order": 40,
            "possible": 809369598,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 809369598,
            "info_lock": null,
            "order": 35,
            "possible": 809369598,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 809369598,
            "info_lock": null,
            "order": 33,
            "possible": 809369598,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 538498740,
            "info_lock": null,
            "order": 31,
            "possible": 538498740,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 270870858,
            "info_lock": null,
            "order": 25,
            "possible": 270870858,
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
        "k": "discard",
        "v": "None"
      }
    ],
    "play_stacks": [
      3,
      2,
      3,
      3,
      5,
      3
    ],
    "strikes": 0,
    "turn_count": 42,
    "waiting": []
  },
  "game_id": 3771,
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
        "rank": 4,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
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
        "rank": 1,
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
        "kind": "C",
        "list": [
          10,
          12,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 0,
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
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
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
        "kind": "C",
        "list": [
          10,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 2,
        "suit": 4,
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
        "order": 16,
        "p": 2,
        "rank": 1,
        "suit": 1,
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
        "clues": 6,
        "max": 30,
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
        "kind": "C",
        "list": [
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
        "kind": "C",
        "list": [
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 4
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
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 5,
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
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 3,
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
        "failed": false,
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 3,
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
        "failed": false,
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 5,
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
          5,
          6,
          7,
          15,
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 4,
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
        "failed": false,
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 7,
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
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
        "t": "turn"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 4,
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
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 18,
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
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 23,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 2,
        "kind": "C",
        "list": [
          27
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
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
        "order": 26,
        "p": 0,
        "rank": 2,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 0,
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
        "clues": 3,
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
        "kind": "C",
        "list": [
          2,
          28
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 2,
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
        "order": 28,
        "p": 0,
        "rank": 3,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 3,
        "suit": 3,
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
        "num": 25,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          30
        ],
        "t": "clue",
        "target": 0,
        "value": 3
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
        "order": 19,
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
        "clues": 1,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 0,
        "rank": 3,
        "suit": 3,
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
        "clues": 1,
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
        "giver": 1,
        "kind": "R",
        "list": [
          10,
          25
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 0,
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
        "failed": false,
        "order": 21,
        "p": 2,
        "rank": 1,
        "suit": 4,
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
        "clues": 1,
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
        "order": 32,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 2,
        "suit": 1,
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
        "num": 31,
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
        "clues": 1,
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
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 35,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 16,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "order": 34,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 0,
        "rank": 5,
        "suit": 4,
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
        "num": 34,
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
        "clues": 0,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 35,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 37,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 0,
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
        "rank": 5,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 38,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 1,
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
        "order": 29,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 39,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
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
        "order": 37,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
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
        "order": 38,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 41,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "failed": false,
        "order": 39,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 42,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
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
        4,
        4
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
        3,
        4
      ],
      [
        0,
        2
      ],
      [
        5,
        5
      ],
      [
        1,
        1
      ],
      [
        4,
        1
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
        2,
        2
      ],
      [
        5,
        1
      ],
      [
        2,
        3
      ],
      [
        4,
        3
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
        1,
        1
      ],
      [
        4,
        2
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
        1,
        1
      ],
      [
        4,
        1
      ],
      [
        1,
        5
      ],
      [
        3,
        1
      ],
      [
        1,
        4
      ],
      null,
      [
        5,
        2
      ],
      [
        2,
        3
      ],
      [
        5,
        3
      ],
      [
        4,
        4
      ],
      [
        3,
        3
      ],
      null,
      [
        4,
        3
      ],
      null,
      [
        1,
        2
      ],
      null,
      [
        4,
        5
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
        0,
        1
      ],
      null,
      [
        2,
        1
      ],
      [
        0,
        5
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
      "variant_name": "Odds and Evens & Dark Prism (6 Suits)"
    },
    "our_player_index": 2,
    "rlocks": false,
    "variant": "Odds and Evens & Dark Prism (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-25T05:59:11.565",
  "turn": 42
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // Order 1 is will-bot67's slot 5 -- the r4 the old elim wrote off.
  EXPECT_TRUE(game.common.thoughts[1].inferred.contains(hanabi::Identity{0, 4}))
      << "the r4 must survive: the finesse that would have named it needed a "
         "connector the reacter's paired slot could not have held";

  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformColour>(action))
      << "expected the red finesse, got " << hanabi::to_json(action, 0).dump();
  const auto& c = std::get<hanabi::PerformColour>(action);
  EXPECT_EQ(c.value, 0) << "red";
  EXPECT_EQ(c.target, 1) << "to yagami -- a reactive clue, so the even parity "
                            "under Odds and Evens: Bob lays the r4, Cathy the r5";
}
