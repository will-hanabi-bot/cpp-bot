// Forced endgame Rule 4 — "two criticals, dead partner".
//
// Three players, two cards left, the next seat's hand entirely trash, and CP
// holding two cards every reading of which is playable AND critical. CP must
// play: they get two turns from here if they act now and one if they stall, the
// partner cannot play whatever he is told, so the deck drains on schedule and
// there is no stall that buys the skipped turn back.
//
// Replay 1974303 T44 is the case (tests/test_reactor0/test_endgame/).
//
// Rule 4 is the ONLY rule that fires at `cards_left == 2`, which is what makes
// these fixtures readable: nothing else can answer, so `forced_endgame_action`
// returning a value is Rule 4 and nothing else. That is also why the deck-size
// negative below is asserted at 3 rather than at 0 or 1 — those belong to Rules
// 0/0b/0c/2/3 by design, and a negative there would be asserting their
// behaviour rather than this one's.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "hanabi/endgame/forced_endgame.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

Game with_cards_left(Game g, int n) {
  g.state.cards_left = n;
  return g;
}

// Every stack on 4, so every 5 is playable and (being the only copy) critical,
// and everything below rank 5 is trash. Alice is CP and the observer; Bob's
// hand is five trash cards; Cathy holds nothing that matters.
//
// Alice's two criticals are her slots 1 and 2 — the caller `seed`s them, since
// `certain_plays` reads CP's OWN view and an unclued card admits everything.
SetupOptions base() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 4, 4, 4};
  opts.hands = {
      {"r5", "b5", "r1", "y1", "g1"},   // Alice (CP): two 5s, then trash
      {"r2", "y2", "g2", "b2", "p2"},   // Bob: all trash
      {"r3", "y3", "g3", "b3", "p3"},   // Cathy: all trash too, but unread
  };
  return opts;
}

int alice_slot(const Game& g, int slot) {
  return g.state.hands[static_cast<int>(TestPlayer::ALICE)][slot - 1];
}

// Seed what CP knows about one of their own cards, then sync the per-player
// view onto it.
//
// The sync is the load-bearing half: `certain_plays` reads `game.me()`, and the
// harness's `pre_clue` / `fully_known` narrow `common` ONLY -- so a fixture
// built with those leaves CP's own view admitting everything and the rule
// silent for a reason that has nothing to do with what is being tested. This is
// the same idiom tests/test_endgame/test_certain_play.cpp uses, for the same
// reason.
void seed(Game& g, int slot, std::initializer_list<Identity> ids) {
  IdentitySet set;
  for (Identity i : ids) set = set.add(i);
  g.with_thought(alice_slot(g, slot), [&](const Thought& t) {
    Thought out = t;
    out.inferred = set;
    return out;
  });
  g.players[0] = g.common;
}

const Identity kR4{0, 4};
const Identity kR5{0, 5};
const Identity kY5{1, 5};
const Identity kG5{2, 5};
const Identity kB5{3, 5};
const Identity kP5{4, 5};

}  // namespace

// --- the fire case ---------------------------------------------------------

TEST(ForcedEndgameRuleFour, FiresWithTwoCriticalsAndADeadPartner) {
  Game g = setup(base());
  seed(g, 1, {kR5});
  seed(g, 2, {kB5});
  g = with_cards_left(std::move(g), 2);

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value())
      << "two playable criticals, a partner who cannot play, two cards left";
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced))
      << "the rule forces a PLAY, never a stall";
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, alice_slot(g, 1))
      << "neither 5 has a successor to unblock, so the tiebreak falls through "
         "to hand order";
}

// The clause that puts this outside Rule 2, and the reason 1974303 needed a new
// rule: the second critical is read as SEVERAL identities. Every one of them is
// playable and critical, which is the property that matters — Rule 2's
// singleton test would throw this card away.
TEST(ForcedEndgameRuleFour, CountsACriticalThatIsNotPinnedToOneIdentity) {
  Game g = setup(base());
  seed(g, 1, {kR5});
  seed(g, 2, {kR5, kY5, kG5, kB5, kP5});  // "a 5", and every 5 is playable here
  g = with_cards_left(std::move(g), 2);

  const IdentitySet live = g.me().thoughts[alice_slot(g, 2)].possibilities();
  ASSERT_GT(live.length(), 1u) << "guard: slot 2 is NOT pinned to one identity";
  ASSERT_TRUE(live.forall([&](Identity i) {
    return g.state.is_playable(i) && g.state.is_critical(i);
  })) << "guard: but every reading of it is playable and critical";

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value()) << "an unpinned critical still counts";
  EXPECT_TRUE(std::holds_alternative<PerformPlay>(*forced));
}

