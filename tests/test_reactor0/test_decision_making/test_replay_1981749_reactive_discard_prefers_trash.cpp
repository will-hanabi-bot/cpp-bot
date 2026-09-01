// Replay 1981749 T17 -- a reactive discard must spend Bob's TRASH, not his
// last critical card. Captured from the per-game JSONL log.
//
// Synesthesia & Black (6 Suits), score 8 of 30, so *target parity* binds: every
// clue to Bob is REACTIVE (he discards, Cathy plays) and there are no stable
// clues at all. Alice (will-bot69) is locked -- all five cards CHOP_MOVED -- so
// section 4 opens and must return a clue.
//
// Cathy's only playable is her slot 3 g5, so every candidate names target slot 3
// and the anchor alone decides which card of Bob's he throws:
//
//   Red    anchor 1 -> slot 3, r5   critical
//   Yellow anchor 2 -> slot 4, y3   critical: the other copy went at T15
//   Green  anchor 3 -> slot 5, r3   both copies live
//   Blue   anchor 4 -> slot 1, y4   both copies live
//   Purple anchor 5 -> slot 2, b3   BASIC TRASH -- blue is already on 3
//
// The bot gave yellow and yagami_black threw the last y3, capping yellow at 2.
// Three separate defects had to line up (all v13.4.0):
//
//   * `is_stable_to_bob` tested the SEAT, not stability, so rung 4.4's fill-in
//     accepted a reactive discard and fired above 4.8, the rung that refuses to
//     make Bob throw a critical card;
//   * red read as shape OTHER with no reacter side -- an undecodable reactive --
//     and rung 4.5 took it as a harmless "safe stall" even though Bob would
//     still have reacted, by throwing his r5;
//   * the ditch tiebreak ranked by missing connectors alone, and basic trash
//     scores ZERO there, so b3 sorted BELOW r3 and y4.
//
// With all three fixed, 4.4 and 4.5 decline, red is dropped, and 4.8 ranks the
// three affordable candidates by the shared ditch rule -- trash first -- and
// gives purple.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Synesthesia & Black (6 Suits). 3 players, our_player_index=1.

