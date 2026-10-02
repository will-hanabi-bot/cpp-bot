// A watched hole play is named by sight, not by a stale common reading
// (tiiah/CONVENTION.md §1e, v19.1.0).
//
// Our common copy of a partner's call can name the wrong card: an outside seat
// reads a call between two partners on the shared floor (§1.3), and in replay
// 2015109 the ladder's narrowing to `{y1}` outlived the MISTAKE our own sight
// read the call as. When that card is then played and we can see what it was,
// our eyes name it, not the copy. There the copy booked a y1 for a y3, and the
// y3 vanished from every shared world.
//
// The stale copy is set up directly: yellow is on 1 for us and on 0 in common,
// and Cathy's y2 reads `{y1}` in common.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

TEST(TiiahHolePlayNamedBySight, AWatchedPlayIsNamedByWhatWeSawNotByAStaleCopy) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", "g4", "b4", "p4", "r3"},
      {"y2", "g3", "b3", "p3", "r5"},
  };
  opts.play_stacks = std::vector<int>{0, 1, 0, 0, 0};
  opts.starting = TestPlayer::CATHY;
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  // The y1 went in unnamed, so the shared floor never took it...
  g.state.common_play_stacks[1] = 0;
  // ...and our common copy of Cathy's call names the y1.
  const int o = order_at(g, TestPlayer::CATHY, 1);
  const Identity y1 = g.state.expand_short("y1");
  g.with_thought(o, [y1](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(y1);
    return out;
  });
  ASSERT_EQ(g.common.thoughts[o].possibilities(), IdentitySet::single(y1));

  // Cathy plays it. We see the y2 go in, and it lands.
  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1,
                    /*reached_the_hole=*/true, "g5");

  EXPECT_EQ(g.meta[o].named_in_hole, IdentitySet::single(g.state.expand_short("y2")))
      << "named by what we saw, not by our copy of the call";
  EXPECT_EQ(g.state.common_play_stacks[1], 2)
      << "a y2 landed, so the y1 under it is down too";
}
