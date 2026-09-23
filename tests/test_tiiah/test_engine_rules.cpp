// The engine half of Throw It in a Hole: what happens when the server refuses
// to say what a card was.
//
// These key on `Variant::throw_it_in_a_hole`, NOT on `Game::convention`, and
// the fixtures are deliberately left under the default convention to prove it:
// a 4+ player TIIAH table falls back to reactor and still needs its stacks to
// be right.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

SetupOptions plain_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "g1", "b1", "p1"},
      {"r2", "y2", "g2", "b2", "p2"},
  };
  opts.starting = TestPlayer::BOB;
  return opts;
}

}  // namespace

// We watched the card sit in Bob's hand, so we know what went into the hole
// even though the wire does not say. Our stacks advance.
TEST(TiiahEngine, APartnersHiddenPlayAdvancesOurStacks) {
  Game g = setup(plain_opts());
  ASSERT_EQ(g.state.play_stacks[0], 0);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "r3");

  EXPECT_EQ(g.state.play_stacks[0], 1) << "the r1 landed, so red is on 1";
  EXPECT_EQ(g.state.strikes, 0);
}

// The control that pins the gate: the identical action in an ordinary variant
// is still the engine's "unknown card", and still leaves the stacks alone.
TEST(TiiahEngine, AHiddenPlayOutsideTheVariantChangesNothing) {
  SetupOptions opts = plain_opts();
  opts.variant_name = "No Variant";
  opts.hands[0] = {"xx", "xx", "xx", "xx", "xx"};
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "r3");

  EXPECT_EQ(g.state.play_stacks[0], 0)
      << "outside TIIAH an identity-free action stays unresolved";
}

// Our OWN play is the genuine gap: `deck[order].id()` is nullopt for our seat,
// so there is nothing to resolve. The convention calls this a superposition and
// tracks it in a later version; here it must simply not crash or guess.
TEST(TiiahEngine, OurOwnHiddenPlayLeavesTheStacksAlone) {
  SetupOptions opts = plain_opts();
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);

  for (int stack : g.state.play_stacks) EXPECT_EQ(stack, 0);
  EXPECT_EQ(g.state.strikes, 0);
  EXPECT_EQ(static_cast<int>(g.state.hands[0].size()), 5) << "and we redrew";
}

// A play that did not land is a strike, and nobody is told. We can see the card,
// so our own belief is right even though the team's need not be.
TEST(TiiahEngine, AHiddenMisplayStrikes) {
  SetupOptions opts = plain_opts();
  opts.hands[1] = {"r3", "y1", "g1", "b1", "p1"};  // r3 on an empty red stack
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "r4");

  EXPECT_EQ(g.state.strikes, 1);
  EXPECT_EQ(g.state.play_stacks[0], 0) << "and nothing landed";
}

// A 5 into the hole returns no clue token. The refund is skipped in
// `State::with_play`, so a 5 DISCARDED still refunds — that pile is visible and
// the rule is about the hole.
TEST(TiiahEngine, AFiveReturnsNoClueToken) {
  SetupOptions opts = plain_opts();
  opts.hands[1] = {"r5", "y1", "g1", "b1", "p1"};
  opts.play_stacks = std::vector<int>{4, 0, 0, 0, 0};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  EXPECT_EQ(g.state.play_stacks[0], 5) << "guard: the r5 did land";
  EXPECT_EQ(g.state.clue_tokens, 4) << "but the hole pays nothing for it";
}

TEST(TiiahEngine, AFiveStillPaysOutsideTheVariant) {
  SetupOptions opts = plain_opts();
  opts.variant_name = "No Variant";
  opts.hands[1] = {"r5", "y1", "g1", "b1", "p1"};
  opts.play_stacks = std::vector<int>{4, 0, 0, 0, 0};
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));

  g.catchup = true;
  g.handle_action(PlayAction{1, order_at(g, TestPlayer::BOB, 1), 0, 5});

  EXPECT_EQ(g.state.clue_tokens, 5);
}

// The inverted suits inherit the mirror. An orange reaches its stack on the
// DISCARD button — a chuck — so a hidden action that reached the hole is that
// button, whatever the wire called it.
TEST(TiiahEngine, AHiddenChuckAdvancesTheOrangeStack) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Orange (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"o1", "r1", "g1", "b1", "y1"},
      {"o2", "r2", "g2", "b2", "y2"},
  };
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  const int orange = 4;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted);

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y3");

  EXPECT_EQ(g.state.play_stacks[orange], 1) << "the chuck stacked the o1";
  EXPECT_EQ(g.state.strikes, 0);
  EXPECT_TRUE(g.state.discard_stacks[orange][0].empty())
      << "and it did not reach the discard pile";
}

// ...while a PITCH of an orange — pressing Play on it — reaches the discard
// pile, which is visible even here, and refunds a token like any discard.
TEST(TiiahEngine, AnOrangePitchReachesTheVisiblePile) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole & Orange (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"o1", "r1", "g1", "b1", "y1"},
      {"o2", "r2", "g2", "b2", "y2"},
  };
  opts.clue_tokens = 4;
  opts.starting = TestPlayer::BOB;
  Game g = setup(std::move(opts));
  const int orange = 4;

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/false, "y3");

  EXPECT_EQ(g.state.play_stacks[orange], 0);
  EXPECT_EQ(static_cast<int>(g.state.discard_stacks[orange][0].size()), 1)
      << "a pitch is a discard, and the pile is not the hole";
  EXPECT_EQ(g.state.clue_tokens, 5);
}
