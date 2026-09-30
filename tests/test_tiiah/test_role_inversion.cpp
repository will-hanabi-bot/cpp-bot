// Role inversion (tiiah/CONVENTION.md §1c, v18.2.0; v18.3.0).
//
// A clue to the giver's Cathy is reactor0's ordinary reactive, with Bob reacting,
// unless the roles are inverted: Bob holds a STANDING play -- a clued card called
// to play, whatever its touches still allow -- and Cathy does not. Then the clue
// to Cathy is stable. Since v18.3.0 the same position makes a clue to Bob a
// reverse reactive, with Cathy reacting (human diagnostic 2013726 T30). Replay
// 2013645 T11.
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

// The same position is the reverse-reactive one (v18.3.0): Bob's colour-called
// card is a standing play, so a clue to Bob is a reverse reactive, with Cathy
// reacting and Bob receiving.
TEST(TiiahRoleInversion, AClueToBobIsAReverseReactive) {
  Game g = setup(inversion_opts());
  call_by_colour(g, order_at(g, TestPlayer::BOB, 1), Identity{3, 3});

  EXPECT_TRUE(reactor::variants::reverse_reactive_position(g, 0));
  EXPECT_FALSE(reactor::variants::inverted_stable(g, 0, 1))
      << "role inversion only ever speaks about a clue to Cathy";
  EXPECT_TRUE(reactor0::dispatch_is_reactive(
      g, rank_clue(TestPlayer::ALICE, TestPlayer::BOB, 4)));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  // Read as a reactive, not as the stable clue it was until v18.3.0. This Bob has
  // no second playable for the walk to find, so the reading is a MISTAKE; what the
  // test pins is the dispatch.
  const auto interp = interp_of(g);
  EXPECT_TRUE(interp == ClueInterp::REACTIVE || interp == ClueInterp::MISTAKE);
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().reacter, 2) << "Cathy reacts";
  EXPECT_EQ(g.waiting.front().receiver, 1) << "Bob receives";
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

// An UNCLUED call counts once it is settled (v18.10.0) -- a receiver's call whose
// reaction has been played, say -- because every seat stamps it alike. Replay
// 2013963 T10: will-bot67's o17 held such a call, and yagami's reverse-reactive
// finesse to will-bot67 read as a MISTAKE while only clued calls counted.
TEST(TiiahRoleInversion, ASettledUncluedCallPutsTheTableInTheReversePosition) {
  Game g = setup(inversion_opts());
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.with_meta(o, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = false;
  });
  ASSERT_FALSE(g.state.deck[o].clued) << "guard: nothing touched it";

  EXPECT_TRUE(reactor::variants::has_standing_play(g, 1));
  EXPECT_TRUE(reactor::variants::reverse_reactive_position(g, 0));
}

// A PENDING reaction call (urgent) does not: it is the kind a seat can stamp
// differently from the others.
TEST(TiiahRoleInversion, APendingUncluedCallDoesNot) {
  Game g = setup(inversion_opts());
  const int o = order_at(g, TestPlayer::BOB, 1);
  g.with_meta(o, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
  });

  EXPECT_FALSE(reactor::variants::has_standing_play(g, 1));
  EXPECT_FALSE(reactor::variants::reverse_reactive_position(g, 0));
}
