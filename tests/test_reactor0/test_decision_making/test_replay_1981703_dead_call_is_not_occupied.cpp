// A call Alice cannot action must not make her "occupied".
// Replay 1981703 T19, `Synesthesia & Brown (3 Suits)`, reactor0.
//
// yagami_green (seat 0) blind-pitched a br5 here and ended the game on the
// second strike. The chain:
//
//   T17  yagami_black clues brown to green. Green is CATHY for it (giver 1 ->
//        reacter 2 -> receiver 0), so it is reactive.
//   T18  yagami_blue, the reacter, plays order 24 = br3 -- and green's called
//        card, order 23, is ALSO br3. Blue played the very card green held, so
//        green's copy is trash. A dupe, and one yagami_black could not foresee.
//   T19  <-- here
//
// Common still reads green's card as br4, which is playable and a legal reading.
// Green knows better: it can SEE the real br4 in blue's hand (order 12), so its
// own view rules br4 out, `pitch_would_strike` is true and `call_is_actionable`
// drops the call from the pitch list. That part worked -- green refused to bomb
// the miscalled card.
//
// What failed is what came next. The STAMP survived and `requires_high_tier`
// counted it with no actionability test, so green was "occupied" -- and THE TWO
// TIER RULES TAKE DIFFERENT PACE THRESHOLDS. 1a (occupied) runs at any pace
// above zero, so at pace 1 it demanded HIGH and flattened all five candidates
// ("tier_gate_rejected_all"). 2a (unoccupied) only runs at `pace() >= 3`, so an
// unoccupied Alice here is outside the window and not gated at all.
//
// That split is the whole mechanism -- NOT 2a's locked clause, which never
// applied: `common.thinks_locked` is false in this position even though phase 2
// reaches rung `12.locked_no_chop`, because the two ask different questions.
//
// With the pool emptied, section 4 never received a candidate, and phase 2 fell
// to rung 12, which pitches `hands.front()` blind: order 25, the br5.
//
// v13.3.0 counts only calls she can action. The turn becomes the clue that was
// there all along: BROWN to yagami_black, which Synesthesia's table reads as
// pitch slot 4 -- and his slot 4 is order 7, a br4, playable with brown on 3.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Synesthesia & Brown (3 Suits). 3 players, our_player_index=0.

TEST(DecisionMaking1981703, T19ADeadCallDoesNotGateTheClueAway) {
  // Reconstruct exactly the Game the live bot saw at turn 19.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_green",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1981703,
  "debug": {
    "cards_left": 3,
    "clue_tokens": 3,
    "current_player_index": 0,
    "discards": [
      {
        "order": 4,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 18,
        "rank": 1,
        "suit": 1
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31876,
            "info_lock": null,
            "order": 25,
            "possible": 31876,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 8192,
            "info_lock": null,
            "order": 23,
            "possible": 31748,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 866,
            "info_lock": null,
            "order": 2,
            "possible": 866,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31748,
            "info_lock": null,
            "order": 1,
            "possible": 31748,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31748,
            "info_lock": null,
            "order": 0,
            "possible": 31748,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_green",
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
            "inferred": 32767,
            "info_lock": null,
            "order": 21,
            "possible": 32767,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 19,
            "possible": 32767,
            "slot": 2,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 8,
            "possible": 32767,
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
              4
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 7,
            "possible": 32767,
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
              2
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 5,
            "possible": 32767,
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
            "id": [
              2,
              1
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 26,
            "possible": 32767,
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
              1
            ],
            "inferred": 32767,
            "info_lock": null,
            "order": 22,
            "possible": 32767,
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
              4
            ],
            "inferred": 2,
            "info_lock": null,
            "order": 16,
            "possible": 34,
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
            "inferred": 30720,
            "info_lock": null,
            "order": 12,
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
              1,
              5
            ],
            "inferred": 768,
            "info_lock": null,
            "order": 11,
            "possible": 832,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_blue",
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
      }
    ],
    "pending_reactions": [],
    "play_stacks": [
      4,
      3,
      3
    ],
    "strikes": 1,
    "turn_count": 19,
    "waiting": []
  },
  "game_id": 9047,
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
        "rank": 2,
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
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 2,
        "suit": 0,
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
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
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
        "suit": 2,
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
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 4,
        "suit": 1,
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
          10,
          11,
          16
        ],
        "t": "clue",
        "target": 2,
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
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 0,
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
        "order": 14,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
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
          16,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 5,
        "max": 15,
        "score": 4,
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
        "rank": 4,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 5,
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
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 6,
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
          20
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 4,
        "max": 15,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 21,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 8,
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
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 8,
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
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 4,
        "max": 15,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "num": 1,
        "order": 18,
        "t": "strike",
        "turn": 14
      },
      {
        "failed": true,
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 25,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
          0,
          1,
          23,
          25
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 24,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 26,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      null,
      null,
      null,
      [
        2,
        2
      ],
      [
        0,
        1
      ],
      [
        1,
        2
      ],
      [
        0,
        1
      ],
      [
        2,
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
        1,
        2
      ],
      [
        1,
        5
      ],
      [
        2,
        4
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
        2,
        1
      ],
      [
        1,
        4
      ],
      [
        0,
        4
      ],
      [
        1,
        1
      ],
      [
        0,
        4
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
        2,
        1
      ],
      null,
      [
        2,
        3
      ],
      null,
      [
        2,
        1
      ]
    ],
    "names": [
      "yagami_green",
      "yagami_black",
      "yagami_blue"
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
      "variant_name": "Synesthesia & Brown (3 Suits)"
    },
    "our_player_index": 0,
    "rlocks": false,
    "variant": "Synesthesia & Brown (3 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-01T18:47:58.793",
  "turn": 19
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.state.pace(), 1) << "guard: inside the occupied gate's window";
  ASSERT_FALSE(game.common.thinks_locked(game, 0))
      << "guard: 2a's locked clause is NOT what saves this -- the fix works "
         "because an unoccupied Alice at pace 1 is outside 2a's pace >= 3 "
         "window, while 1a bites from pace 1";

  hanabi::PerformAction action = game.take_action();
  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  EXPECT_EQ(play, nullptr)
      << "she must not blind-pitch: order " << (play ? play->target : -1)
      << " is what rung 12 reached for when the gate emptied the clue pool";
  auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(colour, nullptr) << "the turn is a colour clue";
  EXPECT_EQ(colour->target, 1) << "to yagami_black";
  EXPECT_EQ(colour->value, 1)
      << "Blue, of r/b/br -- v14.0.0's pitch of slot 4, and his slot 4 is the "
         "br4. Through v13.5.0 the same call was spelled Brown, the old "
         "catch-all; the table rewrite moved slot-4-pitch onto Blue";
}
