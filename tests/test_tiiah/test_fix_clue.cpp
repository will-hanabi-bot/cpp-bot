// THE FIX CLUE (tiiah/CONVENTION.md §1h, v16.20.0).
//
// A partner holds a standing call on a card the giver can see is dead — the identity
// has gone down, and the holder's own reading still admits a good identity beside it,
// so left alone they play it and strike. Any stable clue whose NET information
// narrows that card to exactly the dead identity says so, and it supersedes whatever
// the stable ladders would otherwise have made of the clue.
//
// The condition splits in two, and each half sits where it can be checked: that the
// identity is dead is COMMON knowledge, so every seat reads the clue alike; that THIS
// card is that identity is the giver's sight, and the clue is what transfers it.
//
// Replay 2010512 is the game that wanted one — will-bot69's `{y2,p1}` call on a p1
// with purple already on 2 — and is written up in §1h.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/fix.h"
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

// Yellow on 1 and purple on 2, so Bob's slot-3 `p1` is dead while the `y2` his
// reading also admits is playable. Seeded stacks are common knowledge in this
// harness, so the deadness is too — which is what the rule requires.
//
// CATHY carries a known play as well, and that is load-bearing rather than scenery:
// §1c's position flips while Bob is loaded and Cathy is not, and under the flip a
// clue to BOB is the reactive one. A fix is stable by construction, so the fixture
// has to keep both seats loaded to leave a stable clue to Bob available at all.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.play_stacks = std::vector<int>{0, 1, 0, 0, 2};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us), the giver
      {"g2", "r4", "p1", "b4", "y4"},  // Bob: slot 3 is the dead call, slot 1 a 2
      {"r1", "y3", "g3", "b3", "p3"},  // Cathy: slot 1 is her known play
  };
  opts.starting = TestPlayer::ALICE;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  return opts;
}

void pin_call(Game& g, int order, const IdentitySet& set) {
  g.with_thought(order, [&set](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    out.possible = set;
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

// Stand a call on Bob's slot 3 reading {y2,p1} — the shape a reactive leaves when
// the bucket holds two playables and one of them has since gone down — and one on
// Cathy so the dispatch stays put.
int stand_a_call(Game& g) {
  pin_call(g, order_at(g, TestPlayer::CATHY, 1),
           IdentitySet::single(Identity{0, 1}));
  const int order = order_at(g, TestPlayer::BOB, 3);
  pin_call(g, order, IdentitySet::empty().add(Identity{1, 2}).add(Identity{4, 1}));
  return order;
}

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

}  // namespace

// A rank 2 does not touch the p1 at all. The NEGATIVE is what names it.
TEST(TiiahFixClue, ANegativeTouchFixesTheCall) {
  Game g = setup(opts_for());
  const int dead = stand_a_call(g);

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_EQ(interp_of(g), ClueInterp::FIX)
      << "and it is read AS a fix, superseding the ladders";
  EXPECT_EQ(g.common.thoughts[dead].possibilities(),
            IdentitySet::single(Identity{4, 1}))
      << "the y2 is gone, so only the dead p1 is left";
  EXPECT_NE(g.meta[dead].status, CardStatus::CALLED_TO_PLAY)
      << "so the call is withdrawn and Bob no longer plays it";
}

// ...and a rank 1 does touch it, which works just as well.
TEST(TiiahFixClue, APositiveTouchFixesItToo) {
  Game g = setup(opts_for());
  const int dead = stand_a_call(g);

  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(interp_of(g), ClueInterp::FIX);
  EXPECT_EQ(g.common.thoughts[dead].possibilities(),
            IdentitySet::single(Identity{4, 1}));
  EXPECT_NE(g.meta[dead].status, CardStatus::CALLED_TO_PLAY);
}

// The control that matters most: the identity has to be DEAD in the shared view.
// Move purple back to 1 and the p1 is playable, so narrowing the call onto it is
// ordinary good news and the clue means whatever the ladders say.
TEST(TiiahFixClue, ALiveIdentityIsNoFix) {
  SetupOptions opts = opts_for();
  opts.play_stacks = std::vector<int>{0, 1, 0, 0, 0};
  Game g = setup(std::move(opts));
  const int dead = stand_a_call(g);

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::FIX)
      << "nothing is being corrected -- the call is fine";
  EXPECT_EQ(g.common.thoughts[dead].possibilities(),
            IdentitySet::single(Identity{4, 1}))
      << "the narrowing still happens; it just does not MEAN the fix";
}

// A call already down to one identity needs no fix: either it is fine, or the
// ordinary invariants have dropped it already.
TEST(TiiahFixClue, ACallAlreadyNamedIsNoFix) {
  Game g = setup(opts_for());
  pin_call(g, order_at(g, TestPlayer::CATHY, 1), IdentitySet::single(Identity{0, 1}));
  const int order = order_at(g, TestPlayer::BOB, 3);
  pin_call(g, order, IdentitySet::single(Identity{4, 1}));

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::FIX);
}

// And with no standing call there is nothing to fix, however dead the card is.
TEST(TiiahFixClue, NoCallMeansNoFix) {
  Game g = setup(opts_for());
  pin_call(g, order_at(g, TestPlayer::CATHY, 1), IdentitySet::single(Identity{0, 1}));
  const int order = order_at(g, TestPlayer::BOB, 3);
  const IdentitySet two =
      IdentitySet::empty().add(Identity{1, 2}).add(Identity{4, 1});
  g.with_thought(order, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    out.possible = two;
    return out;
  });

  g = take_turn(std::move(g), "Alice clues 2 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::FIX);
  EXPECT_EQ(dead_call_fix(g, g, static_cast<int>(TestPlayer::BOB)), std::nullopt)
      << "the predicate itself declines, which is what the dispatcher asks";
}
