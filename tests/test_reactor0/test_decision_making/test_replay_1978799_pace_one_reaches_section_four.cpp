// At pace 1 Alice must not throw a card that leaves the endgame unwinnable.
//
// Replay 1978799 T18, `Synesthesia & Brown (3 Suits)`. will-bot67 is Alice with
// stacks [3,3,3], 4 clue tokens and 4 cards left against 6 points still owed
// across three players -- pace 1. She is not locked and not at 8 tokens.
//
// Through v13.1.0 section 4's pace arm read `pace() == 0`, so at pace 1 the arm
// was shut, `choose_clue` declined and phase 2 discarded order 24 -- spending
// one of the last turns on a burn.
//
// v13.2.0 widens it to `pace() <= 1` guarded by 4a-4c, and 4a holds here: none
// of will-bot67's five cards has an all-playable inference, so
// `thinks_playables` is empty and she has nothing of her own to make.
//
// The clue it reaches is Brown to yagami_black. Three separate things have to
// line up for that to be legal and readable, and all three do:
//   * score 9 of 15, so `2 * 9 >= 15` is past the 50% switch and a clue to Bob
//     is STABLE rather than reactive;
//   * the variant has three clue colours (r/b/br), so v13.0.0's two-colour rule
//     -- colour is never stable -- does not apply;
//   * Synesthesia reads a stable colour off its fixed table, and Brown falls to
//     the catch-all: PITCH SLOT 4 (`synesthesia_stable.h`).
//
// So rung 4.1 ("same as 3.1", `pool_stable_play`) picks it, and yagami_black is
// called to play his slot 4.

#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Synesthesia & Brown (3 Suits). 3 players, our_player_index=2.

