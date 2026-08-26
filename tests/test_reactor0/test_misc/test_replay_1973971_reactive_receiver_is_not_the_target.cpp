// Under target parity the RECEIVER is not the clued seat.
//
// Alternating Clues and Synesthesia have no stable clues: Bob is always the
// reacter and Cathy always the receiver, so a clue given TO Bob touches the
// reacter's own hand while still identifying a slot in Cathy's. That makes
// `action.target` and the receiver two different players -- the only place in
// the convention where they come apart.
//
// v10.0.0 threaded the receiver into `interpret_reactive` but not into the two
// branch functions, which each re-derived `int receiver = action.target;`. The
// branch therefore walked the REACTER's hand; from his own seat every deck id
// there is nullopt, so the play pool and the dc-target list came back empty and
// the clue read as a MISTAKE.
//
// Replay 1973971 T15, "Alternating Clues & Brown (5 Suits)". will-bot67 is p0,
// yagami_black p1, will-bot69 p2. On T14 yagami clued rank 5 to will-bot69, so
// Alice = yagami, Bob = will-bot69, Cathy = will-bot67, and the clue is ODD --
// exactly one play.
//
// Rank 5's anchor is 5 and `react_slot + target_slot = 5 (mod 5)`, so Bob
// playing slot 3 designates Cathy's slot 2 -- a b2 with blue on 3, her leftmost
// trash. will-bot69 discarded its chop instead.
#include <gtest/gtest.h>

#include <algorithm>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Alternating Clues & Brown (5 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1973971, ReactiveReceiverIsNotTheCluedSeat) {
  // Reconstruct exactly the Game the live bot saw at turn 15.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1973971,
  "debug": {
    "cards_left": 27,
    "clue_tokens": 4,
    "current_player_index": 2,
    "discards": [
      {
        "order": 19,
        "rank": 4,
        "suit": 0
      },
      {
        "order": 17,
        "rank": 4,
        "suit": 1
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
              5
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 22,
            "possible": 33554431,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 18,
            "possible": 33554431,
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
              5
            ],
            "inferred": 540688,
            "info_lock": null,
            "order": 4,
            "possible": 540688,
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
              2
            ],
            "inferred": 31930382,
            "info_lock": null,
            "order": 1,
            "possible": 33012751,
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
              3
            ],
            "inferred": 31930382,
            "info_lock": null,
            "order": 0,
            "possible": 33012751,
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
              4,
              5
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 20,
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
              3,
              5
            ],
            "inferred": 1015808,
            "info_lock": null,
            "order": 8,
            "possible": 1015808,
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
              4
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 7,
            "possible": 31744,
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
              3
            ],
            "inferred": 32506879,
            "info_lock": null,
            "order": 6,
            "possible": 32506879,
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
              1
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 5,
            "possible": 31744,
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
            "inferred": 33013231,
            "info_lock": null,
            "order": 21,
            "possible": 33013231,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33013231,
            "info_lock": null,
            "order": 14,
            "possible": 33013231,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33013231,
            "info_lock": null,
            "order": 13,
            "possible": 33013231,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 541200,
            "info_lock": null,
            "order": 11,
            "possible": 541200,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 33013231,
            "info_lock": null,
            "order": 10,
            "possible": 33013231,
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
        "v": "Mistake"
      }
    ],
    "play_stacks": [
      0,
      1,
      0,
      3,
      2
    ],
    "strikes": 0,
    "turn_count": 15,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_value": 5,
        "focus_slot": 5,
        "giver": 1,
        "inverted": false,
        "react_order": -1,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          22,
          18,
          4,
          1,
          0
        ],
        "rlocks": false,
        "turn": 14
      }
    ]
  },
  "game_id": 20,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 0,
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
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 1,
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
        "rank": 5,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
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
        "kind": "C",
        "list": [
          5,
          7
        ],
        "t": "clue",
        "target": 1,
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
        "kind": "R",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 5
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
        "suit": 3,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": -1,
        "suit": -1,
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
        "order": 3,
        "p": 0,
        "rank": 2,
        "suit": 3,
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
        "order": 15,
        "p": 2,
        "rank": 1,
        "suit": 4,
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
        "order": 16,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 2,
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
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 5
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
        "giver": 2,
        "kind": "C",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 3,
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
        "order": 2,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
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
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 4,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 3,
        "max": 25,
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
        "order": 17,
        "p": 2,
        "rank": 4,
        "suit": 1,
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
        "order": 19,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 5,
        "suit": 2,
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
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 5
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
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        0,
        3
      ],
      [
        0,
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
        0,
        5
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
        3,
        5
      ],
      [
        4,
        2
      ],
      null,
      null,
      [
        3,
        1
      ],
      null,
      null,
      [
        4,
        1
      ],
      [
        1,
        1
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
        0,
        4
      ],
      [
        4,
        5
      ],
      null,
      [
        2,
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
      "variant_name": "Alternating Clues & Brown (5 Suits)"
    },
    "our_player_index": 2,
    "rlocks": false,
    "variant": "Alternating Clues & Brown (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T18:30:51.337",
  "turn": 15
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards: the position is the one described above --------------------
  ASSERT_TRUE(hanabi::reactor::variants::uses_target_parity(*s.variant))
      << "guard: Alternating Clues is a target-parity variant";
  ASSERT_EQ(s.our_player_index, 2) << "we are will-bot69, the reacter";

  // The clue must have been UNDERSTOOD. Before the fix this was a MISTAKE,
  // which is the whole failure -- everything below follows from it.
  auto move = game.last_move();
  ASSERT_TRUE(move && std::holds_alternative<hanabi::ClueInterp>(*move));
  EXPECT_NE(std::get<hanabi::ClueInterp>(*move), hanabi::ClueInterp::MISTAKE)
      << "the rank-5 clue to us is a reactive discard clue, not an "
         "uninterpretable one";

  // The waiting connection names the two seats separately: we react, and the
  // seat we are NOT is the receiver.
  ASSERT_FALSE(game.waiting.empty()) << "a reactive clue leaves a connection";
  const hanabi::ReactorWC& wc = game.waiting.front();
  EXPECT_EQ(wc.reacter, 2) << "will-bot69 reacts";
  EXPECT_EQ(wc.receiver, 0)
      << "will-bot67 receives -- NOT the clued seat, which was us";
  EXPECT_EQ(wc.focus_slot, 5) << "rank 5's anchor";

  // Cathy's slot 2 is the designated card, and it really is her leftmost trash.
  const int cathy_slot2 = s.hands[0][1];
  auto cathy_id = s.deck[cathy_slot2].id();
  ASSERT_TRUE(cathy_id.has_value());
  EXPECT_TRUE(s.is_basic_trash(*cathy_id))
      << "guard: blue is on 3, so the b2 in her slot 2 is trash";
  auto cathy_slot1 = s.deck[s.hands[0][0]].id();
  ASSERT_TRUE(cathy_slot1.has_value());
  EXPECT_FALSE(s.is_basic_trash(*cathy_slot1))
      << "guard: and her slot 1 is not, so slot 2 is the LEFTMOST trash";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "the reaction is a play; got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 13)
      << "slot 3 -- react_slot 3 + target_slot 2 = the anchor of 5, which "
         "points at will-bot67's slot 2";
}
