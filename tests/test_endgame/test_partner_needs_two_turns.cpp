// Forced endgame Rule 5 — "the partner needs two turns".
//
// Three players, ONE card left, a clue available, and the NEXT seat holding two
// cards CP can see are both critical and playable. Then CP must STALL.
//
// The arithmetic, counting from CP's turn:
//
//   CP acts   -> CP draws the last card. Final round is Bob, Cathy, CP, so Bob
//                gets ONE turn and only one of his two criticals is cashed.
//   CP stalls -> Bob acts and draws. Final round is Cathy, CP, Bob, so Bob gets
//                a turn now AND one at the end, and both are cashed.
//
// So the stall is worth a point, and it costs CP nothing: CP still has a final
// round turn for whatever they were going to do. Replay 1974119 T53 is the case
// (tests/test_reactor0/test_endgame/).
//
// The fixtures below are PLAIN "No Variant" on purpose. The replay that found
// this is an Alternating Clues game, and the report read it as forced-endgame
// losing a priority fight with an urgent reactive call -- so the point of
// testing it here is that the rule and that priority are not
// Alternating-Clues-specific.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
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

// Every stack on 4, so every 5 is playable and, being the only copy, critical.
// BOB is the one holding two of them; Alice holds nothing that matters, which
// is what keeps Rules 0/0b/0c/2/3 quiet and leaves the answer to this rule.
SetupOptions base() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {4, 4, 4, 4, 4};
  opts.clue_tokens = 2;
  opts.hands = {
      {"r1", "y1", "g1", "b1", "p1"},   // Alice (CP): all trash
      {"r5", "y2", "g2", "b5", "p2"},   // Bob: TWO playable criticals
      {"r3", "y3", "g3", "b3", "p3"},   // Cathy: all trash
  };
  return opts;
}

bool action_is_clue(const PerformAction& a) {
  return std::holds_alternative<PerformColour>(a) ||
         std::holds_alternative<PerformRank>(a);
}

}  // namespace

TEST(ForcedEndgameRuleFive, FiresWhenTheNextSeatHoldsTwoPlayableCriticals) {
  Game g = with_cards_left(setup(base()), 1);
  const State& s = g.state;
  ASSERT_TRUE(s.is_critical(Identity{0, 5}) && s.is_playable(Identity{0, 5}))
      << "guard: Bob's r5 is the last copy and plays now";
  ASSERT_TRUE(s.is_critical(Identity{3, 5}) && s.is_playable(Identity{3, 5}))
      << "guard: and so is his b5";

  auto forced = hanabi::endgame::forced_endgame_action(g);
  ASSERT_TRUE(forced.has_value())
      << "Bob needs two turns and can only have them if Alice does not draw";
  EXPECT_TRUE(action_is_clue(*forced))
      << "the rule forces a STALL -- drawing the last card is the whole thing "
         "it exists to prevent";
}

// One critical is not enough: Bob cashes it in his single final-round turn, so
// the stall buys nothing and the turn is better spent.
TEST(ForcedEndgameRuleFive, DeclinesWithOnlyOneCritical) {
  SetupOptions opts = base();
  opts.hands[1] = {"r5", "y2", "g2", "b2", "p2"};
  Game g = with_cards_left(setup(std::move(opts)), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "one critical needs one turn, which Bob gets either way";
}

// The rule is about the NEXT seat, not about criticals in general. Cathy gets a
// turn on either schedule, so nothing is bought by declining to draw.
//
// Note what this negative does NOT test: "critical but not yet playable". That
// one cannot be isolated here -- making a critical unplayable means lowering its
// stack below 4, which wakes the 5-lockout, and the lockout answers with a clue
// too. The playability clause is covered by the replay instead.
TEST(ForcedEndgameRuleFive, DeclinesWhenTheCriticalsAreNotInTheNextSeatsHand) {
  SetupOptions opts = base();
  opts.hands[1] = {"r3", "y2", "g2", "b3", "p2"};  // Bob: all trash
  opts.hands[2] = {"r5", "y3", "g3", "b5", "p3"};  // Cathy: the two criticals
  Game g = with_cards_left(setup(std::move(opts)), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "Cathy acts after Bob either way, so she is not short of a turn";
}

// No clue means no stall exists. The rule must fall through rather than invent
// an action -- `find_all_clues` comes back empty and there is nothing to return.
TEST(ForcedEndgameRuleFive, DeclinesWithNoClueTokens) {
  SetupOptions opts = base();
  opts.clue_tokens = 0;
  Game g = with_cards_left(setup(std::move(opts)), 1);

  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "with no token there is no stall, so the rule cannot fire";
}

// Scoped to one card left. At two, the deck outlasts the final-round arithmetic
// this rule is built on -- and `cards_left == 2` belongs to Rule 4.
TEST(ForcedEndgameRuleFive, DeclinesWithTwoCardsLeft) {
  Game g = with_cards_left(setup(base()), 2);
  EXPECT_FALSE(hanabi::endgame::forced_endgame_action(g).has_value())
      << "Rule 4 owns cards_left == 2, and it wants criticals in CP's OWN hand";
}

// The invariant the bug report was really about: a forced endgame action
// outranks a standing urgent reacter call. `forced_endgame_action` runs above
// `endgame.honours_reacter_call` in decide.cpp, so a pending reaction cannot
// take the turn away from a rule that has fired -- in a PLAIN variant, not just
// under Alternating Clues.
TEST(ForcedEndgameRuleFive, ForcedActionOutranksAStandingUrgentCall) {
  SetupOptions opts = base();
  // `endgame.honours_reacter_call` -- the branch that took the turn at 1974119
  // T53 -- is reactor0-only, so the fixture has to be. The VARIANT is still
  // plain, which is the half being checked: nothing here is specific to
  // Alternating Clues.
  opts.init = [](Game& game) { game.convention = Convention::REACTOR0; };
  Game g = with_cards_left(setup(std::move(opts)), 1);
  // Give Alice an urgent CALLED_TO_DISCARD on her slot 1 trash -- exactly the
  // shape that took the turn at 1974119 T53.
  const int alice = static_cast<int>(TestPlayer::ALICE);
  const int order = g.state.hands[alice][0];
  g.with_meta(order, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.urgent = true;
  });
  ASSERT_TRUE(g.meta[order].urgent) << "guard: a standing urgent call exists";

  PerformAction action = g.take_action();
  EXPECT_TRUE(action_is_clue(action))
      << "the forced stall must win. A discard here means the urgent call "
         "pre-empted it, which is the inversion the report described. Got "
      << hanabi::to_json(action, 0).dump();
}
