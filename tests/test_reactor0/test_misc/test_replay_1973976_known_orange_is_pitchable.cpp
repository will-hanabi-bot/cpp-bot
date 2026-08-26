// Pressing PLAY on a known orange is a PITCH, and both the vet and the stamp
// have to know it.
//
// On an inverted suit the Play button DISCARDS. So when every reading of the
// reacter's card is orange, the call is not a blind play at all: it cannot
// strike, it need not be playable, and the only question is whether the card
// can be spared.
//
// Two separate sites got that wrong, and BOTH had to change -- reverting either
// one alone puts this replay back:
//
//   * the vet asked `can_pitch_for_free`, which demands EVERY reading be a dead
//     orange. Here the last o2 was already accounted for, so the card's
//     effective empathy was {o1, o3, o4} -- nothing playable and nothing a
//     connector -- and the target was retargeted away.
//   * the stamp called `reactor::target_play`, which narrows `inferred` to the
//     playable set and bails when that empties. {o3, o4} has nothing playable,
//     so it refused to stamp and the target was skipped again.
//
// Replay 1973976 T12, "Null-Fives & Orange (4 Suits)", stacks [0,1,1,1] so
// {r1, g2, b2, o2} play. will-bot67 is p0, yagami p1, will-bot69 p2. On T11
// yagami clued RED to will-bot67, so Alice = yagami, Bob = will-bot69,
// Cathy = will-bot67.
//
// Red's anchor is 1 and Cathy's slot 3 is a playable o2, so
// `calc_slot(1, 3, 5) = 3`: will-bot69 pitches his slot 3, Cathy chucks the o2
// onto the stack. Instead the reading fell through to Cathy's slot 5 (an r1),
// stamped will-bot69's slot 1 CTD, and he discarded it.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Null-Fives & Orange (4 Suits). 3 players, our_player_index=2.