TEST(DecisionMaking1981749, T17ReactiveDiscardSpendsTrashNotTheLastY3) {
  // Reconstruct exactly the Game the live bot saw at turn 17.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 1,
  "database_id": 1981749,
  "debug": {
    "cards_left": 30,
    "clue_tokens": 4,
    "current_player_index": 1,
    "discards": [
      {
        "order": 10,
        "rank": 3,
        "suit": 1
      },
      {
        "order": 17,
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
              5,
              3
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 24,
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
              1,
              2
            ],
            "inferred": 935166843,
            "info_lock": null,
            "order": 20,
            "possible": 935166843,
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
            "inferred": 4210688,
            "info_lock": null,
            "order": 18,
            "possible": 4210688,
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
              5
            ],
            "inferred": 565707280,
            "info_lock": null,
            "order": 1,
            "possible": 565707280,
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
              2
            ],
            "inferred": 565707280,
            "info_lock": null,
            "order": 0,
            "possible": 565707280,
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
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 21,
            "possible": 1073741823,
            "slot": 1,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1038640094,
            "info_lock": null,
            "order": 19,
            "possible": 1073741823,
            "slot": 2,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 33822,
            "info_lock": null,
            "order": 15,
            "possible": 34636863,
            "slot": 3,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1038838720,
            "info_lock": null,
            "order": 8,
            "possible": 1039104960,
            "slot": 4,
            "status": "CHOP_MOVED",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1038838720,
            "info_lock": null,
            "order": 5,
            "possible": 1039104960,
            "slot": 5,
            "status": "CHOP_MOVED",
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
              4
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 23,
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
              3,
              3
            ],
            "inferred": 133120,
            "info_lock": null,
            "order": 16,
            "possible": 470235584,
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
            "inferred": 1048592,
            "info_lock": null,
            "order": 13,
            "possible": 1048592,
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
            "inferred": 470235584,
            "info_lock": null,
            "order": 12,
            "possible": 470235584,
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
              3
            ],
            "inferred": 33588271,
            "info_lock": null,
            "order": 11,
            "possible": 33588271,
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
        "k": "discard",
        "v": "None"
      },
      {
        "k": "play",
        "v": "None"
      }
    ],
    "pending_reactions": [],
    "play_stacks": [
      1,
      0,
      4,
      3,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 17,
    "waiting": []
  },
  "game_id": 9103,
  "replay": {
    "actions": [
      {
        "order": 0,
        "p": 0,
        "rank": 2,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 5,
        "suit": 3,
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
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 4,
        "p": 0,
        "rank": 4,
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
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 3,
        "suit": 0,
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
        "rank": 5,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "draw"
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
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 2,
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
        "order": 14,
        "p": 2,
        "rank": 1,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 3,
        "suit": 3,
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
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
        "giver": 2,
        "kind": "C",
        "list": [
          15,
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 0
      },
      {
        "clues": 5,
        "max": 30,
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
        "rank": 3,
        "suit": 3,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 5,
        "suit": 2,
        "t": "draw"
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
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 19,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "draw"
      },
      {
        "clues": 5,
        "max": 30,
        "score": 5,
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
          0,
          1,
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 4
      },
      {
        "clues": 4,
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
        "order": 3,
        "p": 0,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 20,
        "p": 0,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 30,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "failed": false,
        "order": 17,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "discard"
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
        "giver": 2,
        "kind": "C",
        "list": [
          4,
          18
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 4,
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
        "order": 4,
        "p": 0,
        "rank": 4,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 0,
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
        "num": 13,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          11,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 0
      },
      {
        "clues": 3,
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
        "order": 10,
        "p": 2,
        "rank": 3,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 4,
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
        "order": 22,
        "p": 0,
        "rank": 1,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 24,
        "p": 0,
        "rank": 3,
        "suit": 5,
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
        "num": 16,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      [
        4,
        2
      ],
      [
        3,
        5
      ],
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
        4
      ],
      null,
      [
        2,
        1
      ],
      [
        3,
        2
      ],
      null,
      [
        2,
        2
      ],
      [
        1,
        3
      ],
      [
        0,
        3
      ],
      [
        1,
        3
      ],
      [
        0,
        5
      ],
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
        1
      ],
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
      [
        0,
        1
      ],
      [
        1,
        4
      ],
      [
        5,
        3
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
      "variant_name": "Synesthesia & Black (6 Suits)"
    },
    "our_player_index": 1,
    "rlocks": true,
    "variant": "Synesthesia & Black (6 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-01T19:30:39.754",
  "turn": 17
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;
  const int bob = 2;  // yagami_black, the reacter
  ASSERT_EQ(s.our_player_index, 1);

  // The premise: Bob's slot 4 y3 is the last copy, and his slot 2 b3 is trash.
  const int y3_order = s.hands[bob][3];
  const int b3_order = s.hands[bob][1];
  ASSERT_TRUE(s.deck[y3_order].id().has_value());
  ASSERT_TRUE(s.deck[b3_order].id().has_value());
  EXPECT_TRUE(s.is_critical(*s.deck[y3_order].id()))
      << "the other y3 was discarded on T15, so this one is the last";
  EXPECT_TRUE(s.is_basic_trash(*s.deck[b3_order].id()))
      << "blue is already on 3, so this b3 can never be played";

  hanabi::PerformAction action = game.take_action();
  auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(colour, nullptr) << hanabi::to_json(action, 0).dump();
  EXPECT_EQ(colour->target, bob);
  EXPECT_EQ(colour->value, 4)
      << "purple (anchor 5) sends yagami_black to his slot 2 b3 -- trash. "
         "Yellow (anchor 2) sends him to the last y3 and red (anchor 1) to his "
         "r5; got "
      << hanabi::to_json(action, 0).dump();
}