TEST(DecisionMaking1978799, T18PaceOneCluesRatherThanBurningATurn) {
  // Reconstruct exactly the Game the live bot saw at turn 18.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 2,
  "database_id": 1978799,
  "debug": {
    "cards_left": 4,
    "clue_tokens": 4,
    "current_player_index": 2,
    "discards": [
      {
        "order": 17,
        "rank": 4,
        "suit": 1
      },
      {
        "order": 15,
        "rank": 3,
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
              0,
              3
            ],
            "inferred": 28671,
            "info_lock": null,
            "order": 18,
            "possible": 28671,
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
            "inferred": 832,
            "info_lock": null,
            "order": 4,
            "possible": 832,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              4
            ],
            "inferred": 59,
            "info_lock": null,
            "order": 3,
            "possible": 59,
            "slot": 3,
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
            "inferred": 27776,
            "info_lock": null,
            "order": 2,
            "possible": 27776,
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
            "inferred": 59,
            "info_lock": null,
            "order": 1,
            "possible": 59,
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
            "id": [
              2,
              4
            ],
            "inferred": 28671,
            "info_lock": null,
            "order": 25,
            "possible": 28671,
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
            "inferred": 27780,
            "info_lock": null,
            "order": 23,
            "possible": 27780,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": [
              1,
              3
            ],
            "inferred": 27780,
            "info_lock": null,
            "order": 21,
            "possible": 27780,
            "slot": 3,
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
            "inferred": 27780,
            "info_lock": null,
            "order": 19,
            "possible": 27780,
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
        "name": "will-bot69",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 28671,
            "info_lock": null,
            "order": 24,
            "possible": 28671,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 635,
            "info_lock": null,
            "order": 22,
            "possible": 891,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 635,
            "info_lock": null,
            "order": 14,
            "possible": 891,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 25728,
            "info_lock": null,
            "order": 12,
            "possible": 27780,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 635,
            "info_lock": null,
            "order": 11,
            "possible": 891,
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
        "k": "play",
        "v": "None"
      }
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      3,
      3
    ],
    "strikes": 0,
    "turn_count": 18,
    "waiting": []
  },
  "game_id": 5546,
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
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 2,
        "p": 0,
        "rank": 4,
        "suit": 2,
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
        "rank": 2,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 5,
        "p": 1,
        "rank": 1,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
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
          10,
          12,
          13
        ],
        "t": "clue",
        "target": 2,
        "value": 2
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
        "suit": 2,
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
          1,
          3
        ],
        "t": "clue",
        "target": 0,
        "value": 0
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
        "order": 0,
        "p": 0,
        "rank": 1,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 0,
        "rank": 2,
        "suit": 1,
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
        "giver": 1,
        "kind": "C",
        "list": [
          2
        ],
        "t": "clue",
        "target": 0,
        "value": 2
      },
      {
        "clues": 5,
        "max": 15,
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
        "suit": 2,
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
        "max": 15,
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
        "rank": 2,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 0,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 5,
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
        "failed": false,
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 19,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
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
        "failed": false,
        "order": 17,
        "p": 2,
        "rank": 4,
        "suit": 1,
        "t": "discard"
      },
      {
        "order": 20,
        "p": 2,
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
        "cpi": 0,
        "num": 9,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 6,
        "max": 15,
        "score": 4,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 10,
        "t": "turn"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "play"
      },
      {
        "order": 21,
        "p": 1,
        "rank": 3,
        "suit": 1,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 15,
        "score": 5,
        "t": "status"
      },
      {
        "cpi": 2,
        "num": 11,
        "t": "turn"
      },
      {
        "order": 20,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 22,
        "p": 2,
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
        "cpi": 0,
        "num": 12,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          10,
          12
        ],
        "t": "clue",
        "target": 2,
        "value": 2
      },
      {
        "clues": 5,
        "max": 15,
        "score": 6,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 13,
        "t": "turn"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 3,
        "suit": 0,
        "t": "play"
      },
      {
        "order": 23,
        "p": 1,
        "rank": 2,
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
        "cpi": 2,
        "num": 14,
        "t": "turn"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 24,
        "p": 2,
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
        "cpi": 0,
        "num": 15,
        "t": "turn"
      },
      {
        "giver": 0,
        "kind": "C",
        "list": [
          6,
          19,
          21,
          23
        ],
        "t": "clue",
        "target": 1,
        "value": 2
      },
      {
        "clues": 4,
        "max": 15,
        "score": 8,
        "t": "status"
      },
      {
        "cpi": 1,
        "num": 16,
        "t": "turn"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 3,
        "suit": 2,
        "t": "play"
      },
      {
        "order": 25,
        "p": 1,
        "rank": 4,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 4,
        "max": 15,
        "score": 9,
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
        1,
        1
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
        4
      ],
      [
        1,
        2
      ],
      [
        0,
        1
      ],
      [
        2,
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
        1
      ],
      [
        2,
        2
      ],
      null,
      null,
      [
        2,
        1
      ],
      null,
      [
        2,
        3
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
        0,
        3
      ],
      [
        2,
        1
      ],
      [
        0,
        2
      ],
      [
        1,
        3
      ],
      null,
      [
        2,
        2
      ],
      null,
      [
        2,
        4
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
      "variant_name": "Synesthesia & Brown (3 Suits)"
    },
    "our_player_index": 2,
    "reactive_overrides": [
      {
        "clue_value": 1,
        "even": true,
        "kind": "C",
        "reactive_value": 5
      }
    ],
    "rlocks": false,
    "variant": "Synesthesia & Brown (3 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-08-30T15:25:00.239",
  "turn": 18
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);
  ASSERT_EQ(game.state.pace(), 1) << "guard: the widened arm's whole subject";
  ASSERT_NE(game.state.clue_tokens, 8)
      << "guard: not the 8-clue arm -- this is the pace arm or nothing";

  hanabi::PerformAction action = game.take_action();
  auto* colour = std::get_if<hanabi::PerformColour>(&action);
  ASSERT_NE(colour, nullptr)
      << "she must clue rather than burn a turn at pace 1";
  EXPECT_EQ(colour->target, 0) << "to yagami_black, her Bob";
  EXPECT_EQ(colour->value, 1)
      << "Blue, of r/b/br -- which v14.0.0's Synesthesia table makes the PITCH "
         "of slot 4. The turn is unchanged in substance: through v13.5.0 the "
         "same slot-4 pitch was spelled Brown (the old catch-all), and the "
         "rewrite moved that meaning onto Blue while sending Brown to a chuck";
}
