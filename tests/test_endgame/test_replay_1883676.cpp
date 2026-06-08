// Hanab.live replay 1883676 turn 1 (action index 0). Variant
// "Orange (3 Suits)". Players: orig P0=will-bot69 (giver / our bot POV),
// P1=yagami_black, P2=will-bot67.
//
// On action 0 will-bot69 was observed picking "blue to will-bot67" — focus
// slot 3, the only reactive play-target is the orange-1 at will-bot67's
// slot 2. yagami's react_slot = calc_slot(3, 2, 5) = 1, which is r3.
// Standard color+play_target ⇒ target_discard(r3) ⇒ yagami physically
// discards r3 (regains a clue) and will-bot67 reads (color + react_discard)
// ⇒ target_i_play on the orange-1 — but the receiver-orange swap means
// will-bot67 should *physically discard* the orange-1 instead, and the
// only way to convey that is target_play(r3). target_play on r3 fails the
// has_consistent_infs check (r3's true id (R,3) is not in {r1,b1,o1}),
// so the convention should return MISTAKE for this clue.
//
// The probe: confirm that our bot rejects this clue (does NOT pick blue
// to will-bot67 at action 0).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/identity.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

Game build_initial() {
  SetupOptions opts;
  opts.hands = {
      // Alice = will-bot69 (P0, observer): hidden.
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob = yagami_black (P1): orig [9,8,7,6,5] = [r3,r2,r3,o2,b2].
      {"r3", "r2", "r3", "o2", "b2"},
      // Cathy = will-bot67 (P2): orig [14,13,12,11,10] = [b5,o1,b4,r4,b4].
      {"b5", "o1", "b4", "r4", "b4"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;  // orig P0 starts.
  return setup(std::move(opts));
}

}  // namespace

TEST(EndgameReplay1883676, BlueClueToWillBot67IsRejected) {
  Game g = build_initial();

  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  // Probe blue-to-Cathy specifically.
  ClueAction blue_to_cathy{
      /*giver=*/0,
      /*target=*/static_cast<int>(TestPlayer::CATHY),
      g.state.clue_touched(g.state.hands[static_cast<int>(TestPlayer::CATHY)],
                             ClueKind::COLOUR, 1),
      BaseClue(ClueKind::COLOUR, 1),
  };
  double blue_score = hanabi::reactor::eval_action(g, Action{blue_to_cathy});
  std::cerr << "[probe] blue-to-Cathy score = " << blue_score << "\n";

  PerformAction perform = g.take_action();
  std::cerr << "[probe] take_action picked: ";
  if (std::holds_alternative<PerformPlay>(perform)) {
    std::cerr << "play order " << std::get<PerformPlay>(perform).target << "\n";
  } else if (std::holds_alternative<PerformDiscard>(perform)) {
    std::cerr << "discard order " << std::get<PerformDiscard>(perform).target << "\n";
  } else if (std::holds_alternative<PerformColour>(perform)) {
    auto p = std::get<PerformColour>(perform);
    std::cerr << "colour " << p.value << " to P" << p.target << "\n";
  } else if (std::holds_alternative<PerformRank>(perform)) {
    auto p = std::get<PerformRank>(perform);
    std::cerr << "rank " << p.value << " to P" << p.target << "\n";
  }

  // Blue-to-Cathy is a misfire and must score as MISTAKE (-100).
  EXPECT_LE(blue_score, -50.0)
      << "blue-to-Cathy should evaluate as MISTAKE — its only reactive "
         "play-target o1 routes through target_play on yagami's r3, which "
         "the convention now rejects (target_play on a non-orange reacter "
         "card whose true id is not in the playable narrowing also fails "
         "the consistency check).";

  // And the bot picks something else.
  if (std::holds_alternative<PerformColour>(perform)) {
    auto p = std::get<PerformColour>(perform);
    EXPECT_FALSE(p.target == static_cast<int>(TestPlayer::CATHY) && p.value == 1)
        << "bot should not give blue to will-bot67";
  }

  // Sanity: also reject any clue whose convention path resolves to
  // target_discard on a non-playable orange reacter (e.g. rank-5 to Cathy,
  // whose orange play-target o1 maps to yagami's slot-4 = o2, which is
  // not playable on the orange stack yet).
  ClueAction rank5_to_cathy{
      /*giver=*/0,
      /*target=*/static_cast<int>(TestPlayer::CATHY),
      g.state.clue_touched(g.state.hands[static_cast<int>(TestPlayer::CATHY)],
                             ClueKind::RANK, 5),
      BaseClue(ClueKind::RANK, 5),
  };
  double rank5_score = hanabi::reactor::eval_action(g, Action{rank5_to_cathy});
  std::cerr << "[probe] rank-5-to-Cathy score = " << rank5_score << "\n";
  EXPECT_LE(rank5_score, -50.0)
      << "rank-5-to-Cathy should also evaluate as MISTAKE — target_discard "
         "on yagami's non-playable o2 would misplay";
}