// The tiebreak, shared with Rule 2: prefer the critical whose successor another
// player holds, so the play unblocks them. Here Alice's slot 1 is a b5 with
// nothing behind it and slot 2 an r4 whose r5 sits in Cathy's hand, so the rule
// must reach past hand order for slot 2.
TEST(ForcedEndgameRuleFour, PrefersTheCriticalThatUnblocksAPartner) {
  SetupOptions opts = base();
  opts.play_stacks = {3, 4, 4, 4, 4};        // red on 3, so the r4 is playable
  opts.discarded = {"r4"};                    // ...and the remaining r4 critical
  opts.hands[0] = {"b5", "r4", "r1", "y1", "g1"};
  opts.hands[2] = {"r5", "y3", "g3", "b3", "p3"};  // Cathy holds the r5
  Game g = setup(std::move(opts));
  seed(g, 1, {kB5});
  seed(g, 2, {kR4});
  g = with_cards_left(std::move(g), 2);

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value());
  ASSERT_TRUE(std::holds_alternative<PerformPlay>(*forced));
  EXPECT_EQ(std::get<PerformPlay>(*forced).target, alice_slot(g, 2))
      << "play the r4: it unblocks Cathy's r5, while the b5 unblocks nothing";
}

// --- one negative per clause ----------------------------------------------

// Condition 3. Bob's slot 5 is a playable p5 rather than trash, so he is not a
// dead partner and the turn accounting the rule rests on no longer holds.
TEST(ForcedEndgameRuleFour, SilentWhenThePartnerHoldsSomethingPlayable) {
  SetupOptions opts = base();
  opts.hands[1] = {"r2", "y2", "g2", "b2", "p5"};
  Game g = setup(std::move(opts));
  seed(g, 1, {kR5});
  seed(g, 2, {kB5});
  g = with_cards_left(std::move(g), 2);

  // Guard that this negative is about the PARTNER and nothing else: the same
  // position with Bob's slot 5 back to trash does fire.
  Game fires = setup(base());
  seed(fires, 1, {kR5});
  seed(fires, 2, {kB5});
  fires = with_cards_left(std::move(fires), 2);
  ASSERT_TRUE(hanabi::endgame::forced_endgame_action(fires).has_value());

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// Condition 2, first half. Only ONE of Alice's cards is a certain critical
// play; slot 2 is left unclued, so her own view of it admits everything.
TEST(ForcedEndgameRuleFour, SilentWithOnlyOneCritical) {
  Game g = setup(base());
  seed(g, 1, {kR5});  // slot 2 left admitting everything
  g = with_cards_left(std::move(g), 2);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// Condition 2, second half. Both cards are critical, but the second is not
// PLAYABLE — a b5 with blue on 3. Two criticals in hand are no reason to hurry
// if only one of them can be cashed.
TEST(ForcedEndgameRuleFour, SilentWhenTheSecondCriticalCannotBePlayed) {
  SetupOptions opts = base();
  opts.play_stacks = {4, 4, 4, 3, 4};  // blue on 3, so the b5 is not playable
  Game g = setup(std::move(opts));
  seed(g, 1, {kR5});
  seed(g, 2, {kB5});
  g = with_cards_left(std::move(g), 2);

  ASSERT_TRUE(g.state.is_critical(Identity{3, 5})) << "guard: still critical";
  ASSERT_FALSE(g.state.is_playable(Identity{3, 5})) << "guard: but not playable";
  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// Condition 1. Three cards left is a different position — Alice is not yet down
// to exactly the turns she needs, and the solver is trusted to see it.
TEST(ForcedEndgameRuleFour, SilentWithThreeCardsLeft) {
  Game g = setup(base());
  seed(g, 1, {kR5});
  seed(g, 2, {kB5});
  g = with_cards_left(std::move(g), 3);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}

// Scope. The two-turns-for-Alice arithmetic is stated for three seats, and a
// forced rule short-circuits the solver, so it is not asserted at counts no
// replay exercises.
TEST(ForcedEndgameRuleFour, SilentAtFourPlayers) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 4, 4, 4};
  opts.hands = {
      {"r5", "b5", "r1", "y1"},
      {"r2", "y2", "g2", "b2"},
      {"r3", "y3", "g3", "b3"},
      {"r4", "y4", "g4", "b4"},
  };
  Game g = setup(std::move(opts));
  seed(g, 1, {kR5});
  seed(g, 2, {kB5});
  g = with_cards_left(std::move(g), 2);

  ASSERT_EQ(g.state.num_players, 4) << "guard: four seats";
  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value());
}
