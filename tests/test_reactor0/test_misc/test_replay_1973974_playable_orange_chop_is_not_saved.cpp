// A playable orange on chop is not a card that needs saving.
//
// On an inverted suit Discard is a CHUCK: it puts the card on its own stack. So
// a playable orange sitting on a partner's chop is the one card he SHOULD be
// throwing -- it scores itself. It belongs with basic trash and a same-hand
// dupe in every predicate that asks whether a chop is worth a clue.
//
// All three had to learn it, because §3 fires on either of the first two:
//   at_risk_chop        -> false, like trash
//   has_playable_chop   -> false, like a same-hand dupe
//   chop_is_expendable  -> true,  like trash
//
// Replay 1973974 T10, "Null-Fives & Orange (4 Suits)", stacks [0,0,2,1] so
// {r1, g1, b3, o2} play. Alice = will-bot67 (us), Bob = will-bot69, whose chop
// is his slot 1 -- an o2. will-bot67 spent a clue LOCKING him over it:
// `rung "3.bob_chop", shape "stable_lock"`.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Null-Fives & Orange (4 Suits). 3 players, our_player_index=0.

TEST(MiscReplay1973974, APlayableOrangeChopIsNotSaved) {
  // Reconstruct exactly the Game the live bot saw at turn 10.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1973974,
  "debug": {
    "cards_left": 20,
    "clue_tokens": 6,
    "current_player_index": 0,
    "discards": [
      {
        "order": 4,
        "rank": 2,
        "suit": 2
      },
      {
        "order": 5,
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
            "id": null,
            "inferred": 1033215,
            "info_lock": null,
            "order": 18,
            "possible": 1033215,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1033215,
            "info_lock": null,
            "order": 16,
            "possible": 1033215,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1033215,
            "info_lock": null,
            "order": 3,
            "possible": 1033215,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 4096,
            "info_lock": 4096,
            "order": 2,
            "possible": 13312,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1033215,
            "info_lock": null,
            "order": 1,
            "possible": 1033215,
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
              3,
              2
            ],
            "inferred": 1046527,
            "info_lock": null,
            "order": 19,
            "possible": 1046527,
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
              3
            ],
            "inferred": 1000414,
            "info_lock": null,
            "order": 17,
            "possible": 1033215,
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
              3
            ],
            "inferred": 1000414,
            "info_lock": null,
            "order": 9,
            "possible": 1000414,
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
              2
            ],
            "inferred": 1000414,
            "info_lock": null,
            "order": 8,
            "possible": 1000414,
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
              3
            ],
            "inferred": 1000414,
            "info_lock": null,
            "order": 6,
            "possible": 1000414,
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
              2,
              1
            ],
            "inferred": 1046527,
            "info_lock": null,
            "order": 15,
            "possible": 1046527,
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
              5
            ],
            "inferred": 1033215,
            "info_lock": null,
            "order": 14,
            "possible": 1033215,
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
            "inferred": 1033215,
            "info_lock": null,
            "order": 13,
            "possible": 1033215,
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
            "inferred": 1033215,
            "info_lock": null,
            "order": 11,
            "possible": 1033215,
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
              4
            ],
            "inferred": 1033215,
            "info_lock": null,
            "order": 10,
            "possible": 1033215,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_black",
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
        "v": "Play"
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
        "v": "Play"
      }
    ],
    "play_stacks": [
      0,
      0,
      2,
      1
    ],
    "strikes": 0,
    "turn_count": 10,
    "waiting": []
  },
  "game_id": 28,
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
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 0,
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
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 5,
        "suit": 1,
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
        "giver": 1,
        "kind": "C",
        "list": [
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 6,
        "max": 20,
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
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 20,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 4,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 16,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 4,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 8,
        "max": 20,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "C",
        "list": [
          7
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 7,
        "max": 20,
        "score": 1,
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
        "suit": 3,
        "t": "discard"
      },
      {
        "order": 18,
        "p": 0,
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
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 20,
        "score": 3,
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
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        3,
        1
      ],
      null,
      null,
      null,
      [
        2,
        2
      ],
      [
        3,
        1
      ],
      [
        3,
        3
      ],
      [
        2,
        2
      ],
      [
        1,
        2
      ],
      [
        0,
        3
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
        2,
        1
      ],
      [
        3,
        1
      ],
      [
        1,
        5
      ],
      [
        2,
        1
      ],
      null,
      [
        1,
        3
      ],
      null,
      [
        3,
        2
      ]
    ],
    "names": [
      "will-bot67",
      "will-bot69",
      "yagami_black"
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
    "our_player_index": 0,
    "rlocks": false,
    "variant": "Null-Fives & Orange (4 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-26T18:35:43.631",
  "turn": 10
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;

  // --- guards -------------------------------------------------------------
  ASSERT_EQ(s.our_player_index, 0) << "we are will-bot67, the giver";
  const int bob = s.next_player_index(0);
  ASSERT_EQ(bob, 1) << "will-bot69";

  auto bob_chop = game.chop(bob);
  ASSERT_TRUE(bob_chop.has_value()) << "guard: Bob has a chop";
  auto chop_id = s.deck[*bob_chop].id();
  ASSERT_TRUE(chop_id.has_value());
  EXPECT_TRUE(hanabi::reactor::variants::is_inverted_id(s, *chop_id))
      << "guard: Bob's chop is an orange";
  EXPECT_TRUE(s.is_playable(*chop_id))
      << "guard: and it is playable, so discarding it CHUCKS it for a point";

  // The three predicates that decide whether a clue is owed. All three must now
  // read the chop as costing nothing -- and §3 fires on either of the first two,
  // so both have to be false or the lock comes back.
  EXPECT_FALSE(hanabi::reactor0::at_risk_chop(game, 0, bob))
      << "a card that chucks itself is not endangered";
  EXPECT_FALSE(hanabi::reactor0::has_playable_chop(game, bob))
      << "and it is not a play the team must arrange";
  EXPECT_TRUE(hanabi::reactor0::chop_is_expendable(game, bob))
      << "it is expendable, like trash";

  // --- the regression -------------------------------------------------------
  hanabi::PerformAction action = game.take_action();

  if (const auto* rank = std::get_if<hanabi::PerformRank>(&action)) {
    EXPECT_FALSE(rank->target == bob && rank->value == 3)
        << "the rank-3 lock to Bob was given over a card he was about to chuck "
           "for a point";
  }
}
