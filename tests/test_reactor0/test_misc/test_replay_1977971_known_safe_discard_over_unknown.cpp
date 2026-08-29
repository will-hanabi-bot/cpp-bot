// The endgame must not burn a card it cannot prove is worthless while one it
// CAN sits in the same hand. Replay 1977971 T22 (reactor0).
//
// Stacks r3 b4 l4, 0 clues, 3 cards left -- deep enough that the endgame fork
// owns the turn (`rem_score` 4 <= `num_suits` + 1) and the solver runs
// (`pace` 2 <= 3). So the Actionable Card Priority never gets a say, and the
// solver reasons from COMMON-KNOWLEDGE empathy, which ranks the two burn
// candidates backwards:
//
//   slot 5, order 19: {b1, b5}                 -- looks like a coin-flip on b5
//   slot 3, order 23: {r1, r5, b1, b3, b5}     -- looks like the safer burn
//
// Private sight inverts it. The last b5 is face-up in yagami_black's hand, so
// order 19 is PROVABLY the b1 -- worthless. Nothing accounts for the r5, so
// order 23 might be the r5 the team still needs, worth two points.
//
// v11.6.0's `prefer_known_discard` (`src/basics/decide.cpp`) swaps the target.
// Before it, this turn discarded order 23.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Alternating Clues & Light Pink (3 Suits). 3 players, our_player_index=0.

