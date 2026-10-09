// Every target gotten (tiiah/CONVENTION.md §1c, v18.5.0).
//
// The target walk skips a card already called to play: it is gotten, and a new
// reactive has to get something new. But when every playable and finesse target
// in the receiver's hand is already gotten, the walk runs again as if nothing
// were, and takes the leftmost one -- as outside TIIAH. Human diagnostic 2013726
// T38 (human_vs_bot_diagnostics/2013726.md): a Brown to black gets blue's n5
// against the g4 black was already called to play.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// Green is on 1. Cathy's only card that is playable or one away is the b1 on her
// slot 4; Bob's slot 4 is the y1 the sum rule pairs with it under a 3 (and
// his slot 3, a p1, is what an r1 on Cathy's slot 5 would pair with).
SetupOptions gotten_opts(const char* cathy_slot_5) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b4", "b5", "p1", "y1", "p3"},
      {"y4", "b3", "y5", "b1", cathy_slot_5},
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 1, 0, 0};
  opts.clue_tokens = 6;
  use_tiiah(opts);
  return opts;
}

// The same position in four suits (buckets {r,y} {g} {b}: the three-bucket rule),
// for the listed fallback test. Nothing is played. Cathy's only card that is
// playable or one away is the g1 on her slot 4; Bob's slot 4 is the y1 the sum rule
// pairs with it under a 3 (a rank clue names green, bucket 1, from yellow, bucket 0).
SetupOptions gotten_opts_4_suits(const char* cathy_slot_5) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b4", "b5", "b1", "y1", "b3"},
      {"y4", "b3", "y5", "g1", cathy_slot_5},
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  return opts;
}

// Stamp the call a stable Green would have left on Cathy's g1.
void call_g1(Game& g, int order) {
  const Identity g1{2, 1};
  g.state.deck[order].clued = true;
  g.with_thought(order, [g1](const Thought& t) {
    Thought out = t;
    out.possible = IdentitySet::single(g1);
    out.inferred = IdentitySet::single(g1);
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

// Stamp the call a stable Blue would have left on Cathy's b1.
void call_b1(Game& g, int order) {
  const Identity b1{3, 1};
  g.state.deck[order].clued = true;
  g.with_thought(order, [b1](const Thought& t) {
    Thought out = t;
    out.possible = IdentitySet::single(b1);
    out.inferred = IdentitySet::single(b1);
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

}  // namespace

// Four suits: Cathy's g1 is her only target and it is already gotten, so the walk
// takes it anyway: the 3 is an ordinary reactive, and Bob's y1 is called as the
// reaction.
TEST(TiiahAllTargetsGotten, TheWalkFallsBackToTheLeftmostCalledTarget) {
  Game g = setup(gotten_opts_4_suits("r4"));
  const int g1 = order_at(g, TestPlayer::CATHY, 4);
  const int y1 = order_at(g, TestPlayer::BOB, 4);
  call_g1(g, g1);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, 1) << "Bob reacts";
  EXPECT_EQ(g.waiting.front().receiver_target_order, g1)
      << "the gotten g1 is the target, there being nothing else to get";
  EXPECT_EQ(g.meta[y1].status, CardStatus::CALLED_TO_PLAY)
      << "Bob's slot 4, the y1, is the reaction";
}

// The control: with an uncalled target left in her hand -- an r1 on slot 5 -- the
// walk skips the gotten b1 as before and takes that.
TEST(TiiahAllTargetsGotten, AnUngottenTargetStillComesFirst) {
  Game g = setup(gotten_opts("r1"));
  const int b1 = order_at(g, TestPlayer::CATHY, 4);
  const int r1 = order_at(g, TestPlayer::CATHY, 5);
  call_b1(g, b1);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().receiver_target_order, r1);
}
