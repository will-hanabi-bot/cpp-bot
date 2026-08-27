// A reactive discard must survive a react slot whose CHUCK reading is dead.
//
// Replay 1974342 T13, `Deceptive-Ones & Orange (5 Suits)`. yagami_black clues
// GREEN to will-bot67: odd bucket, anchor 3, so `react_slot + target_slot = 3`.
// The receiver's leftmost playable is his slot 3 (b2, blue on 1), which pairs
// with the reacter's slot 5 -- will-bot69 discards slot 5, will-bot67 plays
// slot 3. A plain reactive discard.
//
// will-bot69 read it as a MISTAKE. Colour mode 1 used to pick between the two
// Discard-button stamps with a `react_could_chuck` gate that asked "could a
// chuck stack this?" of `possible`, while `stamp_orange_chuck` asks the same
// question of `possibilities()` -- `inferred` once non-empty. reactor0's own
// deferred negatives (`slot_elims`) strip the PLAYABLE identities from a
// reacter's unpaired slots, which is that exact axis. Both of will-bot69's
// candidate react slots held {r2,r3,r4,y3,y4,g2,g3,g4,o2,o3,o4} with orange on
// 0: `possible` still admitted the o1, `inferred` did not. So the gate routed
// to the chuck, the chuck found nothing playable and inverted left to name, and
// with no fallback to the ordinary discard both pairings died and the pool ran
// out. Both slots were chuckable all along through the plain arm -- r2, two
// copies, neither discarded.
//
// Cost: the bot fell through to rung `12.discard_chop` and threw its slot 1,
// which was the Blue 5 (max 25 -> 24). will-bot67, whose receiver POV returns
// REACTIVE unconditionally, then decoded that unrelated chop discard as the
// reaction -- react_slot 1 => target_slot 2 -- and stamped CALLED_TO_PLAY on
// its r4 with red on 0.
//
// v10.9.0 replaces the gate with `stamp_react_discard_button`, the mirror of
// v10.8.0's `stamp_react_play_button`: chuck first, ordinary discard second,
// refuse only when neither reading exists.
//
// The interpretation asserts below are the point of the test as much as the
// action is. While the clue reads MISTAKE, `Game::find_all_clues` filters it out
// of every candidate list, so no bot can ever GIVE it -- which is why bot-vs-bot
// self-play could never have surfaced this and it took a human at the table.

#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Deceptive-Ones & Orange (5 Suits). 3 players, our_player_index=0.

