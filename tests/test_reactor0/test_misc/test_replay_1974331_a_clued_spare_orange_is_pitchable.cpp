// A CLUED slot with an inverted reading it can spare is pitchable, even when
// other readings are plain.
//
// Replay 1974331 T8, "Deceptive-Ones & Orange (5 Suits)", stacks r1 y0 g1 b0 o1.
// yagami clues rank 2 to will-bot67: reactive, anchor 2, even bucket, us as
// reacter. Phase A walks his playables leftmost-first -- slot 3 (b1), then slot
// 5 (y1) -- and the first pairs calc_slot(2,3,5) = 4 onto OUR slot 4.
//
// Our slot 4 is order 8, clued to {r3,y1,y3,g3,b3,o3}, and it is the o3: two
// copies, none discarded, so throwing it costs nothing. Pressing Play on an
// orange pitches it, which is the whole call.
//
// The bot skipped it. `target_play` narrows INFERRED = {r3,y3,g3,b3,o3}, none of
// which can play (the playables are r2/y1/g2/b1/o2), and no connector existed at
// clue time -- so it refused to stamp and Phase A walked to the second
// candidate: react slot 2, order 15, a b2 with blue on 0. That play STRUCK.
//
// Both pitch arms wanted EVERY reading inverted -- `react_slot_is_a_pitch` in
// the vet, `can_pitch_for_free` in the stamp -- and this card is a mixed
// empathy. v10.8.0 adds the third step of `stamp_react_play_button`: for a card
// the team has CLUED or stamped, a spare inverted reading licenses a pitch once
// the play reading has been ruled out.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Deceptive-Ones & Orange (5 Suits). 3 players, our_player_index=1.

TEST(MiscReplay1974331, ACluedSpareOrangeIsPitchable) {
  // Reconstruct exactly the Game the live bot saw at turn 8.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1974331,
  "debug": {
    "cards_left": 31,
    "clue_tokens": 6,
    "current_player_index": 1,
    "discards": [
      {
        "order": 14,
        "rank": 1,
        "suit": 2
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
              1,
              4
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
            "clued": false,
            "focused": false,
            "id": [
              0,
              5
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 4,
            "possible": 33554431,
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
              4
            ],
            "inferred": 33554431,
            "info_lock": null,
            "order": 3,
            "possible": 33554431,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 1,
            "possible": 33554431,
            "slot": 4,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 0,
            "possible": 33554431,
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
            "inferred": 33554431,
            "info_lock": null,
            "order": 17,
            "possible": 33554431,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": null,
            "inferred": 9439234,
            "info_lock": 2050,
            "order": 15,
            "possible": 29224795,
            "slot": 2,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 18434642,
            "info_lock": null,
            "order": 9,
            "possible": 20564563,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 4329604,
            "info_lock": null,
            "order": 8,
            "possible": 4329636,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 18434642,
            "info_lock": null,
            "order": 7,
            "possible": 20564563,
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
            "inferred": 30341052,
            "info_lock": null,
            "order": 18,
            "possible": 30341052,
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
            "inferred": 30341052,
            "info_lock": null,
            "order": 13,
            "possible": 30341052,
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
              1
            ],
            "inferred": 30341052,
            "info_lock": null,
            "order": 12,
            "possible": 30341052,
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
              2
            ],
            "inferred": 3213379,
            "info_lock": null,
            "order": 11,
            "possible": 3213379,
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
              1
            ],
            "inferred": 30341052,
            "info_lock": null,
            "order": 10,
            "possible": 30341052,
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
        "v": "Discard"
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
      0,
      1,
      0,
      1
    ],
    "strikes": 0,
    "turn_count": 8,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "R",
        "clue_value": 2,
        "focus_slot": 2,
        "giver": 0,
        "inverted": false,
        "react_order": 15,
        "reacter": 1,
        "receiver": 2,
        "receiver_hand": [
          18,
          13,
          12,
          11,
          10
        ],
        "rlocks": false,
        "turn": 7
      }
    ]
  },
  "game_id": 225,
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
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 4,
        "suit": 1,
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
        "suit": 1,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
        "failed": false,
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "discard"
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 3
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
        "order": 2,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 4,
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 4,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "giver": 0,
        "kind": "R",
        "list": [
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 6,
        "max": 25,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 7,
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
        2
      ],
      [
        0,
        1
      ],
      [
        1,
        4
      ],
      [
        0,
        5
      ],
      [
        4,
        1
      ],
      [
        2,
        1
      ],
      null,
      null,
      null,
      [
        1,
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
        3,
        4
      ],
      [
        2,
        1
      ],
      null,
      [
        1,
        4
      ],
      null,
      [
        4,
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
      "variant_name": "Deceptive-Ones & Orange (5 Suits)"
    },
    "our_player_index": 1,
    "rlocks": false,
    "variant": "Deceptive-Ones & Orange (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-27T00:28:15.949",
  "turn": 8
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards ---------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 1) << "we are will-bot69, the reacter";
  ASSERT_EQ(s.hands[1][3], 8) << "order 8 is our slot 4";
  ASSERT_EQ(s.hands[1][1], 15) << "order 15 is our slot 2";
  ASSERT_TRUE(s.deck[8].clued) << "the clause that licenses the pitch: it is CLUED";

  const hanabi::Thought& t = game.common.thoughts[8];
  EXPECT_FALSE(t.possible.forall([&](hanabi::Identity i) {
    return hanabi::reactor::variants::is_inverted_id(s, i);
  })) << "guard: NOT every reading is inverted -- that is why the old gate "
         "refused, and why this is not the v10.3.0 case";
  // `possible` is the pre-clue set the stamp does not touch, so it is the one
  // to assert on -- `inferred` here has already been narrowed by the pitch call
  // this test is about.
  EXPECT_TRUE(t.possible.exists([&](hanabi::Identity i) {
    return s.is_playable(i);
  })) << "guard: `possible` DOES hold a playable (the y1 Deceptive-Ones let the "
         "rank-3 clue touch), which is why the vet passed -- and why step 3 "
         "must ask `slot_has_spare_inverted` rather than `slot_is_pitchable`, "
         "since the wider predicate would say yes for this y1 and not for the "
         "orange";
  EXPECT_TRUE(t.inferred.contains(hanabi::Identity{4, 3}))
      << "and the call that lands narrows to the o3 -- the reading is 'pitch "
         "the orange', not 'play something'";
  EXPECT_EQ(game.meta[8].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "a pitch is issued on the Play button";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, 8)
      << "pitch the spare orange 3 on slot 4";
  EXPECT_NE(std::get<hanabi::PerformPlay>(action).target, 15)
      << "order 15 is a b2 on an empty blue stack -- this play struck";
}
