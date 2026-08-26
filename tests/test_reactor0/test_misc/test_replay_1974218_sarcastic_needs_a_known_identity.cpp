// A sarcastic discard is a SIGNAL, and a signal needs something to point at.
//
// Replay 1974218. At T8 yagami discarded a clued i4 whose empathy was all six
// 4s -- a rank-4 clue in a six-suit variant tells you the rank and nothing
// else. `interpret_useful_dc` read it as sarcastic anyway; `try_finding` found
// no i4 in any hand, because the second one was still in the DECK (it reached
// will-bot67 as order 19 one turn later), and fell back on "then it must be in
// MY hand", linking over our own. Fourteen turns later that link collapsed onto
// order 0 -- a CARDINAL 2 -- and stamped it SARCASTIC with inferred {i4}.
//
// The damage lands at T24. will-bot67 gives a rank-3 reactive PLAY clue; rank
// is the even bucket, so anchor 3 pairs react slot 5 with target slot 3, and
// both cards are playable: our ca2 and yagami's c3. We never see it, because
// `target_play` narrows INFERRED, and {i4} intersected with the playable set is
// empty -- so Phase A walks past the ca2 in silence, Phase B finds a "finesse"
// pairing yagami's t3 with our slot 1, and slot 1 is a dark 3 with the dark
// stack on 0. Strike, game over.
//
// The fix is the precondition, not the fallback: the team must already know
// what was thrown. See the control in test_basics/test_sarcastic_needs_a_known
// _identity.cpp -- a sarcastic on a card whose identity IS pinned still fires.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Dual-Color & Dark Prism (6 Suits). 3 players, our_player_index=0.

