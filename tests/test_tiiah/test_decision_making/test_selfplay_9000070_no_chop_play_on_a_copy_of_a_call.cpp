// Self-play TIIAH & Black (6 Suits), game 9000070, from the first A/B of v23.40.0
// (tiiah/CONVENTION.md §2v, the user's ruling; human_vs_bot_diagnostics/2028742.md).
// sim-bob holds o6, the y1 called to play, so a clue to sim-cathy is stable. She has no
// safe action, and her chop o15 is the k5. Yellow would call her o11, the other y1: a
// copy of sim-bob's call, which she would play into a strike after his landed. So it is
// not rung 2d's play, and the 5 that touches the k5 is given.

#include <gtest/gtest.h>

#include <utility>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/logging/state_snapshot.h"
#include "replay_helpers.h"
#include "test_harness.h"

namespace {

// T4, sim-alice's seat. Variant: Throw It in a Hole & Black (6 Suits). 3 players,
// our_player_index=0. The embedded JSON is the STATE record's `replay` section.
hanabi::Game position() {
  static const char* kSnapshotJson = R"json(
{
  "bot": "sim-alice",
  "ch": "STATE",
  "current_player_index": 0,
  "debug": {
    "cards_left": 39,
    "clue_tokens": 6,
    "common_play_stacks": [
      0,
      0,
      0,
      1,
      0,
      0
    ],
    "current_player_index": 0,
    "discards": [],
    "endgame_turns": null,
    "hands": [
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 4,
            "possible": 1073741823,
            "slot": 1,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 3,
            "possible": 1073741823,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 2,
            "possible": 1073741823,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 1,
            "possible": 1073741823,
            "slot": 4,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": null,
            "inferred": 1073741823,
            "info_lock": null,
            "order": 0,
            "possible": 1073741823,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-alice",
        "player": 0
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              3,
              4
            ],
            "inferred": 1073740831,
            "info_lock": null,
            "order": 9,
            "possible": 1073740831,
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
            "inferred": 1073740831,
            "info_lock": null,
            "order": 8,
            "possible": 1073740831,
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
              4
            ],
            "inferred": 1073740831,
            "info_lock": null,
            "order": 7,
            "possible": 1073740831,
            "slot": 3,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": true,
            "focused": true,
            "id": [
              1,
              1
            ],
            "inferred": 32,
            "info_lock": 32,
            "order": 6,
            "possible": 992,
            "slot": 4,
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
            "inferred": 1073740831,
            "info_lock": null,
            "order": 5,
            "possible": 1073740831,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-bob",
        "player": 1
      },
      {
        "cards": [
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              5
            ],
            "inferred": 1073741823,
            "info_lock": null,
            "order": 15,
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
              2,
              3
            ],
            "inferred": 1072726015,
            "info_lock": null,
            "order": 13,
            "possible": 1072726015,
            "slot": 2,
            "status": "NONE",
            "trash": false,
            "urgent": false
          },
          {
            "clued": false,
            "focused": false,
            "id": [
              5,
              2
            ],
            "inferred": 1072726015,
            "info_lock": null,
            "order": 12,
            "possible": 1072726015,
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
              1
            ],
            "inferred": 1072726015,
            "info_lock": null,
            "order": 11,
            "possible": 1072726015,
            "slot": 4,
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
            "inferred": 1072726015,
            "info_lock": null,
            "order": 10,
            "possible": 1072726015,
            "slot": 5,
            "status": "NONE",
            "trash": false,
            "urgent": false
          }
        ],
        "name": "sim-cathy",
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
        "v": "Play"
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
    "pairwise_play_stacks": [
      [
        0,
        0,
        0,
        0,
        0,
        0
      ],
      [
        0,
        0,
        0,
        1,
        0,
        0
      ],
      [
        0,
        0,
        0,
        1,
        0,
        0
      ]
    ],
    "pending_reactions": [],
    "play_stacks": [
      0,
      0,
      0,
      1,
      0,
      0
    ],
    "strikes": 0,
    "turn_count": 4,
    "waiting": []
  },
  "game_id": 9000070,
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
        "rank": 5,
        "suit": 2,
        "t": "draw"
      },
      {
        "order": 6,
        "p": 1,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 7,
        "p": 1,
        "rank": 4,
        "suit": 3,
        "t": "draw"
      },
      {
        "order": 8,
        "p": 1,
        "rank": 4,
        "suit": 0,
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
        "rank": 4,
        "suit": 4,
        "t": "draw"
      },
      {
        "order": 11,
        "p": 2,
        "rank": 1,
        "suit": 1,
        "t": "draw"
      },
      {
        "order": 12,
        "p": 2,
        "rank": 2,
        "suit": 5,
        "t": "draw"
      },
      {
        "order": 13,
        "p": 2,
        "rank": 3,
        "suit": 2,
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
          6
        ],
        "t": "clue",
        "target": 1,
        "value": 1
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
          14
        ],
        "t": "clue",
        "target": 2,
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
        "order": 14,
        "p": 2,
        "rank": -1,
        "suit": -1,
        "t": "play"
      },
      {
        "order": 15,
        "p": 2,
        "rank": 5,
        "suit": 5,
        "t": "draw"
      },
      {
        "clues": 6,
        "max": 25,
        "score": 0,
        "t": "status"
      },
      {
        "cpi": 0,
        "num": 3,
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
        2,
        5
      ],
      [
        1,
        1
      ],
      [
        3,
        4
      ],
      [
        0,
        4
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
        1,
        1
      ],
      [
        5,
        2
      ],
      [
        2,
        3
      ],
      [
        3,
        1
      ],
      [
        5,
        5
      ]
    ],
    "names": [
      "sim-alice",
      "sim-bob",
      "sim-cathy"
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
      "variant_name": "Throw It in a Hole & Black (6 Suits)"
    },
    "our_player_index": 0,
    "rlocks": true,
    "variant": "Throw It in a Hole & Black (6 Suits)",
    "zcs_cleared": 0,
    "zcs_turn": -1
  },
  "ts": "2026-10-12T18:51:25.245",
  "turn": 4
}
  )json";
  return hanabi::logging::apply_snapshot(nlohmann::json::parse(kSnapshotJson));
}

