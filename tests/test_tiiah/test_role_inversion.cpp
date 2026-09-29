// Role inversion (tiiah/CONVENTION.md §1c, v18.2.0).
//
// A clue to the giver's Cathy is reactor0's ordinary reactive, with Bob reacting,
// unless the roles are inverted: Bob holds a STANDING play -- any card called to
// play, whatever its touches still allow -- and Cathy does not. Then the clue to
// Cathy is stable. That is all it changes. Whether a clue to Bob is a reverse
// reactive still asks for a touch-known play (v18.0.0), so a called card whose
// touches allow unplayable identities makes the clue to Bob stable as well.
// Replay 2013645 T11.
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/hole.h"
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

// Alice gives. Bob's slot 1 is a b3 with blue on 2; Cathy's hand holds nothing
// playable.
SetupOptions inversion_opts() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b3", "r3", "y4", "g4", "p4"},
      {"r2", "y3", "g3", "b5", "p3"},
  };
  opts.play_stacks = std::vector<int>{0, 0, 0, 2, 0};
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

// Stamp a call the way a stable Blue does: touched as any blue, read as `id`.
void call_by_colour(Game& g, int order, Identity id) {
  IdentitySet blues = IdentitySet::empty();
  for (int r = 1; r <= 5; ++r) blues = blues.add(Identity{id.suit_index, r});
  g.state.deck[order].clued = true;
  g.with_thought(order, [&blues, id](const Thought& t) {
    Thought out = t;
    out.possible = blues;
    out.inferred = IdentitySet::single(id);
    return out;
  });
  g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
}

ClueAction rank_clue(TestPlayer giver, TestPlayer target, int rank) {
  return ClueAction(static_cast<int>(giver), static_cast<int>(target), {},
                    BaseClue(ClueKind::RANK, rank));
}

}  // namespace

// Bob's called b3 is a standing play, though its touches still allow b1-b5, and
// Cathy holds none: a clue to Cathy is stable.
TEST(TiiahRoleInversion, AStandingCallOnBobKeepsAClueToCathyStable) {
  Game g = setup(inversion_opts());
  call_by_colour(g, order_at(g, TestPlayer::BOB, 1), Identity{3, 3});

  ASSERT_FALSE(reactor::variants::has_known_play(g, 1))
      << "guard: the touches alone do not name it playable";
  EXPECT_TRUE(reactor::variants::has_standing_play(g, 1));
  EXPECT_TRUE(reactor::variants::inverted_stable(g, 0, 2));
  EXPECT_FALSE(reactor0::dispatch_is_reactive(
      g, rank_clue(TestPlayer::ALICE, TestPlayer::CATHY, 3)))
      << "the giver's side predicts the same stable reading";

  g = take_turn(std::move(g), "Alice clues 3 to Cathy");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_TRUE(g.waiting.empty()) << "nobody is asked to react";
}

// Role inversion creates no reverse reactive: the clue to Bob stays stable,
// because his call is not a touch-known play.
TEST(TiiahRoleInversion, AClueToBobStaysStable) {
  Game g = setup(inversion_opts());
  call_by_colour(g, order_at(g, TestPlayer::BOB, 1), Identity{3, 3});

  EXPECT_FALSE(reactor::variants::reverse_reactive_position(g, 0));
  EXPECT_FALSE(reactor::variants::inverted_stable(g, 0, 1))
      << "role inversion only ever speaks about a clue to Cathy";
  EXPECT_FALSE(reactor0::dispatch_is_reactive(
      g, rank_clue(TestPlayer::ALICE, TestPlayer::BOB, 4)));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_TRUE(g.waiting.empty());
}

// A two-sided test, like the reverse position: once Cathy holds a call too, the
// roles are not inverted, and a clue to her is the ordinary reactive again.
TEST(TiiahRoleInversion, ACallOnCathyTooLeavesTheOrdinaryReactive) {
  Game g = setup(inversion_opts());
  call_by_colour(g, order_at(g, TestPlayer::BOB, 1), Identity{3, 3});
  call_by_colour(g, order_at(g, TestPlayer::CATHY, 1), Identity{0, 1});

  EXPECT_TRUE(reactor::variants::has_standing_play(g, 2));
  EXPECT_FALSE(reactor::variants::inverted_stable(g, 0, 2));
  EXPECT_TRUE(reactor0::dispatch_is_reactive(
      g, rank_clue(TestPlayer::ALICE, TestPlayer::CATHY, 3)));
}
