// A reaction stops being urgent once its target has left the receiver's hand.
//
// A pending reaction interrupts everything below it in the Precedence list
// because the RECEIVER is decoding against it: he learns which of his own slots
// the clue named from which of ours we action. That is the whole justification,
// and it expires. Once the paired card has left his hand there is nobody left to
// inform, and the call must stop pre-empting the turn.
//
// In practice this only bites after a DEFERRAL, because the reacter normally
// acts before the receiver ever gets another turn. Reactor0 allows the deferral
// and deliberately keeps the call across it -- but clears `Game::waiting` while
// doing so, which is why the paired order is recorded on the card itself
// (`ConvData::react_target_order`) rather than read back off the connection.
//
// Replay 1972716, "Odds and Evens & Dark Rainbow (5 Suits)", Alice = will-bot69
// (p1), Bob = will-bot67 (p2), Cathy = yagami_black (p0):
//
//   T1  Cathy clues rank 2 to Bob. Alice is the reacter: anchor 4, her slot 1
//       (order 9) is stamped urgent CTD, and `calc_slot(4, 1, 5) = 3` pairs it
//       with the receiver's slot 3 -- order 12.
//   T2  Alice defers by cluing Cathy. The waiting connection is cleared; the
//       call survives, as it is meant to.
//   T3  Bob plays order 12. The pairing is now spent.
//   T5  Alice discards order 9 anyway.
//
// It cost two cards: order 9 was a playable g3, and at T6 Bob discarded the
// playable y1 that was sitting on his chop. Bob had no safe action and his chop
// was endangered, so either yellow or green to Bob was a legal stable play clue
// and one of them was owed.
//
// Note this is NOT the H4 (critical-chop) rule: Bob's y1 has three copies and is
// merely endangered. H4 is only HIGH, and a pending reaction outranks HIGH --
// the staleness rule is what has to answer here.
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Odds and Evens & Dark Rainbow (5 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1972716, SpentReactionStopsBeingUrgent) {
  // Reconstruct exactly the Game the live bot saw at turn 5.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1972716,
  "debug": {
    "cards_left": 28,
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
              3
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 16,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
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
            "inferred": 32440320,
            "info_lock": null,
            "order": 3,
            "possible": 33521664,
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
              3
            ],
            "inferred": 33521664,
            "info_lock": null,
            "order": 2,
            "possible": 33521664,
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
            "inferred": 32767,
            "info_lock": null,
            "order": 1,
            "possible": 32767,
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
              4
            ],
            "inferred": 32440320,
            "info_lock": null,
            "order": 0,
            "possible": 33521664,
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
            "inferred": 473550,
            "info_lock": null,
            "order": 9,
            "possible": 33554431,
            "slot": 1,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 8,
            "possible": 33554431,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 7,
            "possible": 33554431,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 6,
            "possible": 33554431,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33554431,
            "info_lock": null,
            "order": 5,
            "possible": 33554431,
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
              1,
              1
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 15,
            "possible": 33554431,
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
              4
            ],
            "inferred": 10824010,
            "info_lock": null,
            "order": 14,
            "possible": 10824010,
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
              3
            ],
            "inferred": 22730421,
            "info_lock": null,
            "order": 13,
            "possible": 22730421,
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
            "inferred": 22730421,
            "info_lock": null,
            "order": 11,
            "possible": 22730421,
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
            "inferred": 10824010,
            "info_lock": null,
            "order": 10,
            "possible": 10824010,
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
      0,
      2,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 5,
    "waiting": []
  },
  "game_id": 4788,
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
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 2,
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
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          10,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "giver": 1,
        "kind": "C",
        "list": [
          0,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 6,
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
        "order": 12,
        "p": 2,
        "rank": 1,
        "suit": 2,
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
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 3,
        "suit": 3,
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
        1,
        4
      ],
      [
        4,
        3
      ],
      [
        4,
        2
      ],
      [
        2,
        2
      ],
      null,
      null,
      null,
      null,
      null,
      [
        2,
        4
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
        3,
        3
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
      "variant_name": "Odds and Evens & Dark Rainbow (5 Suits)"
    },
    "our_player_index": 1,
    "reactive_overrides": [
      {
        "clue_value": 1,
        "even": true,
        "kind": "R",
        "reactive_value": 5
      }
    ],
    "rlocks": true,
    "variant": "Odds and Evens & Dark Rainbow (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-25T21:29:48.125",
  "turn": 5
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // --- guards: the position really is the one described above --------------
  const hanabi::State& s = game.state;
  ASSERT_EQ(s.our_player_index, 1);
  ASSERT_TRUE(game.waiting.empty())
      << "guard: the deferral at T2 cleared the waiting connection, which is "
         "why the pairing has to be read off the card";
  ASSERT_TRUE(game.meta[9].urgent) << "guard: order 9 still carries the call";
  ASSERT_EQ(game.meta[9].status, hanabi::CardStatus::CALLED_TO_DISCARD);

  // The pairing itself, and the fact that it is spent. Order 12 was the
  // receiver's slot 3 at clue time; Bob played it at T3.
  ASSERT_EQ(game.meta[9].react_target_order, 12)
      << "guard: slot 1 pairs with receiver slot calc_slot(4, 1, 5) = 3";
  const std::vector<int>& receiver_hand = s.hands[2];
  ASSERT_EQ(std::find(receiver_hand.begin(), receiver_hand.end(), 12),
            receiver_hand.end())
      << "guard: the paired card has left the receiver's hand";

  // Bob's chop is the playable, unclued y1 -- the card actually at stake.
  auto bob_chop = game.chop(2);
  ASSERT_TRUE(bob_chop.has_value());
  auto chop_id = s.deck[*bob_chop].id();
  ASSERT_TRUE(chop_id.has_value());
  EXPECT_TRUE(s.is_playable(*chop_id)) << "guard: Bob's chop is playable";
  EXPECT_FALSE(s.is_critical(*chop_id))
      << "guard: three copies, so this is endangered and NOT the H4 rule";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformColour>(action))
      << "the spent call must not pre-empt the stable play clue Bob is owed; "
         "got " << hanabi::to_json(action, 0).dump();
  const auto& colour = std::get<hanabi::PerformColour>(action);
  EXPECT_EQ(colour.target, 2) << "the clue goes to Bob, whose chop is at risk";
  EXPECT_EQ(colour.value, 1)
      << "yellow: it touches two useful cards to green's one, so it wins the "
         "default tiebreak on rung 3.bob_chop";
}
