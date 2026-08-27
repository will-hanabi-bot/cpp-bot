// Phase B walks past a pairing whose connector its own inference has excluded.
//
// Replay 1973406 T18, `Odds and Evens & Cocoa Rainbow (5 Suits)` with
// `/set 1 even 5` in force -- so an R1 clue sits in the EVEN bucket with anchor
// 5, and `react_slot + target_slot = 5`.
//
// yagami_black clues R1 to Neema. Two of her cards are one away:
//
//   slot 3  g5, green on 3  -> pairs with our slot 2, inferred {g1}
//   slot 4  b3, blue on 1   -> pairs with our slot 1, unclued
//
// The walk is leftmost-first, so g5 comes first. Our slot 2 cannot be the g4
// connector -- its inference says g1 -- but the guard asked
// `effective_possible_for`, which filters `possible` (still the whole green
// suit), so the pairing passed the guard and then died at `target_play`, which
// narrows `inferred`. Phase B answered that refusal by returning `nullopt` for
// the WHOLE clue rather than walking on, so it never reached slot 4, whose
// pairing is our slot 1 -- an unclued card that can perfectly well hold the b2.
//
// The clue read as a MISTAKE, yagami_blue never reacted, and it gave Neema a
// rank-2 lock clue instead of blind-playing the connector.
//
// Two fixes, either of which is sufficient and both of which are right: the
// guard now asks `possibilities()` as well as `effective_possible_for` (the pin
// at the bottom of the loop writes `inferred = {connector}` unconditionally,
// which would WIDEN a reading that had excluded it), and a refused stamp now
// `continue`s, as Phase A, Phase C and both colour modes have since v10.6.0.
//
// Note the receiver's own stamp is NOT made here: reactor0 stamps the receiver
// when the reacter actually acts (CONVENTION.md 1d), so the pairing is checked
// through `waiting.front().react_order` and `meta[].react_target_order`.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Cocoa Rainbow (5 Suits). 3 players, our_player_index=2.

