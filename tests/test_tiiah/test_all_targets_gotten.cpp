// Every target gotten (tiiah/CONVENTION.md §1c, v18.5.0).
//
// The target walk skips a card already called to play: it is gotten, and a new
// reactive has to get something new. But when every playable and finesse target
// in the receiver's hand is already gotten, the walk runs again as if nothing
// were, and takes the leftmost one -- as outside TIIAH. Human diagnostic 2013726
// T38 (v18_human_vs_bot_diagnostics/2013726.md): a Brown to black gets blue's n5
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

// Cathy's b1 is her only target and it is already gotten, so the walk takes it
// anyway: the 3 is an ordinary reactive, and Bob's y1 is called as the reaction.
TEST(TiiahAllTargetsGotten, TheWalkFallsBackToTheLeftmostCalledTarget) {
  Game g = setup(gotten_opts("p4"));
  const int b1 = order_at(g, TestPlayer::CATHY, 4);
  const int y1 = order_at(g, TestPlayer::BOB, 4);
  call_b1(g, b1);

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, 1) << "Bob reacts";
  EXPECT_EQ(g.waiting.front().receiver_target_order, b1)
      << "the gotten b1 is the target, there being nothing else to get";
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
