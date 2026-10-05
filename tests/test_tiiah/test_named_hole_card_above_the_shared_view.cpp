// A hole card the team names ABOVE the shared view explains the gap below it
// (tiiah/CONVENTION.md §1e, v20.16.0; replay 2019408 T16).
//
// Bob throws two cards into the hole: the first could be the r1 or the y1, the
// second the y2 or the b1. Cathy then plays a b1 she knows, so the second was the
// y2 -- with yellow on 0 in common, since the first is unnamed. Until v20.16.0 the
// y2 settled and put nothing on the shared view. Now the worlds explain it: the first
// was the y1, and yellow is on 2. The control: a first card that cannot be the y1
// leaves nothing to explain it, and stays superposed.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr int kYellow = 1;

struct Hole {
  Game g;
  int first = -1;
  int second = -1;
};

Hole two_in_the_hole(const char* first_could_be) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b4", "p4", "y2", "y1"},
      {"b1", "g3", "p3", "r3", "y4"},
  };
  opts.clue_tokens = 6;
  opts.starting = TestPlayer::BOB;
  use_tiiah(opts);
  Hole h;
  h.g = setup(std::move(opts));
  h.g = pre_clue(std::move(h.g), TestPlayer::CATHY, 1, {"blue", "1"});
  h.first = order_at(h.g, TestPlayer::BOB, 5);
  h.g = hidden_action(std::move(h.g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/true,
                      "g5");
  h.g = hidden_action(std::move(h.g), TestPlayer::CATHY, /*slot=*/5, /*reached_the_hole=*/false,
                      "r5");
  h.g = hidden_action(std::move(h.g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/false);
  h.second = order_at(h.g, TestPlayer::BOB, 5);
  h.g = hidden_action(std::move(h.g), TestPlayer::BOB, /*slot=*/5, /*reached_the_hole=*/true,
                      "p5");
  const IdentitySet first_set = IdentitySet::empty()
                                    .add(h.g.state.expand_short("r1"))
                                    .add(h.g.state.expand_short(first_could_be));
  const IdentitySet second_set =
      IdentitySet::empty().add(h.g.state.expand_short("y2")).add(h.g.state.expand_short("b1"));
  h.g.with_meta(h.first, [first_set](ConvData& m) {
    m.superposition = first_set;
    m.shared_left = IdentitySet::empty();
  });
  h.g.with_meta(h.second, [second_set](ConvData& m) {
    m.superposition = second_set;
    m.shared_left = IdentitySet::empty();
  });
  // Cathy plays the b1 she knows.
  h.g = hidden_action(std::move(h.g), TestPlayer::CATHY, /*slot=*/2, /*reached_the_hole=*/true,
                      "g2");
  return h;
}

}  // namespace

TEST(TiiahNamedAboveTheView, TheGapIsExplained) {
  Hole h = two_in_the_hole("y1");
  ASSERT_FALSE(h.g.meta[h.second].superposed()) << "guard: the second settled as the y2";
  EXPECT_EQ(h.g.state.common_play_stacks[kYellow], 2);
  EXPECT_FALSE(h.g.meta[h.first].superposed()) << "the first was the y1";
}

TEST(TiiahNamedAboveTheView, NothingToExplainItLeavesTheGapOpen) {
  Hole h = two_in_the_hole("b2");
  ASSERT_FALSE(h.g.meta[h.second].superposed()) << "guard: the second settled as the y2";
  EXPECT_TRUE(h.g.meta[h.first].superposed()) << "the first cannot be the y1";
}