std::vector<std::pair<hanabi::PerformAction, hanabi::Action>> all_candidate_clues(
    const hanabi::Game& g) {
  const hanabi::State& s = g.state;
  std::vector<std::pair<hanabi::PerformAction, hanabi::Action>> out;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == s.our_player_index) continue;
    for (const hanabi::Clue& clue : s.all_valid_clues(target)) {
      hanabi::PerformAction perform =
          clue.kind == hanabi::ClueKind::COLOUR
              ? hanabi::PerformAction{hanabi::PerformColour{clue.target, clue.value}}
              : hanabi::PerformAction{hanabi::PerformRank{clue.target, clue.value}};
      hanabi::ClueAction act{s.our_player_index, clue.target,
                             s.clue_touched(s.hands[target], clue.kind, clue.value),
                             clue.base()};
      out.emplace_back(perform, hanabi::Action{act});
    }
  }
  return out;
}

const hanabi::reactor0::ClueCandidate* clue_to(
    const std::vector<hanabi::reactor0::ClueCandidate>& cands, int target,
    hanabi::ClueKind kind, int value) {
  for (const auto& c : cands) {
    if (c.action.target == target && c.action.clue.kind == kind &&
        c.action.clue.value == value) {
      return &c;
    }
  }
  return nullptr;
}

constexpr int kYellow = 1;  // suit index: r y g b p k

}  // namespace

TEST(TiiahSelfPlay9000070, NoChopSavingPlayOnACopyOfBobsCall) {
  hanabi::Game game = position();
  const auto cands = hanabi::reactor0::analyse_clues(game, all_candidate_clues(game));
  const auto* yellow = clue_to(cands, 2, hanabi::ClueKind::COLOUR, kYellow);
  ASSERT_NE(yellow, nullptr);
  EXPECT_EQ(yellow->reading.shape, hanabi::reactor0::ClueShape::STABLE_PLAY);
  EXPECT_EQ(yellow->reading.stable_subject, 11) << "guard: a call on o11, the y1";
  EXPECT_FALSE(yellow->cathy_play_saves_chop) << "sim-bob's o6 is the called y1";
  hanabi::PerformAction action = game.take_action();
  const auto* rank = std::get_if<hanabi::PerformRank>(&action);
  ASSERT_NE(rank, nullptr) << "a rank clue";
  EXPECT_EQ(rank->target, 2) << "to sim-cathy";
  EXPECT_EQ(rank->value, 5) << "touching o15, the k5";
}
