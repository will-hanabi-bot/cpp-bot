// A reacter call whose paired target has gone is RELEGATED, not stranded.
//
// Replay 1975197 T5, "Extremely Ambiguous & Dark Orange (6 Suits)". yagami's
// reactive rank-2 clue pairs will-bot67's slot 3 (order 7) with will-bot69's
// slot 4 (order 11). will-bot67 DEFERS at T2, cluing instead of reacting, and
// will-bot69 plays order 11 at T3 -- so the target leaves the receiver's hand
// and there is nobody left decoding against the call.
//
// v9.3.0 made such a call stop being urgent, correctly. But it de-urgented it
// inside `decide.cpp`'s urgent scan, which merely SKIPPED it and left the flag
// set -- and `urgent` is the discriminator `calls_of` routes on. So the call
// went on being filed as a `reacter_ctp`, where the only thing that ever
// actions it is the very scan that had just skipped it: `choose_action` has no
// rung 1 (`calls.h`). The call became permanently unactionable, and at T5 the
// bot discarded its chop while holding one read {i1,s1,b1,n1} -- every reading
// playable.
//
// Rule 0 of `enforce_call_invariants` now clears the flag for real, which moves
// the card into the receiver deque, onto the pitch list, and within reach of
// phase 2. The reading and the call itself are untouched.
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Extremely Ambiguous & Dark Orange (6 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1975197, ASpentReacterCallRelegatesInsteadOfStranding) {
  // Reconstruct exactly the Game the live bot saw at turn 5.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1975197,
  "debug": {
    "cards_left": 38,
    "clue_tokens": 6,
    "current_player_index": 1,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              5
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 16,
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
              4,
              4
            ],
            "inferred": 969831324,
            "info_lock": null,
            "order": 4,
            "possible": 1039104990,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              4,
              2
            ],
            "inferred": 1039104990,
            "info_lock": null,
            "order": 2,
            "possible": 1039104990,
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
              5
            ],
            "inferred": 1039104990,
            "info_lock": null,
            "order": 1,
            "possible": 1039104990,
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
              1
            ],
            "inferred": 1082401,
            "info_lock": null,
            "order": 0,
            "possible": 34636833,
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
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 9,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 8,
            "possible": 1073741823,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 1082369,
            "info_lock": 34636833,
            "order": 7,
            "possible": 1073741823,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 6,
            "possible": 1073741823,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 5,
            "possible": 1073741823,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "will-bot67",
        "player": 1
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
            "inferred": 1073741823,
            "info_lock": null,
            "order": 15,
            "possible": 1073741823,
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
              2
            ],
            "inferred": 69273666,
            "info_lock": null,
            "order": 14,
            "possible": 69273666,
            "slot": 2,
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
            "inferred": 1004468157,
            "info_lock": null,
            "order": 13,
            "possible": 1004468157,
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
              3
            ],
            "inferred": 1004468157,
            "info_lock": null,
            "order": 12,
            "possible": 1004468157,
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
              1
            ],
            "inferred": 1004468157,
            "info_lock": null,
            "order": 10,
            "possible": 1004468157,
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
      0,
      2,
      0,
      0,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 5,
    "waiting": []
  },
  "game_id": 1246,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 5,
        "suit": 4,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 4,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 5,
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
        "giver": 1,
        "kind": "R",
        "list": [
          0
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 30,
        "score": 0,
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
        "suit": 1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
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
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 5,
        "suit": 3,
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
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        1,
        1
      ],
      [
        4,
        5
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
        4,
        4
      ],
      null,
      null,
      null,
      null,
      null,
      [
        0,
        1
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
        5,
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
        3,
        5
      ]
    ],
    "names": [
      "yagami_black",
      "will-bot67",
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
      "variant_name": "Extremely Ambiguous & Dark Orange (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Extremely Ambiguous & Dark Orange (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-27T17:32:27.259",
  "turn": 5
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards ---------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 1) << "we are will-bot67";
  ASSERT_EQ(s.hands[1][2], 7) << "order 7 is our slot 3";
  ASSERT_TRUE(game.waiting.empty())
      << "guard: the connection is gone -- we deferred, so this is the case "
         "`react_target_order` exists for";

  // The call and its inference stand; only the urgency has gone.
  EXPECT_EQ(game.meta[7].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "relegated, not erased";
  EXPECT_FALSE(game.meta[7].urgent)
      << "the receiver played the paired card, so nobody is decoding against "
         "this any more";
  EXPECT_TRUE(game.common.thoughts[7].inferred.exists([&](hanabi::Identity i) {
    return s.is_playable(i);
  })) << "and the reading it installed still names something playable";

  const int paired = game.meta[7].react_target_order;
  ASSERT_GE(paired, 0);
  const auto& receiver_hand = s.hands[s.holder_of(paired)];
  ASSERT_EQ(std::find(receiver_hand.begin(), receiver_hand.end(), paired),
            receiver_hand.end())
      << "guard: the paired card really has left the receiver's hand";

  // It has moved to the deque the pitch list is built from -- which is the
  // whole mechanism, and what was missing before.
  const auto calls = hanabi::reactor0::calls_of(game, 1);
  EXPECT_NE(std::find(calls.receiver_ctp.begin(), calls.receiver_ctp.end(), 7),
            calls.receiver_ctp.end());
  EXPECT_FALSE(calls.has_reaction());

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 7)
      << "play the called card";
  EXPECT_NE(std::get<hanabi::PerformPlay>(action).target, 9)
      << "order 9 is the chop, which the rung-12 floor threw when the call was "
         "unreachable";
}
