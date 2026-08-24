// A play that HAS to happen, when nothing in hand is certain.
//
// Turn 24, "Odds and Evens & Omni (3 Suits)" -- o is Omni, not Orange, so
// nothing here is inverted. Stacks [3,5,5], score 13 of 15, deck EMPTY, three
// turns left: us (p2), then p0, then p1.
//
//   * p0's whole hand is visibly trash. They cannot contribute.
//   * p1 visibly holds BOTH r4 (order 8) and r5 (order 9) and gets ONE turn --
//     so whichever they lay, the other dies with the game.
//   * our order 12 is clued and reads {r2,r4,o1,o2,o3,o4}. It COULD be the
//     other r4. It is.
//
// The only line to 15 is for us to lay that r4 so p1 can cash the r5. The bot
// chucked a known-trash b1 instead, p1 blind-played the r5 into an unadvanced
// stack, and the game ended 13/15.
//
// The search generated our play and then PRUNED it. `thinks_playables`
// subtracts known trash only from TOUCHED cards
// (src/basics/player_game.cpp:186): our order 12 is clued, so its reading
// collapsed to {r4} and was offered; p1's r5 is UNCLUED, so {r1..r5} never
// collapsed and p1 read as having no play at all. The line therefore looked
// unwinnable and the action was dropped (src/endgame/solver.cpp:168). The
// solver could imagine our gamble but not p1 cashing it.
//
// Rule 0b of `forced_endgame_action` answers it without a search: with the deck
// empty, `best_reachable_plays` prices the rest of the round with full sight of
// every other hand. Baseline 1 (p1 lays the r4 themselves); with r4 already
// laid, 2 (p1 lays the r5). r4 lifts the ceiling, so it is REQUIRED -- and
// among our cards that could be it, the leftmost CLUED one wins.
//
// The clued-first priority is what makes this work. Orders 27 (slot 1) and 25
// (slot 2) could also have been the r4 and sit further left; slot 1 is an
// omni 1. Leftmost-of-any would have struck.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/endgame/helper.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Omni (3 Suits). 3 players, our_player_index=2.

TEST(EndgameReplay1970943, RequiredPlayWhenNoneIsCertain) {
  // Reconstruct exactly the Game the live bot saw at turn 24.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1970943,
  "debug": {
    "cards_left": 0,
    "clue_tokens": 4,
    "current_player_index": 2,
    "discards": [
      {
        "order": 2,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 0,
        "rank": 1,
        "suit": 2
      }
    ],
    "endgame_turns": 3,
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
            "inferred": 15743,
            "info_lock": null,
            "order": 28,
            "possible": 15743,
            "slot": 1,
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
            "inferred": 7217,
            "info_lock": null,
            "order": 26,
            "possible": 15413,
            "slot": 2,
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
            "inferred": 7217,
            "info_lock": null,
            "order": 24,
            "possible": 15413,
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
            "inferred": 15360,
            "info_lock": null,
            "order": 3,
            "possible": 15360,
            "slot": 4,
            "status": "CHOP_MOVED",
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
            "inferred": 53,
            "info_lock": null,
            "order": 1,
            "possible": 53,
            "slot": 5,
            "status": "CHOP_MOVED",
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
              3
            ],
            "inferred": 15743,
            "info_lock": null,
            "order": 29,
            "possible": 15743,
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
              2
            ],
            "inferred": 15743,
            "info_lock": null,
            "order": 20,
            "possible": 15743,
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
            "inferred": 15743,
            "info_lock": null,
            "order": 17,
            "possible": 15743,
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
            "inferred": 31,
            "info_lock": null,
            "order": 9,
            "possible": 31,
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
              4
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
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 15743,
            "info_lock": null,
            "order": 27,
            "possible": 15743,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 15743,
            "info_lock": null,
            "order": 25,
            "possible": 15743,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 32,
            "info_lock": null,
            "order": 19,
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
            "inferred": 15370,
            "info_lock": null,
            "order": 12,
            "possible": 15370,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 320,
            "info_lock": null,
            "order": 11,
            "possible": 320,
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
        "v": "Play"
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
      }
    ],
    "play_stacks": [
      3,
      5,
      5
    ],
    "strikes": 0,
    "turn_count": 24,
    "waiting": []
  },
  "game_id": 2637,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 2,
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
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 0,
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
        "kind": "C",
        "list": [
          10,
          12,
          13,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
        "order": 13,
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
        "kind": "C",
        "list": [
          5,
          7,
          15
        ],
        "t": "clue",
        "target": 1,
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
        "suit": 1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 3,
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
        "giver": 2,
        "kind": "R",
        "list": [
          0,
          1,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 15,
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
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
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
          1,
          2,
          3,
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 15,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 2,
        "suit": 1,
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
        "max": 15,
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
          11,
          12,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 15,
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
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 5,
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
        "suit": 2,
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
        "clues": 4,
        "max": 15,
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
        "rank": 4,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 3,
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
        "cpi": 1,
        "num": 13,
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
        "value": 2
      },
      {
        "clues": 3,
        "max": 15,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 0,
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
        "max": 15,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 5,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 9,
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
          12,
          21,
          23
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 15,
        "score": 9,
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
        "rank": 3,
        "suit": 2,
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
        "failed": false,
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 26,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 1,
        "kind": "R",
        "list": [
          1,
          3,
          22,
          24,
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 15,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 4,
        "suit": 2,
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
        "max": 15,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 28,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 3,
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
        "order": 7,
        "p": 1,
        "rank": 5,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 29,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 23,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        2,
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
        2,
        4
      ],
      [
        1,
        4
      ],
      [
        1,
        3
      ],
      [
        0,
        1
      ],
      [
        2,
        5
      ],
      [
        0,
        4
      ],
      [
        0,
        5
      ],
      [
        0,
        2
      ],
      null,
      null,
      [
        2,
        1
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
        2
      ],
      [
        2,
        3
      ],
      [
        1,
        5
      ],
      null,
      [
        0,
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
      [
        2,
        3
      ],
      [
        2,
        2
      ],
      null,
      [
        0,
        1
      ],
      null,
      [
        1,
        1
      ],
      [
        0,
        3
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
      "variant_name": "Odds and Evens & Omni (3 Suits)"
    },
    "our_player_index": 2,
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
    "rlocks": false,
    "variant": "Odds and Evens & Omni (3 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-24T15:46:04.226",
  "turn": 24
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  const hanabi::State& s = game.state;
  ASSERT_EQ(s.cards_left, 0) << "guard: Rule 0b is gated on an empty deck";
  ASSERT_TRUE(hanabi::endgame::certain_plays(game).empty())
      << "guard: nothing here certainly scores, so Rule 0 stands down and the "
         "gamble is what must answer";

  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "red is a plain suit, so the button that lays it is Play";
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 12)
      << "the leftmost CLUED card that could be the required r4 -- not the "
         "leftmost card overall, which is an omni 1";
}