TEST(BadReactiveTarget1974342, ReactiveDiscardSurvivesADeadChuck) {
  // Reconstruct exactly the Game the live bot saw at turn 13.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot69",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 1974342,
  "debug": {
    "cards_left": 29,
    "clue_tokens": 3,
    "current_player_index": 0,
    "discards": [
      {
        "order": 7,
        "rank": 2,
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
            "id": null,
            "inferred": 33554367,
            "info_lock": null,
            "order": 18,
            "possible": 33554367,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 17351184,
            "info_lock": null,
            "order": 16,
            "possible": 17351184,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 16794128,
            "info_lock": null,
            "order": 3,
            "possible": 16794128,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 14694798,
            "info_lock": null,
            "order": 2,
            "possible": 15744431,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 14694798,
            "info_lock": null,
            "order": 0,
            "possible": 15744431,
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
            "clued": true,
            "focused": false,
            "id": [
              2,
              1
            ],
            "inferred": 31744,
            "info_lock": null,
            "order": 20,
            "possible": 31744,
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
            "inferred": 33522623,
            "info_lock": null,
            "order": 15,
            "possible": 33522623,
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
            "inferred": 29197083,
            "info_lock": null,
            "order": 9,
            "possible": 29197083,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              2,
              3
            ],
            "inferred": 4096,
            "info_lock": null,
            "order": 8,
            "possible": 4096,
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
            "inferred": 29197083,
            "info_lock": null,
            "order": 6,
            "possible": 29197083,
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
              0,
              3
            ],
            "inferred": 32538559,
            "info_lock": null,
            "order": 19,
            "possible": 32538559,
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
            "inferred": 32537631,
            "info_lock": null,
            "order": 14,
            "possible": 32537631,
            "slot": 2,
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
            "inferred": 1015808,
            "info_lock": null,
            "order": 13,
            "possible": 1015808,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              3,
              1
            ],
            "inferred": 425984,
            "info_lock": null,
            "order": 11,
            "possible": 1015808,
            "slot": 4,
            "status": "CALLED_TO_DISCARD",
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
            "inferred": 32537631,
            "info_lock": null,
            "order": 10,
            "possible": 32537631,
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
        "v": "Play"
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
        "k": "clue",
        "v": "Reactive"
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
      0,
      2,
      2,
      1,
      0
    ],
    "strikes": 0,
    "turn_count": 13,
    "waiting": [
      {
        "all_plays": false,
        "clue_kind": "C",
        "clue_value": 2,
        "focus_slot": 3,
        "giver": 2,
        "inverted": false,
        "react_order": -1,
        "reacter": 0,
        "receiver": 1,
        "receiver_hand": [
          20,
          15,
          9,
          8,
          6
        ],
        "rlocks": false,
        "turn": 12
      }
    ]
  },
  "game_id": 244,
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
        "rank": 4,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 2,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 3,
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
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 14,
        "p": 2,
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "giver": 0,
        "kind": "R",
        "list": [
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 3
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
        "order": 7,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 8,
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
        "giver": 2,
        "kind": "C",
        "list": [
          1
        ],
        "t": "clue",
        "target": 0,
        "value": 3
      },
      {
        "clues": 7,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
        "t": "turn"
      },
      {
        "order": 1,
        "p": 0,
        "rank": 1,
        "suit": 3,
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
        "clues": 7,
        "max": 25,
        "score": 1,
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
          3,
          16
        ],
        "t": "clue",
        "target": 0,
        "value": 5
      },
      {
        "clues": 6,
        "max": 25,
        "score": 1,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 5,
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
        "order": 17,
        "p": 2,
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
        "cpi": 0,
        "num": 6,
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
        "order": 18,
        "p": 0,
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
        "cpi": 1,
        "num": 7,
        "t": "turn"
      },
      {
        "giver": 1,
        "kind": "C",
        "list": [
          17
        ],
        "t": "clue",
        "target": 2,
        "value": 1
      },
      {
        "clues": 5,
        "max": 25,
        "score": 3,
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
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "giver": 0,
        "kind": "C",
        "list": [
          11,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 3
      },
      {
        "clues": 4,
        "max": 25,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
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
        "giver": 2,
        "kind": "C",
        "list": [
          8,
          20
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 3,
        "max": 25,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 12,
        "t": "turn"
      }
    ],
    "all_plays": false,
    "convention": "reactor0",
    "deck": [
      null,
      [
        3,
        1
      ],
      null,
      null,
      [
        2,
        2
      ],
      [
        1,
        2
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
        3,
        2
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
        2,
        1
      ],
      [
        3,
        4
      ],
      [
        4,
        4
      ],
      [
        0,
        4
      ],
      null,
      [
        1,
        1
      ],
      null,
      [
        0,
        3
      ],
      [
        2,
        1
      ]
    ],
    "names": [
      "will-bot69",
      "will-bot67",
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
      "variant_name": "Deceptive-Ones & Orange (5 Suits)"
    },
    "our_player_index": 0,
    "rlocks": false,
    "variant": "Deceptive-Ones & Orange (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-27T00:36:39.808",
  "turn": 13
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  const hanabi::State& s = game.state;
  const int us = s.our_player_index;

  const int slot5_order = s.hands[us][4];  // order 0 -- the react slot
  const int slot1_order = s.hands[us][0];  // order 18 -- the Blue 5 it threw
  ASSERT_EQ(slot5_order, 0);
  ASSERT_EQ(slot1_order, 18);

  // 1. The clue must be READ. A MISTAKE here is not merely a missed reaction:
  //    `find_all_clues` drops MISTAKE candidates, so for as long as this reads
  //    MISTAKE the clue is one no bot can offer.
  auto last = game.last_move();
  ASSERT_TRUE(last.has_value()) << "the green clue should have been interpreted";
  ASSERT_TRUE(std::holds_alternative<hanabi::ClueInterp>(*last))
      << "the last move is a clue";
  EXPECT_EQ(std::get<hanabi::ClueInterp>(*last), hanabi::ClueInterp::REACTIVE)
      << "a colour clue to Cathy with a playable in her hand is a reactive; "
         "reading it as a MISTAKE is the bug, and also makes the clue "
         "unofferable by any bot";

  // 2. The pairing must actually be recorded, not merely coincide with the
  //    action. `react_order == -1` is what a swallowed reading leaves behind.
  ASSERT_FALSE(game.waiting.empty()) << "a reactive leaves a waiting connection";
  EXPECT_EQ(game.waiting.front().react_order, slot5_order)
      << "anchor 3 pairs the receiver's slot-3 playable with our slot 5";
  EXPECT_EQ(game.waiting.front().focus_slot, 3) << "green's reactive value";

  // 3. And the reaction is actioned.
  hanabi::PerformAction action = game.take_action();

  auto* discard = std::get_if<hanabi::PerformDiscard>(&action);
  ASSERT_NE(discard, nullptr)
      << "the odd bucket puts the reacter on the Discard button";
  EXPECT_EQ(discard->target, slot5_order)
      << "slot 5 is the paired react slot";
  EXPECT_NE(discard->target, slot1_order)
      << "chucking the chop -- which was the Blue 5 -- was the bug";
}