TEST(MiscReplay1977971, KnownSafeDiscardBeatsAnUnknownOne) {
  // Reconstruct exactly the Game the live bot saw at turn 22.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1977971,
  "debug": {
    "cards_left": 3,
    "clue_tokens": 0,
    "current_player_index": 0,
    "discards": [
      {
        "order": 16,
        "rank": 2,
        "suit": 2
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": true,
            "id": null,
            "inferred": 16392,
            "info_lock": 16392,
            "order": 26,
            "possible": 29960,
            "slot": 1,
            "status": "CALLED_TO_PLAY",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 29960,
            "info_lock": null,
            "order": 25,
            "possible": 29960,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 689,
            "info_lock": null,
            "order": 23,
            "possible": 693,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 17,
            "info_lock": null,
            "order": 21,
            "possible": 21,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 544,
            "info_lock": null,
            "order": 19,
            "possible": 544,
            "slot": 5,
            "status": "NONE",
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
              0,
              1
            ],
            "inferred": 891,
            "info_lock": null,
            "order": 17,
            "possible": 891,
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
            "inferred": 28804,
            "info_lock": null,
            "order": 15,
            "possible": 29828,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": true,
            "id": [
              1,
              5
            ],
            "inferred": 520,
            "info_lock": null,
            "order": 8,
            "possible": 891,
            "slot": 3,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 891,
            "info_lock": null,
            "order": 6,
            "possible": 891,
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
            "inferred": 891,
            "info_lock": null,
            "order": 5,
            "possible": 891,
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
              1,
              2
            ],
            "inferred": 30688,
            "info_lock": null,
            "order": 24,
            "possible": 30688,
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
              5
            ],
            "inferred": 30688,
            "info_lock": null,
            "order": 22,
            "possible": 30688,
            "slot": 2,
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
            "inferred": 30688,
            "info_lock": null,
            "order": 20,
            "possible": 30688,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              0,
              4
            ],
            "inferred": 8,
            "info_lock": 8,
            "order": 14,
            "possible": 27,
            "slot": 4,
            "status": "CALLED_TO_PLAY",
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
            "inferred": 864,
            "info_lock": null,
            "order": 10,
            "possible": 864,
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
        "v": "Play"
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
        "v": "Play"
      },
      {
        "k": "clue",
        "v": "Play"
      }
    ],
    "play_stacks": [
      3,
      4,
      4
    ],
    "strikes": 0,
    "turn_count": 22,
    "waiting": []
  },
  "game_id": 4532,
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
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 1,
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
        "rank": 2,
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
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 3
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
        "order": 9,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 0,
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
        "giver": 2,
        "kind": "C",
        "list": [
          0,
          2,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 6,
        "max": 15,
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
        "rank": 1,
        "suit": 1,
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
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 1,
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
        "order": 13,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 0,
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
        "failed": false,
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 7,
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
        "giver": 1,
        "kind": "R",
        "list": [
          0,
          1,
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 6,
        "max": 15,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 8,
        "t": "turn"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 21,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 6,
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
        "giver": 1,
        "kind": "C",
        "list": [
          0,
          2,
          19
        ],
        "t": "clue",
        "target": 0,
        "value": 1
      },
      {
        "clues": 5,
        "max": 15,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 15,
        "score": 7,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "order": 0,
        "p": 0,
        "rank": 3,
        "suit": 1,
        "t": "play"
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
        "kind": "R",
        "list": [
          4
        ],
        "t": "clue",
        "target": 0,
        "value": 2
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
        "order": 18,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 24,
        "p": 2,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 9,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
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
        "score": 10,
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
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
        "max": 15,
        "score": 10,
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
          15
        ],
        "t": "clue",
        "target": 1,
        "value": 3
      },
      {
        "clues": 2,
        "max": 15,
        "score": 10,
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
        "rank": 4,
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
        "clues": 2,
        "max": 15,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 19,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          14
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 1,
        "max": 15,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 20,
        "t": "turn"
      },
      {
        "giver": 2,
        "kind": "R",
        "list": [
          25,
          26
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 0,
        "max": 15,
        "score": 11,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 21,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        1,
        3
      ],
      [
        2,
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
        4
      ],
      [
        1,
        1
      ],
      [
        0,
        2
      ],
      [
        2,
        1
      ],
      [
        1,
        5
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
        1,
        2
      ],
      [
        2,
        3
      ],
      [
        0,
        2
      ],
      [
        0,
        4
      ],
      [
        0,
        3
      ],
      [
        2,
        2
      ],
      [
        0,
        1
      ],
      [
        0,
        3
      ],
      null,
      [
        2,
        4
      ],
      null,
      [
        2,
        5
      ],
      null,
      [
        1,
        2
      ],
      null,
      null
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
      "variant_name": "Alternating Clues & Light Pink (3 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Alternating Clues & Light Pink (3 Suits)",
    "zcs_turn": 21
  },
  "ts": "2026-08-29T22:50:33.471",
  "turn": 22
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  // Guard the premise: the endgame fork owns this turn, so the failure cannot
  // be blamed on -- or fixed in -- the convention ladder.
  ASSERT_EQ(game.state.clue_tokens, 0);
  ASSERT_EQ(game.state.cards_left, 3);

  // Guard the asymmetry the empathy view misses.
  EXPECT_TRUE(hanabi::reactor0::provably_trash(game, 19))
      << "the last b5 is visible in a partner's hand, so order 19 can only be "
         "the b1";
  EXPECT_FALSE(hanabi::reactor0::provably_trash(game, 23))
      << "nothing accounts for the r5, so order 23 is not provably worthless";

  // The convention ladder never faces this choice at all: it pitches the CTP in
  // slot 1 (order 26, {r4, l5}, playable either way) and never reaches a burn.
  // The decision to spend the turn burning a card is the ENDGAME's, which is
  // why the fix belongs at the fork and not in Actionable Card Priority.
  {
    auto ladder = hanabi::reactor0::choose_action(game);
    ASSERT_TRUE(ladder.has_value());
    auto* p = std::get_if<hanabi::PerformPlay>(&*ladder);
    auto* d = std::get_if<hanabi::PerformDiscard>(&*ladder);
    EXPECT_EQ(p ? p->target : -1, 26)
        << "expected the ladder to pitch the CTP; it played "
        << (p ? p->target : -1) << " / discarded " << (d ? d->target : -1);
  }

  hanabi::PerformAction action = game.take_action();
  auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  ASSERT_NE(discard, nullptr) << "expected a discard on a 0-clue burn turn";
  EXPECT_EQ(discard->target, 19)
      << "must burn the card it can PROVE is trash (order 19), not the unknown "
         "slot 3 (order 23) that might be the needed r5";
}
