// A reactive is read against the stacks the GIVER and the RECEIVER share, not
// the ones all three seats share (1.3, v16.18.0).
//
// TIIAH replay 2010329 T14: yagami rank-2s will-bot67, will-bot69 answers with a
// y2, and will-bot67 -- holding the b2 the pairing named -- discarded its chop
// instead. Generated from the live log at turn 16; `apply_snapshot` replays the
// action history, so the clue and the reaction are interpreted by the build under
// test rather than restored from the log.

#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

// Variant: Throw It in a Hole (5 Suits). 3 players, our_player_index=0.

TEST(TiiahReplay2010329, TheFrameIsThePairNotTheTeam) {
  // Reconstruct exactly the Game the live bot saw at turn 16.
  // The embedded JSON is the STATE record's `replay` section.
  const char* kSnapshotJson = R"json(
{
  "bot": "will-bot67",
  "ch": "STATE",
  "current_player_index": 0,
  "database_id": 2010329,
  "debug": {
    "cards_left": 26,
    "clue_tokens": 4,
    "common_play_stacks": [
      1,
      0,
      0,
      0,
      0
    ],
    "current_player_index": 0,
    "discards": [
      {
        "order": 19,
        "rank": 1,
        "suit": 0
      },
      {
        "order": 18,
        "rank": 1,
        "suit": 2
      }
    ],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 2164802,
            "info_lock": null,
            "order": 22,
            "possible": 2164802,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 31389629,
            "info_lock": null,
            "order": 4,
            "possible": 31389629,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": false,
            "id": null,
            "inferred": 2164802,
            "info_lock": null,
            "order": 2,
            "possible": 2164802,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 31389629,
            "info_lock": null,
            "order": 1,
            "possible": 31389629,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 31389629,
            "info_lock": null,
            "order": 0,
            "possible": 31389629,
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
              1
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
              2
            ],
            "inferred": 65602,
            "info_lock": null,
            "order": 17,
            "possible": 67650,
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
            "inferred": 948117,
            "info_lock": null,
            "order": 15,
            "possible": 980925,
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
              5
            ],
            "inferred": 948117,
            "info_lock": null,
            "order": 9,
            "possible": 980925,
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
              1
            ],
            "inferred": 29360128,
            "info_lock": null,
            "order": 8,
            "possible": 30408704,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "yagami_light",
        "player": 1
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
            "clued": false,
            "focused": false,
            "id": [
              0,
              4
            ],
            "inferred": 31458267,
            "info_lock": null,
            "order": 16,
            "possible": 32506879,
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
              4
            ],
            "inferred": 14680522,
            "info_lock": null,
            "order": 14,
            "possible": 15729135,
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
            "inferred": 458752,
            "info_lock": null,
            "order": 13,
            "possible": 491520,
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
              5
            ],
            "inferred": 16777744,
            "info_lock": null,
            "order": 11,
            "possible": 16777744,
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
      }
    ],
    "pairwise_play_stacks": [
      [
        3,
        0,
        1,
        1,
        0
      ],
      [
        3,
        0,
        0,
        0,
        0
      ],
      [
        1,
        0,
        1,
        1,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      3,
      2,
      1,
      1,
      0
    ],
    "strikes": 0,
    "superpositions": [
      {
        "holder": 1,
        "order": 6,
        "superposition": 33792
      },
      {
        "holder": 1,
        "order": 7,
        "superposition": 33792
      },
      {
        "holder": 2,
        "order": 10,
        "superposition": 32770
      },
      {
        "holder": 2,
        "order": 12,
        "superposition": 2050
      },
      {
        "holder": 2,
        "order": 21,
        "superposition": 33554431
      }
    ],
    "turn_count": 16,
    "waiting": []
  },
  "game_id": 855,
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
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 1,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 1,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 9,
        "p": 1,
        "rank": 5,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 10,
        "p": 2,
        "rank": 2,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 5,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 3,
        "suit": 0,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 3,
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
          11
        ],
        "t": "clue",
        "target": 2,
        "value": 5
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 1,
        "rank": 3,
        "suit": 1,
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
        "order": 10,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 16,
        "p": 2,
        "rank": 4,
        "suit": 0,
        "t": "draw"
      },
      {
        "clues": 7,
        "max": 25,
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
        "value": 3
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 17,
        "p": 1,
        "rank": 2,
        "suit": 3,
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
        "order": 12,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 4,
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
          8
        ],
        "t": "clue",
        "target": 1,
        "value": 4
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
          18
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
        "giver": 2,
        "kind": "R",
        "list": [
          17
        ],
        "t": "clue",
        "target": 1,
        "value": 2
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
        "order": 3,
        "p": 0,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 19,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
        "order": 7,
        "p": 1,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 20,
        "p": 1,
        "rank": 1,
        "suit": 1,
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
        "order": 18,
        "p": 2,
        "rank": 1,
        "suit": 2,
        "t": "discard"
      },
      {
        "order": 21,
        "p": 2,
        "rank": 2,
        "suit": 1,
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
        "rank": 1,
        "suit": 0,
        "t": "discard"
      },
      {
        "order": 22,
        "p": 0,
        "rank": -1,
        "suit": -1,
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
          2,
          22
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
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 23,
        "p": 2,
        "rank": 3,
        "suit": 3,
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
      }
    ],
    "all_plays": false,
    "convention": "tiiah",
    "deck": [
      null,
      null,
      null,
      null,
      null,
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
        1
      ],
      [
        4,
        1
      ],
      [
        1,
        5
      ],
      [
        0,
        2
      ],
      [
        4,
        5
      ],
      [
        0,
        3
      ],
      [
        3,
        3
      ],
      [
        0,
        4
      ],
      [
        1,
        3
      ],
      [
        0,
        4
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
        0,
        1
      ],
      [
        1,
        1
      ],
      [
        1,
        2
      ],
      null,
      [
        3,
        3
      ]
    ],
    "names": [
      "will-bot67",
      "yagami_light",
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
      "variant_name": "Throw It in a Hole (5 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole (5 Suits)",
    "zcs_turn": -1
  },
  "ts": "2026-09-26T20:36:37.234",
  "turn": 16
}
  )json";
  auto rec = nlohmann::json::parse(kSnapshotJson);
  hanabi::Game game = hanabi::logging::apply_snapshot(rec);

  // THE FRAME. yagami's rank 2 named will-bot67's order 22, and the reading of
  // what that card IS binds to the stacks the two of them share -- not to the
  // shared view, which is held back by whatever the third seat cannot name.
  //
  // Their row has to be [3,1,1,1,0] here, and three separate deductions put it
  // there, none of which the bot used to make:
  //   * red 3 was already common.
  //   * yellow 1: our own order 3 was a {r4,y1}, and both r4 copies sit in
  //     will-bot69's hand -- a hand NEITHER of us holds -- so yagami rules the r4
  //     out exactly as we do (1e rule 3's pair form).
  //   * green 1 and blue 1: yagami's orders 6 and 7 are each {g1,b1}. It cannot
  //     name its own plays, but (g1,g1) and (b1,b1) each strike, so under rule 6
  //     both suits are on 1 in every world that survives.
  // [3,1,1,1,0] at the moment yagami clued; the snapshot is one turn further on,
  // after the reacter's y2, which every seat but will-bot69 watched.
  const std::vector<int> expected_row{3, 2, 1, 1, 0};
  EXPECT_EQ(game.state.pairwise_play_stacks[1], expected_row)
      << "what will-bot67 knows yagami knows -- it read [3,0,0,0,0] before, and "
         "nothing in it had moved since turn 7";

  // Read against the shared [1,0,0,0,0] instead, the promise came out {r2,y2} --
  // red was on 3 and yellow on 2 by then, so both were trash and
  // `stamp_receiver_call`'s Rule 5 dropped the whole call as a stale reading. The
  // card kept its bare rank-2 empathy and will-bot67 discarded its chop.
  const int order = 22;
  EXPECT_EQ(game.meta[order].status, hanabi::CardStatus::CALLED_TO_PLAY)
      << "the reaction called this card, and nothing about it went stale";

  hanabi::IdentitySet g2b2 = hanabi::IdentitySet::empty();
  g2b2 = g2b2.add(hanabi::Identity{2, 2});
  g2b2 = g2b2.add(hanabi::Identity{3, 2});
  EXPECT_EQ(game.common.thoughts[order].inferred, g2b2)
      << "bucket 1 is {g,b}, one step up from the y2 the reacter played, and the "
         "frame makes both of them playable -- the card is the b2";

  hanabi::PerformAction action = game.take_action();
  const auto* play = std::get_if<hanabi::PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "a called card that is playable in every world of "
                              "its own reading is a play, not a chop discard";
  EXPECT_EQ(play->target, order);
}