TEST(MiscReplay1973976, AKnownOrangeIsPitchable) {
  // Reconstruct exactly the Game the live bot saw at turn 12.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1973976,
  "debug": {
    "cards_left": 19,
    "clue_tokens": 6,
    "current_player_index": 2,
    "discards": [
      {
        "order": 4,
        "rank": 1,
        "suit": 1
      },
      {
        "order": 12,
        "rank": 2,
        "suit": 1
      },
      {
        "order": 6,
        "rank": 2,
        "suit": 3
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": [
              0,
              4
            ],
            "inferred": 15,
            "info_lock": null,
            "order": 20,
            "possible": 15,
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
              3
            ],
            "inferred": 1048560,
            "info_lock": null,
            "order": 3,
            "possible": 1048560,
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
              2
            ],
            "inferred": 1048560,
            "info_lock": null,
            "order": 2,
            "possible": 1048560,
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
              1
            ],
            "inferred": 1048560,
            "info_lock": null,
            "order": 1,
            "possible": 1048560,
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
              1
            ],
            "inferred": 15,
            "info_lock": null,
            "order": 0,
            "possible": 15,
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
              1,
              4
            ],
            "inferred": 557055,
            "info_lock": null,
            "order": 17,
            "possible": 557055,
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
            "inferred": 557055,
            "info_lock": null,
            "order": 15,
            "possible": 557055,
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
            "inferred": 491520,
            "info_lock": null,
            "order": 9,
            "possible": 491520,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 557055,
            "info_lock": null,
            "order": 8,
            "possible": 557055,
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
              4
            ],
            "inferred": 557055,
            "info_lock": null,
            "order": 7,
            "possible": 557055,
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
            "focused": true,
            "id": null,
            "inferred": 79278,
            "info_lock": 65536,
            "order": 19,
            "possible": 1048575,
            "slot": 1,
            "status": "CALLED_TO_DISCARD",
            "trash": false,
            "urgent": true
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 934430,
            "info_lock": null,
            "order": 16,
            "possible": 1032735,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 393216,
            "info_lock": null,
            "order": 14,
            "possible": 491520,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 425984,
            "info_lock": null,
            "order": 11,
            "possible": 491520,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 14336,
            "info_lock": null,
            "order": 10,
            "possible": 15360,
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
      5
    ],
    "move_history": [
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
        "v": "Discard"
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
      }
    ],
    "play_stacks": [
      0,
      1,
      1,
      1
    ],
    "strikes": 0,
    "turn_count": 12,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_value": 0,
        "focus_slot": 1,
        "giver": 1,
        "inverted": false,
        "react_order": 19,
        "reacter": 2,
        "receiver": 0,
        "receiver_hand": [
          20,
          3,
          2,
          1,
          0
        ],
        "rlocks": false,
        "turn": 11
      }
    ]
  },
  "game_id": 31,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 4,
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
          11,
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 7,
        "max": 20,
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
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
        "suit": 1,
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
        "max": 20,
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
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 6,
        "max": 20,
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
        "rank": 2,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 20,
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
        "kind": "C",
        "list": [
          9
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 7,
        "max": 20,
        "score": 2,
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
          10,
          18
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 6,
        "max": 20,
        "score": 2,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 1,
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
        "clues": 6,
        "max": 20,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
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
          0,
          20
        ],
        "t": "clue",
        "target": 0,
        "value": 0
      },
      {
        "clues": 6,
        "max": 20,
        "score": 3,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        0,
        1
      ],
      [
        2,
        1
      ],
      [
        3,
        2
      ],
      [
        3,
        3
      ],
      [
        1,
        1
      ],
      [
        3,
        1
      ],
      [
        3,
        2
      ],
      [
        2,
        4
      ],
      [
        2,
        5
      ],
      [
        3,
        4
      ],
      null,
      null,
      [
        1,
        2
      ],
      [
        1,
        1
      ],
      null,
      [
        0,
        5
      ],
      null,
      [
        1,
        4
      ],
      [
        2,
        1
      ],
      null,
      [
        0,
        4
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
      "variant_name": "Null-Fives & Orange (4 Suits)"
    },
    "our_player_index": 2,
    "rlocks": false,
    "variant": "Null-Fives & Orange (4 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T18:37:00.738",
  "turn": 12
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards -------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 2) << "we are will-bot69, the reacter";
  const int our_slot3 = s.hands[2][2];
  ASSERT_EQ(our_slot3, 14);

  // Our slot 3 is a KNOWN orange, which is what makes the call a pitch...
  const hanabi::IdentitySet poss = game.common.thoughts[our_slot3].possible;
  ASSERT_TRUE(poss.non_empty());
  EXPECT_TRUE(poss.forall([&s](hanabi::Identity i) {
    return hanabi::reactor::variants::is_inverted_id(s, i);
  })) << "guard: every reading of our slot 3 is orange";

  // ...and nothing it could be is PLAYABLE, which is what `target_play` chokes
  // on. This is the condition that made the stamp refuse.
  EXPECT_FALSE(game.common.thoughts[our_slot3].inferred.exists(
      [&s](hanabi::Identity i) { return s.is_playable(i); }))
      << "guard: no reading in `inferred` is playable, so a play stamp cannot "
         "narrow to anything";

  // Cathy's slot 3 is the playable o2 the pitch is paired with.
  const int cathy_slot3 = s.hands[0][2];
  auto cathy_id = s.deck[cathy_slot3].id();
  ASSERT_TRUE(cathy_id.has_value());
  EXPECT_TRUE(s.is_playable(*cathy_id)) << "guard: her slot 3 is playable";
  EXPECT_TRUE(hanabi::reactor::variants::is_inverted_id(s, *cathy_id))
      << "guard: and it is the orange, so she chucks it";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  ASSERT_TRUE(std::holds_alternative<hanabi::PerformPlay>(action))
      << "the pitch presses Play; got " << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(std::get<hanabi::PerformPlay>(action).target, our_slot3)
      << "slot 3 -- anchor 1 pairs it with Cathy's slot 3, the playable o2";
}
