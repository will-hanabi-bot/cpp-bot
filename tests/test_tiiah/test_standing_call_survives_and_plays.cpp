// A standing call in Throw It in a Hole (tiiah/CONVENTION.md §1c and §2k, v20.11.0,
// replay 2018874; the user's rulings).
//
// 1. A newer call to its right does not erase it. reactor0's call invariant rule 1
//    erases every older play call to the left of the newest one, since a later clue
//    would not have pointed past a card still playable. In this variant the target
//    walk passes over a standing call because it is already gotten, so the rule is
//    off here. reactor0 itself still erases (the No Variant control below, and
//    `Reactor0CallInvariants.NewerClueCallingAnOlderSlotErasesTheEarlierCall`).
// 2. Our own call whose reading plays on the SHARED view is a known play, though our
//    private stacks lag that view.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/reactor0/call_invariants.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Bob holds the b1 in slot 1 and the r1 in slot 5. The b1 was called at turn 1,
// the r1 at turn 3: the newer call lands to the right of the standing one.
Game two_calls(bool tiiah) {
  SetupOptions opts;
  opts.variant_name = tiiah ? "Throw It in a Hole (5 Suits)" : "No Variant";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b1", "g4", "y3", "p4", "r1"},
      {"b2", "y4", "g3", "p3", "r4"},
  };
  opts.clue_tokens = 5;
  if (tiiah) use_tiiah(opts);
  Game g = setup(std::move(opts));
  auto call = [&g](int order, Identity id, int turn) {
    g.narrow_thought(order, IdentitySet::empty().add(id));
    g.with_meta(order, [turn](ConvData& m) {
      m.status = CardStatus::CALLED_TO_PLAY;
      m.signal_turn = turn;
    });
  };
  call(order_at(g, TestPlayer::BOB, 1), g.state.expand_short("b1"), 1);
  call(order_at(g, TestPlayer::BOB, 5), g.state.expand_short("r1"), 3);
  hanabi::reactor0::enforce_call_invariants(g);
  return g;
}

// We hold a card clued blue and 5, and called to play. Blue is on 3 on every view,
// or (`shared_ahead`) on 4 on the shared view while our own stacks still have it on
// 3. With `own_hole`, we first threw an unknown card into the hole, which could have
// been the b4 the shared view counts; without it, nothing of ours explains the gap.
Game own_call(bool shared_ahead, bool own_hole) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g4", "y3", "p4", "r2"},
      {"b2", "y4", "g3", "p3", "r4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 3, 0};
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  if (own_hole) {
    g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/5, /*reached_the_hole=*/true);
  }
  if (shared_ahead) {
    g.with_state([](State& st) { st = st.with_common_floor({0, 0, 0, 4, 0}); });
  }
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"blue", "5"});
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  g.with_meta(ours, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.signal_turn = 1;
  });
  return g;
}

}  // namespace

TEST(TiiahStandingCall, ANewerCallToItsRightLeavesItStanding) {
  Game g = two_calls(/*tiiah=*/true);
  ASSERT_EQ(g.meta[order_at(g, TestPlayer::BOB, 5)].status, CardStatus::CALLED_TO_PLAY)
      << "guard: the newer call";
  EXPECT_EQ(g.meta[order_at(g, TestPlayer::BOB, 1)].status, CardStatus::CALLED_TO_PLAY);
}

// The control: reactor0 without the hole still erases the older call.
TEST(TiiahStandingCall, OutsideTheHoleRuleOneStillErases) {
  Game g = two_calls(/*tiiah=*/false);
  ASSERT_EQ(g.meta[order_at(g, TestPlayer::BOB, 5)].status, CardStatus::CALLED_TO_PLAY)
      << "guard: the newer call";
  EXPECT_NE(g.meta[order_at(g, TestPlayer::BOB, 1)].status, CardStatus::CALLED_TO_PLAY);
}

TEST(TiiahStandingCall, ACallPlayableOnTheSharedViewIsAKnownPlay) {
  Game g = own_call(/*shared_ahead=*/true, /*own_hole=*/true);
  ASSERT_EQ(g.state.play_stacks[3], 3) << "guard: our own stacks lag";
  EXPECT_TRUE(g.common.order_playable(g, order_at(g, TestPlayer::ALICE, 1)));
}

// The controls. The b5 plays on no view, so it is no known play.
TEST(TiiahStandingCall, ACallUnplayableOnTheSharedViewIsNot) {
  Game g = own_call(/*shared_ahead=*/false, /*own_hole=*/true);
  EXPECT_FALSE(g.common.order_playable(g, order_at(g, TestPlayer::ALICE, 1)));
}

// Nor when the shared view is ahead and none of our own hole cards accounts for it:
// then our view knows better. Self-play seed 21 T58 (the first cut): red on 1
// shared, on 0 for the seat and in truth, and its `{r2}` call struck.
TEST(TiiahStandingCall, SharedAheadWithoutOurHoleCardIsNot) {
  Game g = own_call(/*shared_ahead=*/true, /*own_hole=*/false);
  ASSERT_EQ(g.state.play_stacks[3], 3) << "guard: our own stacks lag";
  EXPECT_FALSE(g.common.order_playable(g, order_at(g, TestPlayer::ALICE, 1)));
}

// ...but not when our own view is AHEAD of the shared one. We watched the b5 go in
// (blue on 5 for us), the team has blue on 4: our called b5 is a dupe, and playing
// it would strike. Self-play seed 289 T5 (v20.11.0's first cut): a seat that knew
// red was on 1 played its `{r1}` call because the shared view still had red on 0.
TEST(TiiahStandingCall, ACallOurOwnViewKnowsIsTrashIsNot) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g4", "y3", "p4", "r2"},
      {"b2", "y4", "g3", "p3", "r4"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 5, 0};
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g.with_state([](State& st) { st.common_play_stacks = {0, 0, 0, 4, 0}; });
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"blue", "5"});
  const int ours = order_at(g, TestPlayer::ALICE, 1);
  g.with_meta(ours, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.signal_turn = 1;
  });
  ASSERT_TRUE(g.state.shared_view().is_playable(g.state.expand_short("b5")))
      << "guard: the shared view still has blue on 4";
  EXPECT_FALSE(g.common.order_playable(g, ours));
}