TEST(BadReactiveTarget1973406, FinesseWalksPastADeadConnector) {
  // Reconstruct exactly the Game the live bot saw at turn 18.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "yagami_blue",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1973406,
  "debug": {
    "cards_left": 20,
    "clue_tokens": 3,
    "current_player_index": 2,
    "discards": [
      {
        "order": 20,
        "rank": 1,
        "suit": 2
      },
      {
        "order": 18,
        "rank": 1,
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
              0,
              4
            ],
            "inferred": 32844106,
            "info_lock": null,
            "order": 24,
            "possible": 32844106,
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
              2
            ],
            "inferred": 262472,
            "info_lock": null,
            "order": 22,
            "possible": 328010,
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
              5
            ],
            "inferred": 21504,
            "info_lock": null,
            "order": 3,
            "possible": 21504,
            "slot": 3,
            "status": "NONE",
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
            "inferred": 688821,
            "info_lock": null,
            "order": 1,
            "possible": 688821,
            "slot": 4,
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
            "inferred": 328010,
            "info_lock": null,
            "order": 0,
            "possible": 328010,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "Neema",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              3
            ],
            "inferred": 33216181,
            "info_lock": null,
            "order": 15,
            "possible": 33216181,
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
              5
            ],
            "inferred": 33216181,
            "info_lock": null,
            "order": 9,
            "possible": 33216181,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              4
            ],
            "inferred": 338186,
            "info_lock": null,
            "order": 8,
            "possible": 338250,
            "slot": 3,
            "status": "CALLED_TO_DISCARD",
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
            "inferred": 33216181,
            "info_lock": null,
            "order": 7,
            "possible": 33216181,
            "slot": 4,
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
            "inferred": 33216181,
            "info_lock": null,
            "order": 6,
            "possible": 33216181,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 23,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 1024,
            "info_lock": null,
            "order": 14,
            "possible": 31744,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 31,
            "info_lock": null,
            "order": 12,
            "possible": 31,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1016800,
            "info_lock": null,
            "order": 11,
            "possible": 1016800,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1016800,
            "info_lock": null,
            "order": 10,
            "possible": 1016800,
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
        "k": "clue",
        "v": "Mistake"
      }
    ],
    "play_stacks": [
      1,
      3,
      3,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 18,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_value": 1,
        "focus_slot": 5,
        "giver": 1,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          24,
          22,
          3,
          1,
          0
        ],
        "rlocks": false,
        "turn": 17
      }
    ]
  },
  "game_id": 5544,
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
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 5,
        "suit": 2,
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
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 5,
        "suit": 3,
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
          12
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
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 3,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 3,
        "suit": 2,
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
        "kind": "R",
        "list": [
          1,
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 1
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
        "order": 13,
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
        "order": 2,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "kind": "C",
        "list": [
          14,
          17
        ],
        "t": "clue",
        "target": 2,
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
        "order": 17,
        "p": 2,
        "rank": 2,
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
        "failed": false,
        "order": 18,
        "p": 0,
        "rank": 1,
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "giver": 1,
        "kind": "C",
        "list": [
          12,
          19
        ],
        "t": "clue",
        "target": 2,
        "value": 0
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
        "order": 19,
        "p": 2,
        "rank": 1,
        "suit": 0,
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
        "failed": false,
        "order": 20,
        "p": 0,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 25,
        "score": 6,
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
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 21,
        "p": 2,
        "rank": 3,
        "suit": 1,
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
        "clues": 4,
        "max": 25,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "giver": 1,
        "kind": "R",
        "list": [
          1,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 3,
        "max": 25,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 17,
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
        3,
        3
      ],
      [
        1,
        2
      ],
      [
        2,
        5
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
        4,
        5
      ],
      [
        0,
        5
      ],
      [
        3,
        4
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
        1
      ],
      null,
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
        2
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
        2,
        1
      ],
      [
        1,
        3
      ],
      [
        1,
        2
      ],
      null,
      [
        0,
        4
      ]
    ],
    "names": [
      "Neema",
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
      "variant_name": "Odds and Evens & Cocoa Rainbow (5 Suits)"
    },
    "our_player_index": 2,
    "reactive_overrides": [
      {
        "clue_value": 1,
        "even": true,
        "kind": "R",
        "reactive_value": 5
      }
    ],
    "rlocks": false,
    "variant": "Odds and Evens & Cocoa Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T06:02:46.598",
  "turn": 18
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;
  const int us = s.our_player_index;              // yagami_blue, the reacter
  const int receiver = 0;                         // Neema, the clued seat

  const int our_slot1 = s.hands[us][0];           // order 23 -- can be the b2
  const int our_slot2 = s.hands[us][1];           // order 14 -- inferred {g1}
  const int neema_slot3 = s.hands[receiver][2];   // order 3  -- g5, one away
  const int neema_slot4 = s.hands[receiver][3];   // order 1  -- b3, one away
  ASSERT_EQ(our_slot1, 23);
  ASSERT_EQ(our_slot2, 14);
  ASSERT_EQ(neema_slot3, 3);
  ASSERT_EQ(neema_slot4, 1);

  // The override is what puts this clue in the even bucket at anchor 5. If it
  // ever stops being restored, the fixture is testing something else entirely.
  ASSERT_FALSE(game.waiting.empty());
  EXPECT_EQ(game.waiting.front().focus_slot, 5)
      << "`/set 1 even 5` -- the R1 clue's reactive value";
  EXPECT_EQ(game.waiting.front().reacter, us);
  EXPECT_EQ(game.waiting.front().receiver, receiver);

  // Both of Neema's one-away cards really are one away, so the walk has a
  // genuine first candidate to reject before it reaches the good one.
  ASSERT_TRUE(s.deck[neema_slot3].id().has_value());
  ASSERT_TRUE(s.deck[neema_slot4].id().has_value());
  EXPECT_EQ(s.playable_away(*s.deck[neema_slot3].id()), 1) << "g5, green on 3";
  EXPECT_EQ(s.playable_away(*s.deck[neema_slot4].id()), 1) << "b3, blue on 1";

  // 1. The clue must READ. A MISTAKE here is the bug.
  auto last = game.last_move();
  ASSERT_TRUE(last.has_value());
  ASSERT_TRUE(std::holds_alternative<hanabi::ClueInterp>(*last));
  EXPECT_EQ(std::get<hanabi::ClueInterp>(*last), hanabi::ClueInterp::REACTIVE)
      << "an even-bucket finesse; reading it as a MISTAKE is the bug";

  // 2. The walk skipped slot 3's pairing and landed on slot 4's.
  EXPECT_EQ(game.waiting.front().react_order, our_slot1)
      << "calc_slot(5, 4, 5) = 1 -- Neema's b3 pairs with our slot 1";
  EXPECT_EQ(game.meta[our_slot1].react_target_order, neema_slot4)
      << "and the pairing recorded on the card names her slot 4";
  EXPECT_EQ(game.meta[our_slot2].status, hanabi::CardStatus::NONE)
      << "slot 2 was the rejected pairing and must carry nothing";

  // 3. It is a blind PLAY of the connector, urgent, pinned to the b2.
  EXPECT_EQ(game.meta[our_slot1].status, hanabi::CardStatus::CALLED_TO_PLAY);
  EXPECT_TRUE(game.meta[our_slot1].urgent) << "a reacter call is urgent";

  // 4. And it is actioned.
  hanabi::PerformAction action = game.take_action();
  auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr)
      << "the reaction is the blind play of the connector, got "
      << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(play->target, our_slot1);
  EXPECT_EQ(std::get_if<hanabi::PerformRank>(&action), nullptr)
      << "locking Neema with a rank 2 clue was the bug";
}
