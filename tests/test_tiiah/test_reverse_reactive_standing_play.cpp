// What puts the table in the reverse-reactive position, and what confirms the
// reverse reactive (tiiah/CONVENTION.md §1c, v20.3.0; replay 2018428).
//
// A STANDING play is a clued or settled call, or a SURE play: a card every identity
// of whose touches is playable on the shared view, none of which the hole could
// already hold. There is no good touch on an ancillary touched card, so a clued 1
// that was never called is no standing play while some 1 may be in the hole. A card
// need not be called, nor its identity a singleton: a yellow card filled in as a 1
// is a sure play, and so is a `{y1,g1}` the hole cannot hold.
//
// And the reverse reactive is confirmed only by a play of one of the standing plays
// that MADE the position. A card the reverse clue itself made playable does not.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/variants/hole.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

constexpr int kRed = 0, kYellow = 1, kGreen = 2;

// Alice gives. Bob holds nothing called; Cathy holds nothing playable or called.
Game position() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "r3", "y4", "g4", "b4"},
      {"r4", "y3", "g3", "b3", "p3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return setup(std::move(opts));
}

// Bob's slot 1, clued and touched down to `ids`, and never called.
int touch_bobs_first(Game& g, IdentitySet ids) {
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.state.deck[o].clued = true;
  g.with_thought(o, [&ids](const Thought& t) {
    Thought out = t;
    out.possible = ids;
    out.inferred = ids;
    return out;
  });
  return o;
}

// An unsettled hole card the team reads as `ids`: one of the undealt orders, which
// no hand holds.
void hole_card(Game& g, IdentitySet ids) {
  const int o = static_cast<int>(g.meta.size()) - 1;
  g.with_meta(o, [&ids](ConvData& m) {
    m.superposition = ids;
    m.shared_left = IdentitySet::empty();
  });
}

IdentitySet set_of(std::initializer_list<Identity> ids) {
  IdentitySet out = IdentitySet::empty();
  for (Identity i : ids) out = out.add(i);
  return out;
}

}  // namespace

// Replay 2018428's o5: a clued 1, never called, touched as any 1. Some 1 may be in
// the hole, so it is no standing play and the table is not in the reverse position.
TEST(TiiahStandingPlay, AnUncalledOneIsNotAStandingPlayWhileTheHoleMayHoldA1) {
  Game g = position();
  const int o = touch_bobs_first(
      g, set_of({{kRed, 1}, {kYellow, 1}, {kGreen, 1}, {3, 1}, {4, 1}}));
  hole_card(g, set_of({{kRed, 1}, {kYellow, 1}}));

  ASSERT_TRUE(reactor::variants::has_known_play(g, 1))
      << "guard: every 1 is playable on the shared view";
  EXPECT_TRUE(reactor::variants::possibly_in_the_hole(g, Identity{kRed, 1}));
  EXPECT_FALSE(reactor::variants::is_standing_play(g, o));
  EXPECT_FALSE(reactor::variants::reverse_reactive_position(g, 0))
      << "a clue to Bob is stable, not a reverse reactive";
}

// The user's example: a yellow card filled in as a 1, never called. Its one
// identity is playable and nothing in the hole could be it: a standing play.
TEST(TiiahStandingPlay, AKnownY1IsAStandingPlayUncalled) {
  Game g = position();
  const int o = touch_bobs_first(g, IdentitySet::single(Identity{kYellow, 1}));

  ASSERT_NE(g.meta[o].status, CardStatus::CALLED_TO_PLAY) << "guard: never called";
  EXPECT_TRUE(reactor::variants::is_standing_play(g, o));
  EXPECT_TRUE(reactor::variants::reverse_reactive_position(g, 0));
}

// Nor need it be a singleton: every identity playable, none in the hole.
TEST(TiiahStandingPlay, AnAllPlayableSetIsAStandingPlay) {
  Game g = position();
  const int o = touch_bobs_first(g, set_of({{kYellow, 1}, {kGreen, 1}}));
  hole_card(g, set_of({{kRed, 1}, {3, 1}}));  // nothing the card could be

  EXPECT_TRUE(reactor::variants::is_standing_play(g, o));
  EXPECT_TRUE(reactor::variants::reverse_reactive_position(g, 0));
}

// The same known y1 is no standing play while a hole card could be the y1.
TEST(TiiahStandingPlay, AKnownY1IsNotAStandingPlayWhileTheHoleMayHoldIt) {
  Game g = position();
  const int o = touch_bobs_first(g, IdentitySet::single(Identity{kYellow, 1}));
  hole_card(g, set_of({{kYellow, 1}, {kGreen, 1}}));

  EXPECT_FALSE(reactor::variants::is_standing_play(g, o));
  EXPECT_FALSE(reactor::variants::reverse_reactive_position(g, 0));
}

// A clued call stays a standing play whatever its touches allow.
TEST(TiiahStandingPlay, ACluedCallIsAStandingPlay) {
  Game g = position();
  const int o = touch_bobs_first(
      g, set_of({{kRed, 1}, {kYellow, 1}, {kGreen, 1}, {3, 1}, {4, 1}}));
  hole_card(g, set_of({{kRed, 1}, {kYellow, 1}}));
  g.with_meta(o, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });

  EXPECT_TRUE(reactor::variants::is_standing_play(g, o));
  EXPECT_TRUE(reactor::variants::reverse_reactive_position(g, 0));
}

// --- the confirmation ---------------------------------------------------------

namespace {

// Bob's known r1 makes the position. Alice's 1 to Bob re-touches it and newly
// touches his y1 (slot 2), which the clue itself makes a sure play.
Game reversed_by_a_one() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y1", "y4", "g4", "b4"},
      {"r2", "y3", "g3", "b3", "p3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");
  g = take_turn(std::move(g), "Alice clues 1 to Bob");
  return g;
}

}  // namespace

// Replay 2018428 T17: Bob plays the card the reverse clue newly touched, not the
// standing play that made the position. The reverse reactive is withdrawn.
TEST(TiiahStandingPlay, PlayingTheNewlyCluedCardWithdrawsTheReverseReactive) {
  Game g = reversed_by_a_one();
  ASSERT_FALSE(g.waiting.empty()) << "guard: read as a reverse reactive";
  ASSERT_TRUE(reactor::variants::is_standing_play(g, order_at(g, TestPlayer::BOB, 2)))
      << "guard: the clue made Bob's y1 a sure play";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/2, /*reached_the_hole=*/true,
                    "p4");

  EXPECT_TRUE(g.waiting.empty()) << "an unrelated play withdraws it";
}

// The control: Bob plays the r1 that made the position, and it stands.
TEST(TiiahStandingPlay, PlayingThePositionsStandingPlayConfirms) {
  Game g = reversed_by_a_one();
  ASSERT_FALSE(g.waiting.empty()) << "guard: read as a reverse reactive";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1, /*reached_the_hole=*/true,
                    "p4");

  EXPECT_FALSE(g.waiting.empty());
}
