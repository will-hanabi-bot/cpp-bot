// The giver may not use private sight to talk itself out of a promise.
//
// A stable colour clue promises the LEFTMOST touched card is playable. Whether
// that promise is true is a question about what the RECEIVER can work out --
// and the giver, who can see the receiver's hand, routinely knows better. When
// the giver lets that private knowledge downgrade the clue to a stall, it gives
// a clue it has itself decided is meaningless while the receiver reads a play.
//
// `provably_trash` is per-seat by design (CONVENTION.md §1g, replay 1967478
// T42) and that is right when READING somebody else's clue: the seat that acts
// has the better view. §1g's rule for the GIVER is the opposite one -- private
// knowledge REJECTS. Until v10.1.0 both sites used the reading.
//
// Replay 1973575 T62, "Odds and Evens & Dark Pink (6 Suits)". Alice =
// will-bot69, Bob = will-bot67. Stacks [5,5,4,5,4,5], so only g5 and p5 are
// playable; 1 clue token and 1 card left.
//
//   Bob:  p3(52)  r2(48)  r1(40)  g3(34)  p5(24)
//
// Purple touches slot 1 (the p3) and slot 5 (the p5), so the promise lands on
// slot 1, whose common empathy is {p3, p5} -- and p5 plays. But Alice can SEE
// the only p5, in Bob's own slot 5. She proved slot 1 was the dead p3, read the
// clue as a stall, and gave it. The reading was OTHER, so `predicts_a_strike`
// had nothing to veto and `choose_endgame_clue` rung 3 selected it. Bob cannot
// see his own p5. He read the play and bombed.
//
// Odd-to-Bob is illegal for the same reason on the rank side: under Odds and
// Evens the promise is the rightmost NEWLY touched card, slots 3/4/5 are
// already clued, so it lands on slot 1 again.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Dark Pink (6 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1973575, GiverSightMustNotDowngradeAPromise) {
  // Reconstruct exactly the Game the live bot saw at turn 62.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1973575,
  "debug": {
    "cards_left": 1,
    "clue_tokens": 1,
    "current_player_index": 1,
    "discards": [
      {
        "order": 41,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 33,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 28,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 26,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 14,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 1,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 4,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 0,
        "rank": 4,
        "suit": 3
      },
      {
        "order": 20,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 8,
        "rank": 1,
        "suit": 4
      },
      {
        "order": 25,
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
              4
            ],
            "inferred": 21231687,
            "info_lock": null,
            "order": 53,
            "possible": 21231687,
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
            "inferred": 247879,
            "info_lock": null,
            "order": 51,
            "possible": 260167,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              2
            ],
            "inferred": 73794,
            "info_lock": null,
            "order": 43,
            "possible": 75842,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 2097152,
            "info_lock": null,
            "order": 35,
            "possible": 2097152,
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
              1
            ],
            "inferred": 32769,
            "info_lock": null,
            "order": 2,
            "possible": 163845,
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
            "inferred": 21231687,
            "info_lock": null,
            "order": 49,
            "possible": 21231687,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 21231687,
            "info_lock": null,
            "order": 45,
            "possible": 21231687,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 21231687,
            "info_lock": null,
            "order": 37,
            "possible": 21231687,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 36869,
            "info_lock": null,
            "order": 15,
            "possible": 21155845,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 21155845,
            "info_lock": null,
            "order": 9,
            "possible": 21155845,
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
              4,
              3
            ],
            "inferred": 21231687,
            "info_lock": null,
            "order": 52,
            "possible": 21231687,
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
            "inferred": 4423751,
            "info_lock": null,
            "order": 48,
            "possible": 21200967,
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
            "inferred": 5,
            "info_lock": null,
            "order": 40,
            "possible": 5,
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
              3
            ],
            "inferred": 20480,
            "info_lock": null,
            "order": 34,
            "possible": 20480,
            "slot": 4,
            "status": "NONE",
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
            "inferred": 21004288,
            "info_lock": null,
            "order": 24,
            "possible": 21135360,
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
        "k": "discard",
        "v": "None"
      },
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
        "v": "Discard"
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
        "v": "Stall"
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
      }
    ],
    "play_stacks": [
      5,
      5,
      4,
      5,
      4,
      5
    ],
    "strikes": 0,
    "turn_count": 62,
    "waiting": []
  },
  "game_id": 5729,
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
        "rank": 1,
        "suit": 2,
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
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
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
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          12,
          14
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
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 4,
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
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 4
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": -1,
        "suit": -1,
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
        "suit": 2,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 4,
        "suit": 5,
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
        "kind": "R",
        "list": [
          10,
          13,
          14,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "failed": false,
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
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
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "giver": 0,
        "kind": "R",
        "list": [
          12,
          18,
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
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
        "order": 21,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "order": 20,
        "p": 2,
        "rank": 1,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "giver": 0,
        "kind": "R",
        "list": [
          13,
          14,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "order": 21,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "giver": 0,
        "kind": "C",
        "list": [
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 5
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
        "kind": "R",
        "list": [
          12,
          18,
          22,
          24
        ],
        "t": "clue",
        "target": 2,
        "value": 1
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
        "giver": 2,
        "kind": "R",
        "list": [
          1,
          2,
          3,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 30,
        "score": 7,
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
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 25,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 7,
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
        "rank": 2,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 26,
        "p": 1,
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
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          1,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 3,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 22,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 26,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 28,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 8,
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
          3,
          25,
          27
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 24,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 25,
        "p": 0,
        "rank": 4,
        "suit": 4,
        "t": "discard"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 25,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 28,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 30,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 30,
        "score": 8,
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
          19,
          30
        ],
        "t": "clue",
        "target": 1,
        "value": 1
      },
      {
        "clues": 5,
        "max": 30,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 27,
        "t": "turn"
      },
      {
        "order": 29,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 28,
        "t": "turn"
      },
      {
        "order": 30,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 32,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 10,
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
          5,
          32
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 4,
        "max": 30,
        "score": 10,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 30,
        "t": "turn"
      },
      {
        "order": 31,
        "p": 0,
        "rank": 2,
        "suit": 0,
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
        "clues": 4,
        "max": 30,
        "score": 11,
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
          1,
          2,
          3,
          33
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 30,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 32,
        "t": "turn"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 34,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 12,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 33,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 33,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 35,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 12,
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
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 3,
        "max": 30,
        "score": 12,
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
        "suit": 1,
        "t": "play"
      },
      {
        "order": 36,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 36,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          36
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 2,
        "max": 30,
        "score": 13,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 37,
        "t": "turn"
      },
      {
        "order": 32,
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
        "clues": 2,
        "max": 30,
        "score": 14,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 38,
        "t": "turn"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 38,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 39,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          34
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 15,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 40,
        "t": "turn"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 5,
        "suit": 1,
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
        "max": 30,
        "score": 16,
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
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 40,
        "p": 2,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 42,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          34
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 17,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 43,
        "t": "turn"
      },
      {
        "order": 39,
        "p": 1,
        "rank": 3,
        "suit": 5,
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
        "clues": 1,
        "max": 30,
        "score": 18,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 44,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 4,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 19,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 45,
        "t": "turn"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 5,
        "t": "play"
      },
      {
        "order": 43,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 20,
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
          27,
          35,
          43
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 1,
        "max": 30,
        "score": 20,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 47,
        "t": "turn"
      },
      {
        "order": 36,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 44,
        "p": 2,
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 21,
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
          24,
          34,
          40,
          42,
          44
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 0,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 49,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 41,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 45,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 21,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 50,
        "t": "turn"
      },
      {
        "order": 42,
        "p": 2,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 46,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 51,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          40,
          46
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 0,
        "max": 30,
        "score": 22,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 52,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 47,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 23,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 53,
        "t": "turn"
      },
      {
        "order": 46,
        "p": 2,
        "rank": 5,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 48,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 54,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          34
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 0,
        "max": 30,
        "score": 24,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 55,
        "t": "turn"
      },
      {
        "order": 47,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 49,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 0,
        "max": 30,
        "score": 25,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 56,
        "t": "turn"
      },
      {
        "order": 44,
        "p": 2,
        "rank": 5,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 50,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 57,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 51,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 2,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 58,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          35
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 1,
        "max": 30,
        "score": 26,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 59,
        "t": "turn"
      },
      {
        "order": 50,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 52,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 27,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 60,
        "t": "turn"
      },
      {
        "order": 27,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 53,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 1,
        "max": 30,
        "score": 28,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 61,
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
        2,
        1
      ],
      [
        3,
        1
      ],
      [
        5,
        5
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
        0,
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
      null,
      [
        5,
        1
      ],
      [
        3,
        1
      ],
      [
        1,
        3
      ],
      [
        4,
        4
      ],
      [
        1,
        4
      ],
      null,
      [
        2,
        1
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
        1,
        5
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
        4,
        3
      ],
      [
        5,
        2
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
        1,
        3
      ],
      [
        2,
        4
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
        1,
        2
      ],
      [
        0,
        2
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
        2,
        3
      ],
      [
        4,
        2
      ],
      [
        0,
        4
      ],
      null,
      [
        0,
        3
      ],
      [
        5,
        3
      ],
      [
        0,
        1
      ],
      [
        0,
        4
      ],
      [
        3,
        3
      ],
      [
        1,
        2
      ],
      [
        3,
        5
      ],
      null,
      [
        0,
        5
      ],
      [
        2,
        2
      ],
      [
        0,
        2
      ],
      null,
      [
        2,
        3
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
        2,
        4
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
      "variant_name": "Odds and Evens & Dark Pink (6 Suits)"
    },
    "our_player_index": 1,
    "reactive_overrides": [
      {
        "clue_value": 5,
        "even": false,
        "kind": "C",
        "reactive_value": 1
      }
    ],
    "rlocks": false,
    "variant": "Odds and Evens & Dark Pink (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T07:57:49.898",
  "turn": 62
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards: the position is the one described above --------------------
  ASSERT_EQ(s.our_player_index, 1) << "we are will-bot69";
  const int bob = s.next_player_index(1);
  ASSERT_EQ(bob, 2) << "will-bot67";

  // Purple is colour value 4 in Red/Yellow/Green/Blue/Purple/Dark Pink.
  const int purple = 4;
  ASSERT_EQ(s.variant->clue_colour_names[purple], "Purple");

  // The p5 really is in Bob's own hand, where we can see it and he cannot.
  const int bob_slot5 = s.hands[bob][4];
  auto slot5_id = s.deck[bob_slot5].id();
  ASSERT_TRUE(slot5_id.has_value());
  EXPECT_TRUE(s.is_playable(*slot5_id))
      << "guard: Bob's slot 5 is the playable p5";

  // ...and Purple's promise lands on slot 1, which is the dead p3.
  const std::vector<int> touched =
      s.clue_touched(s.hands[bob], hanabi::ClueKind::COLOUR, purple);
  hanabi::ClueAction purple_to_bob{1, bob, touched,
                                   hanabi::BaseClue{hanabi::ClueKind::COLOUR,
                                                    purple}};
  auto promised = hanabi::reactor0::leftmost_could_be_playable(
      game, purple_to_bob, purple_to_bob.list_);
  ASSERT_TRUE(promised.has_value()) << "guard: the clue does promise a card";
  EXPECT_EQ(*promised, s.hands[bob][0]) << "guard: it promises slot 1";
  auto promised_id = s.deck[*promised].id();
  ASSERT_TRUE(promised_id.has_value());
  EXPECT_FALSE(s.is_playable(*promised_id))
      << "guard: and that card is NOT playable -- the promise is a lie";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  if (const auto* colour = std::get_if<hanabi::PerformColour>(&action)) {
    EXPECT_FALSE(colour->target == bob && colour->value == purple)
        << "Purple to Bob promises his slot 1 is playable and it is the dead "
           "p3; the giver may not give it just because she can privately prove "
           "the promise is empty";
  }
  // The rank half: `odd` is rank clue value 1 under Odds and Evens, and it is
  // illegal here for the same reason.
  if (const auto* rank = std::get_if<hanabi::PerformRank>(&action)) {
    EXPECT_FALSE(rank->target == bob && rank->value == 1)
        << "odd to Bob promises the rightmost NEWLY touched card -- slot 1 "
           "again, since slots 3/4/5 are already clued";
  }
}
