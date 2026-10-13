// §1d: the receiver reads what the REACTER played (tiiah/CONVENTION.md, v16.19.0).
//
// `interpret_reactive` narrows the reacter's card at clue time, into `common`, so
// `note_hidden_action` later finds a singleton and advances the shared stacks instead
// of stamping a superposition. The receiver never gets there — it returns before the
// target walk, because it cannot see its own hand to find the target — so until
// v16.19.0 its copy of the reacter's card stayed as wide as the pre-clue empathy for
// the rest of the game, and its shared stacks stayed behind every reactive blind play
// the table ever made.
//
// The receiver can do it once the reaction resolves, because it WATCHED the card: the
// bucket the reacter reasoned in is the bucket of the identity the receiver saw.
//
// Alice is us AND the receiver here, which takes a clue given BY Bob: the reacter is
// then `bob_of(Bob)` = Cathy and the receiver the seat after her, which is Alice. The
// clue is built by hand rather than through `take_turn`, because the harness derives
// which cards a clue touches from their identities and ours has none — the server
// tells us the slots and nothing else, which is exactly what `list_` is.
#include <gtest/gtest.h>

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Nothing is playable-and-known in Cathy's hand, so the position does not flip and
// Bob's clue to Alice is an ordinary reactive with Cathy reacting.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us), the receiver
      {"r4", "y4", "g4", "b4", "p4"},  // Bob, the giver
      {"p1", "y3", "g3", "b3", "r3"},  // Cathy, the reacter — slot 1 is the play
  };
  opts.starting = TestPlayer::BOB;
  opts.clue_tokens = 6;
  use_tiiah(opts);
  return opts;
}

// Bob clues Alice, naming two of her slots. Which slots hardly matters: the receiver
// cannot run the walk either way, and the point of the test is what happens to
// CATHY's card afterwards.
Game bob_clues_alice(Game g) {
  const std::vector<int> touched{order_at(g, TestPlayer::ALICE, 2),
                                 order_at(g, TestPlayer::ALICE, 4)};
  g.catchup = true;
  g.handle_action(ClueAction{static_cast<int>(TestPlayer::BOB),
                             static_cast<int>(TestPlayer::ALICE), touched,
                             BaseClue{ClueKind::RANK, 3}});
  g.handle_action(TurnAction{g.state.turn_count,
                             static_cast<int>(TestPlayer::CATHY)});
  g.catchup = false;
  return g;
}

}  // namespace

// Purple is bucket 2 on its own, so "a playable card of bucket 2" is one identity and
// the receiver can name what the reacter played — which moves the SHARED stacks, the
// thing a superposition cannot do.
TEST(TiiahReacterPlayRead, OnePlayableInTheBucketResolvesTheCard) {
  Game g = setup(opts_for());
  const int played = order_at(g, TestPlayer::CATHY, 1);  // the p1

  g = bob_clues_alice(std::move(g));
  ASSERT_FALSE(g.waiting.empty()) << "guard: the clue read as a reactive";
  ASSERT_EQ(g.waiting.front().reacter, static_cast<int>(TestPlayer::CATHY));
  ASSERT_EQ(g.waiting.front().receiver, static_cast<int>(TestPlayer::ALICE));
  ASSERT_EQ(g.waiting.front().react_order, -1)
      << "guard: as the receiver we could not run the walk, so nothing was stamped";

  g = hidden_action(std::move(g), TestPlayer::CATHY, /*slot=*/1,
                    /*reached_the_hole=*/true, "y2");

  EXPECT_FALSE(g.meta[played].superposed())
      << "bucket 2 holds exactly one playable, so the reacter knows its own card and "
         "so do we -- before v16.19.0 this was all 25 identities";
  EXPECT_EQ(g.state.common_play_stacks[4], 1)
      << "and a play every seat can name advances the view every seat shares";
  EXPECT_EQ(g.state.play_stacks[4], 1) << "our own belief had it either way";
}

// The other seats already worked this out at clue time, and the new rule must leave
// them exactly as they were. Replay 2008177 T3's fixture, moved to four suits (Cathy's
// target is a g1, bucket 2), where Alice gives the clue: Bob reacts on his slot 3 and
// the walk read it as the y1 -- the one playable of bucket 1 -- so that is what goes
// into the hole, and the reaction-time rule recomputes the same reading and changes
// nothing. (Until v24.0.0 red and yellow shared bucket 0, the card was an r1, and the
// reading the two-card superposition `{r1,y1}`.)
TEST(TiiahReacterPlayRead, TheGiversSeatKeepsTheReadingItAlreadyHad) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (4 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // Alice (us), the giver
      {"b4", "b5", "y1", "r1", "b3"},  // Bob, the reacter
      {"y4", "b3", "y5", "g1", "y2"},  // Cathy, the receiver
  };
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = std::vector<int>{0, 0, 0, 0};
  opts.clue_tokens = 6;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int played = order_at(g, TestPlayer::BOB, 3);

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");
  ASSERT_FALSE(g.waiting.empty()) << "guard: an ordinary reactive, Bob reacting";
  ASSERT_EQ(g.waiting.front().reacter, static_cast<int>(TestPlayer::BOB));
  ASSERT_EQ(g.waiting.front().react_order, played)
      << "guard: the walk ran at this seat and named Bob's slot 3";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/3,
                    /*reached_the_hole=*/true, "y2");

  EXPECT_FALSE(g.meta[played].superposed())
      << "the clue-time reading, untouched: one identity, so nothing to superpose";
  EXPECT_EQ(g.common.thoughts[played].possibilities(),
            IdentitySet::single(Identity{1, 1}));
  EXPECT_EQ(g.state.common_play_stacks[1], 1)
      << "and a play every seat can name advances the view every seat shares";
}