TEST(MiscReplay1974218, ASarcasticSignalNeedsAKnownIdentity) {
  // Reconstruct exactly the Game the live bot saw at turn 25.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1974218,
  "debug": {
    "cards_left": 27,
    "clue_tokens": 3,
    "current_player_index": 0,
    "discards": [
      {
        "order": 17,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 13,
        "rank": 1,
        "suit": 3
      },
      {
        "order": 7,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 10,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 23,
        "rank": 3,
        "suit": 4
      },
      {
        "order": 6,
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
            "focused": true,
            "id": null,
            "inferred": 2,
            "info_lock": 35721346,
            "order": 26,
            "possible": 1073741823,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 554189328,
            "info_lock": null,
            "order": 24,
            "possible": 554189328,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 484915662,
            "info_lock": null,
            "order": 22,
            "possible": 484915662,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 348520460,
            "info_lock": null,
            "order": 20,
            "possible": 350683150,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 262144,
            "info_lock": null,
            "order": 0,
            "possible": 350683150,
            "slot": 5,
            "status": "SARCASTIC",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot69",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              2
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 27,
            "possible": 935194491,
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
              3
            ],
            "inferred": 138412164,
            "info_lock": null,
            "order": 18,
            "possible": 138412164,
            "slot": 2,
            "status": "CHOP_MOVED",
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
            "inferred": 135168,
            "info_lock": null,
            "order": 15,
            "possible": 135168,
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
              3
            ],
            "inferred": 135168,
            "info_lock": null,
            "order": 9,
            "possible": 135168,
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
              5
            ],
            "inferred": 553648656,
            "info_lock": null,
            "order": 8,
            "possible": 553648656,
            "slot": 5,
            "status": "CHOP_MOVED",
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
            "id": [
              2,
              2
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 25,
            "possible": 1073741823,
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
              4
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 19,
            "possible": 935194491,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              4,
              3
            ],
            "inferred": 138547332,
            "info_lock": null,
            "order": 14,
            "possible": 138547332,
            "slot": 3,
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
            "inferred": 935194491,
            "info_lock": null,
            "order": 12,
            "possible": 935194491,
            "slot": 4,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              2,
              5
            ],
            "inferred": 935194491,
            "info_lock": null,
            "order": 11,
            "possible": 935194491,
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
        "k": "clue",
        "v": "Reactive"
      },
      {
        "k": "discard",
        "v": "None"
      },
      {
        "k": "clue",
        "v": "Lock"
      },
      {
        "k": "discard",
        "v": "Sarcastic"
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
        "v": "Lock"
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
        "v": "Reactive"
      }
    ],
    "play_stacks": [
      1,
      2,
      2,
      1,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 25,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_value": 3,
        "focus_slot": 3,
        "giver": 2,
        "inverted": false,
        "react_order": 26,
        "reacter": 0,
        "receiver": 1,
        "receiver_hand": [
          27,
          18,
          15,
          9,
          8
        ],
        "rlocks": false,
        "turn": 24
      }
    ]
  },
  "game_id": 102,
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
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 3,
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
        "rank": 5,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 3,
        "suit": 4,
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
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 2,
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
          1,
          2,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
        "kind": "C",
        "list": [
          1,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "failed": false,
        "order": 13,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 17,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "giver": 0,
        "kind": "R",
        "list": [
          6,
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 4
      },
      {
        "clues": 5,
        "max": 30,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 1,
        "rank": 3,
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
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 4,
        "suit": 3,
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
        "num": 9,
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
        "order": 20,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 3,
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
          1,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
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
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 30,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 5
      },
      {
        "clues": 6,
        "max": 30,
        "score": 3,
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
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 5,
        "max": 30,
        "score": 3,
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
          9,
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 4,
        "max": 30,
        "score": 3,
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
        "rank": 1,
        "suit": 1,
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
        "clues": 4,
        "max": 30,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 6,
        "p": 1,
        "rank": 4,
        "suit": 4,
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
        "clues": 5,
        "max": 30,
        "score": 4,
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
          2
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
        "cpi": 0,
        "num": 18,
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
        "order": 24,
        "p": 0,
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
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "R",
        "list": [
          24
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 30,
        "score": 5,
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
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 25,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 26,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 23,
        "p": 1,
        "rank": 3,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
        "num": 23,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          9,
          15,
          18
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
        "num": 24,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      null,
      [
        1,
        1
      ],
      [
        2,
        1
      ],
      [
        1,
        2
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
        4,
        4
      ],
      [
        3,
        4
      ],
      [
        0,
        5
      ],
      [
        3,
        3
      ],
      [
        4,
        1
      ],
      [
        2,
        5
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
        3
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
        1,
        3
      ],
      [
        0,
        3
      ],
      [
        3,
        4
      ],
      null,
      [
        2,
        2
      ],
      null,
      [
        4,
        3
      ],
      null,
      [
        2,
        2
      ],
      null,
      [
        3,
        2
      ]
    ],
    "names": [
      "will-bot69",
      "yagami_black",
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
      "variant_name": "Dual-Color & Dark Prism (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": false,
    "variant": "Dual-Color & Dark Prism (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T22:44:57.024",
  "turn": 25
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards ---------------------------------------------------------------
  // `apply_snapshot` replays the recorded actions from the start, so the T8
  // discard really is re-interpreted here -- this is not a frozen state dump.
  ASSERT_EQ(s.our_player_index, 0) << "we are will-bot69";
  // Order 0 is a ca2 and order 26 a d3 -- but both are in OUR hand, and the log
  // this replays was written from our own seat, so neither `state.deck` nor
  // `deck_ids` carries an identity for them. The identities are on record in
  // will-bot67's log for the same game; here they are asserted through the only
  // channel that survives, which is what the bot does with them.
  ASSERT_EQ(s.hands[0].size(), 5u);
  ASSERT_EQ(s.hands[0][4], 0) << "order 0 is our slot 5";
  ASSERT_EQ(s.hands[0][0], 26) << "order 26 is our slot 1";
  ASSERT_EQ(s.play_stacks[4], 1) << "cardinal is on 1, so the ca2 plays";
  ASSERT_EQ(s.play_stacks[5], 0) << "dark is on 0, so the d3 strikes";

  const hanabi::Identity i4{3, 4};
  EXPECT_NE(game.common.thoughts[0].inferred, hanabi::IdentitySet::single(i4))
      << "the ca2 must not be pinned to {i4} by a sarcastic that was never "
         "signalled -- yagami's discarded i4 read as all six 4s";
  EXPECT_NE(game.meta[0].status, hanabi::CardStatus::SARCASTIC);
  EXPECT_NE(game.meta[26].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "and with the ca2 readable, Phase A never falls through to the "
         "Phase B finesse on slot 1";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 0)
      << "play the cardinal 2, which is what the rank-3 reactive play clue "
         "asked for";
  EXPECT_NE(std::get<hanabi::PerformPlay>(action).target, 26)
      << "order 26 is a dark 3 on an empty dark stack -- this play lost the "
         "game";
}
