// On the reverse arm the receiver moves first, and its standing play is not the
// reaction: it is not read as the reacter's card (tiiah/CONVENTION.md §1d, v20.16.0;
// self-play seed 270 T41).
//
// Bob holds a standing play called `{b1,b2}` (the b1). We clue Bob: a reverse
// reactive, Cathy reacting. Bob plays his standing card into the hole. Read as the
// reacter's card on the frame Cathy and we share (blue on 1 there), the bucket
// reading would settle it for the team as the b2 -- which is what happened in seed
// 270, and with the v20.16.0 settle above the shared view it then floored blue at 2.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

TEST(TiiahReverseReceiver, ItsStandingPlayIsNotReadAsTheReactersCard) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g4", "b1", "y3", "p4", "r4"},
      {"g3", "y1", "p3", "r3", "y4"},
  };
  opts.clue_tokens = 6;
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"blue"});
  const int standing = order_at(g, TestPlayer::BOB, 2);
  const IdentitySet b1_b2 =
      IdentitySet::empty().add(g.state.expand_short("b1")).add(g.state.expand_short("b2"));
  g.narrow_thought(standing, b1_b2);
  g.with_meta(standing, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.signal_turn = 1;
  });
  // The row Cathy and we share has blue on 1; the shared view still has it on 0.
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  g.with_state([cathy](State& st) { st.pairwise_play_stacks[cathy][3] = 1; });

  g = take_turn(std::move(g), "Alice clues 3 to Bob");
  ASSERT_FALSE(g.waiting.empty()) << "guard: a reverse reactive is pending";
  ASSERT_EQ(g.waiting.front().receiver, static_cast<int>(TestPlayer::BOB));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2, /*reached_the_hole=*/true,
                    "g5");

  EXPECT_NE(g.meta[standing].named_in_hole, IdentitySet::single(g.state.expand_short("b2")))
      << "the receiver's standing play is not the reacter's card";
}
