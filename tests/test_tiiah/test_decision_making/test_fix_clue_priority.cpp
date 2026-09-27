// THE FIX CLUE, given (tiiah/CONVENTION.md §1h, v16.20.0).
//
// A fix joins **Precedence step 1** the way §1c's refusal does, and inside step 1 it
// ranks between rung 2 and rung 3 — which is the "between 2 and 3" the rule was given.
// Step 1 is above the pending reaction, because a fix is not an alternative to
// anything: left ungiven it is a strike on a card the team no longer needs.
//
// And it carries one thing the refusal does not: an exemption from the tier gate. A
// fix stamps nothing and satisfies no arm of `clue_tier`, so it is always LOW, and
// `clue_is_admissible` would drop every fix an OCCUPIED Alice could give — which is
// exactly the position replay 2010512 was in when the strike happened.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

void pin_call(Game& g, int order, const IdentitySet& set) {
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

// Yellow on 1, purple on 2. Bob's slot 3 is a `p1` reading `{y2,p1}`: dead, and he
// cannot tell. Cathy carries a known play so §1c's position does not flip and a stable
// clue to Bob exists at all. ALICE carries one too, which is the point of the first
// test — it makes her OCCUPIED, so every LOW candidate would otherwise be inadmissible.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{0, 1, 0, 0, 2};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us), the giver
      {"g2", "r4", "p1", "b4", "y4"},  // Bob
      {"r1", "y3", "g3", "b3", "p3"},  // Cathy
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return opts;
}

// Alice's own actionable call, which is what `requires_high_tier` reads.
void occupy_alice(Game& g) {
  pin_call(g, order_at(g, TestPlayer::ALICE, 1), IdentitySet::single(Identity{0, 1}));
}

void load_cathy(Game& g) {
  pin_call(g, order_at(g, TestPlayer::CATHY, 1), IdentitySet::single(Identity{0, 1}));
}

int stand_the_dead_call(Game& g) {
  const int order = order_at(g, TestPlayer::BOB, 3);
  pin_call(g, order, IdentitySet::empty().add(Identity{1, 2}).add(Identity{4, 1}));
  return order;
}

}  // namespace

// Alice is occupied and holds a playable card of her own, so the tier gate wants
// HIGH and a fix is LOW. She gives it anyway, and it reads as a fix.
TEST(TiiahFixCluePriority, AFixIsGivenWhileAliceIsOccupied) {
  Game g = setup(opts_for());
  occupy_alice(g);
  load_cathy(g);
  const int dead = stand_the_dead_call(g);
  ASSERT_EQ(g.meta[dead].status, CardStatus::CALLED_TO_PLAY) << "guard";

  PerformAction action;
  ASSERT_NO_THROW(action = g.take_action());
  ASSERT_FALSE(std::holds_alternative<PerformPlay>(action))
      << "her own call is the thing the tier gate says she could do instead, and a "
         "fix outranks it";
  const int target = std::visit(
      [](const auto& a) {
        if constexpr (requires { a.target; }) return a.target;
        return -1;
      },
      action);
  EXPECT_EQ(target, static_cast<int>(TestPlayer::BOB));

  // ...and what she gave really is a fix rather than a clue that happens to land
  // there: replay the chosen clue and read it back.
  const auto* clue = std::get_if<PerformRank>(&action);
  const auto* colour = std::get_if<PerformColour>(&action);
  ASSERT_TRUE(clue != nullptr || colour != nullptr);
  const std::vector<int> touched = g.state.clue_touched(
      g.state.hands[static_cast<int>(TestPlayer::BOB)],
      clue ? ClueKind::RANK : ClueKind::COLOUR,
      clue ? clue->value : colour->value);
  Game after = g.simulate(Action{ClueAction{
      static_cast<int>(TestPlayer::ALICE), static_cast<int>(TestPlayer::BOB),
      touched,
      BaseClue{clue ? ClueKind::RANK : ClueKind::COLOUR,
               clue ? clue->value : colour->value}}});
  ASSERT_FALSE(after.move_history.empty());
  const auto* interp = std::get_if<ClueInterp>(&after.move_history.back());
  ASSERT_NE(interp, nullptr);
  EXPECT_EQ(*interp, ClueInterp::FIX);
  EXPECT_NE(after.meta[dead].status, CardStatus::CALLED_TO_PLAY)
      << "and Bob stops being about to play it, which is the whole object";
}

// The control. Take the dead call away and the same position plays the card Alice
// was called to play — so it is the fix, and not the fixture, that redirected her.
TEST(TiiahFixCluePriority, WithNothingToFixSheActionsHerOwnCall) {
  Game g = setup(opts_for());
  occupy_alice(g);
  load_cathy(g);
  // Bob's slot 3 keeps its two-identity reading but carries no call.
  const int order = order_at(g, TestPlayer::BOB, 3);
  const IdentitySet two =
      IdentitySet::empty().add(Identity{1, 2}).add(Identity{4, 1});
  g.with_thought(order, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    out.possible = two;
    return out;
  });

  PerformAction action;
  ASSERT_NO_THROW(action = g.take_action());
  const auto* play = std::get_if<PerformPlay>(&action);
  ASSERT_NE(play, nullptr) << "nothing outranks her own actionable call here";
  EXPECT_EQ(play->target, order_at(g, TestPlayer::ALICE, 1));
}
