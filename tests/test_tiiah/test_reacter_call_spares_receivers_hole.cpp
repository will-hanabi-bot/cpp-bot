// A REACTER'S CALL IS NO EVIDENCE ABOUT THE RECEIVER'S HOLE (tiiah/CONVENTION.md §1e
// rule 2, v23.15.0; human_vs_bot_diagnostics/9000118.md T6-T8).
//
// A reactive clue's call on the reacter's card is stamped at clue time by the giver
// and the reacter alone; the receiver decodes it only once the reacter acts. So "the
// called identity is still needed" narrows every seat's hole cards but the
// receiver's. (Replaces the 9000118 replay test, whose history v23.18.0's stable 1
// re-reads: its T1 1 now names the hole card outright.)
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/conventions/tiiah/superposition.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

const Identity kR1{0, 1}, kB1{3, 1};

// Bob has thrown a card into the hole that the team reads as `{r1,b1}`; Cathy now
// clues `target`, and Alice's slot 1 is newly called as exactly the b1.
struct Position {
  Game prev;
  Game game;
  int hole = -1;
};

Position position(int target) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {{"xx", "xx", "xx", "xx", "xx"},
                {"r4", "y4", "g4", "b4", "p4"},
                {"r3", "y3", "g3", "b3", "p3"}};
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 5;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int hole = order_at(g, TestPlayer::BOB, 1);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1, /*reached_the_hole=*/true,
                    "g2");
  const IdentitySet set = IdentitySet::empty().add(kR1).add(kB1);
  g.with_meta(hole, [&set](ConvData& m) {
    m.superposition = set;
    m.shared_left = IdentitySet::empty();
  });
  Position p{g, g, hole};
  const int called = order_at(p.game, TestPlayer::ALICE, 1);
  p.game.with_meta(called, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  p.game.with_thought(called, [](const Thought& t) {
    Thought out = t;
    out.possible = IdentitySet::single(kB1);
    out.inferred = IdentitySet::single(kB1);
    return out;
  });
  const int touched = order_at(p.game, static_cast<TestPlayer>(target), 2);
  hanabi::tiiah::collapse_superpositions(
      p.game, p.prev, Action{ClueAction{2, target, {touched}, BaseClue(ClueKind::RANK, 4)}});
  return p;
}

}  // namespace

// Cathy clues Bob, and Alice's called card is the REACTER's: Bob's hole card keeps
// the b1, as Bob himself does.
TEST(TiiahReacterCallSparesReceiversHole, TheReceiversHoleCardKeepsTheIdentity) {
  const Position p = position(/*target=*/1);
  ASSERT_TRUE(p.game.meta[p.hole].superposed());
  EXPECT_TRUE(p.game.meta[p.hole].superposition.contains(kB1));
}

// The control: the same call on the clue's own TARGET is evidence for every seat,
// and the b1 leaves Bob's hole card.
TEST(TiiahReacterCallSparesReceiversHole, ACallOnTheTargetIsEvidence) {
  const Position p = position(/*target=*/0);
  EXPECT_FALSE(p.game.meta[p.hole].superposed() &&
               p.game.meta[p.hole].superposition.contains(kB1));
}
