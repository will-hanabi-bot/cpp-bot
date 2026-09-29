// §1e rule 6's collapse is SHARED, and it corrects the frozen clue frame
// (tiiah/CONVENTION.md, v16.21.0).
//
// When a partner's play only lands in one of the worlds our own hole cards leave open,
// that world is the one we are in — and the conclusion is not ours alone. Every seat
// watched our card go into the hole, every seat holds its candidate set (it is built
// from `common`), the seat that made the play knows its own reading of it, and "never
// presume a strike" is the convention rather than one seat's opinion. So the shared view
// and every pairwise row move with our belief.
//
// Two things stay as they were, and both are here as controls. Rule 6 asked of our OWN
// plays judges them against our OWN belief, which no partner can reproduce, so it stays
// private. And a frozen `ReactorWC::clue_play_stacks` is our ESTIMATE of the stacks the
// giver chose a target in — our own hole plays are the one thing that estimate can be
// wrong about, since the giver could always see the card — so naming one corrects it
// rather than moving it.
//
// Replay 2011133 is the game the private form cost; it is written up in §1e rule 6 and
// pinned by `test_replay_2011133_rule_six_is_shared.cpp`.
#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

// Alice is us and cannot see her own hand. Her slot 1 goes into the hole reading every
// rank 1; Bob then plays the `r2`, which lands only where that card was the `r1`.
SetupOptions opts_for() {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "y4", "g4", "b4", "p4"},
      {"y1", "g1", "b1", "p1", "r5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  return opts;
}

// A frozen frame on a pending reaction nobody in this fixture will resolve: the reacter
// is Cathy, so Bob's play below never matches it.
void park_a_frame(Game& g, int receiver, std::vector<int> frame) {
  if (static_cast<int>(g.pending_reactions.size()) != g.state.num_players) {
    g.pending_reactions.assign(g.state.num_players, std::nullopt);
  }
  ReactorWC wc;
  wc.giver = static_cast<int>(TestPlayer::ALICE);
  wc.reacter = static_cast<int>(TestPlayer::CATHY);
  wc.receiver = receiver;
  wc.clue_play_stacks = std::move(frame);
  g.pending_reactions[receiver] = wc;
}

}  // namespace

// The shared half: the collapse moves the view every seat computes, not only ours.
TEST(TiiahSharedRuleSix, APartnersRescuedPlayIsSharedEvidence) {
  Game g = setup(opts_for());
  const int mine = order_at(g, TestPlayer::ALICE, 1);

  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  ASSERT_TRUE(g.meta[mine].superposed()) << "guard: five rank 1s in the hole";
  ASSERT_EQ(g.state.common_play_stacks[0], 0) << "guard: nobody knows any red yet";

  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y1");

  EXPECT_EQ(g.state.strikes, 0) << "the r2 is presumed to land, not to strike";
  EXPECT_TRUE(g.meta[mine].superposed() == false)
      << "only one world lets it land, so we have learned what we threw";
  EXPECT_EQ(g.state.play_stacks[0], 2) << "our r1, then the r2 we watched";
  EXPECT_EQ(g.state.common_play_stacks[0], 1)
      << "and the r1 is COMMON knowledge: every seat watched the card, holds its "
         "candidate set, and applies the same rule. Before v16.21.0 this stayed 0";
  for (size_t p = 0; p < g.state.pairwise_play_stacks.size(); ++p) {
    EXPECT_GE(g.state.pairwise_play_stacks[p][0], 1) << "every row learns it, row " << p;
  }
}

// The control: rule 6 asked of our OWN plays reads our own belief, which Bob cannot
// reproduce, since he cannot see his own g1. So it moves our stacks, not the shared view
// and not Bob's row. Bob's `g1` lands on its own here, so the partner form never fires
// and only the own-play form does. Cathy watched both cards and holds our `{g1,b1}`
// too, so her row's world replay makes the same argument (v18.1.0).
TEST(TiiahSharedRuleSix, OurOwnCollapseReachesOnlyTheRowThatWatchedIt) {
  SetupOptions opts = opts_for();
  opts.hands[1] = {"g1", "y4", "g4", "b4", "p4"};
  Game g = setup(std::move(opts));
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  const IdentitySet two =
      IdentitySet::empty().add(Identity{2, 1}).add(Identity{3, 1});
  g.state.deck[mine].clued = true;
  g.with_thought(mine, [&two](const Thought& t) {
    Thought out = t;
    out.inferred = two;
    out.possible = two;
    return out;
  });

  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y1");

  ASSERT_FALSE(g.meta[mine].superposed())
      << "guard: our card cannot have duped Bob's g1, so it was the b1";
  EXPECT_EQ(g.state.play_stacks[3], 1) << "our own belief has the blue";
  EXPECT_EQ(g.state.common_play_stacks[3], 0)
      << "but the argument rests on our own stacks, so the shared view may not take it";
  EXPECT_EQ(g.state.pairwise_play_stacks[static_cast<int>(TestPlayer::BOB)][3], 0)
      << "nor Bob's row: he cannot see the g1 he threw";
  EXPECT_EQ(g.state.pairwise_play_stacks[static_cast<int>(TestPlayer::CATHY)][3], 1)
      << "Cathy watched both cards, and replays ours with the same {g1,b1}";
}

// The frozen frame is an estimate of what the giver could see, and our own hole plays
// are the one thing it can be wrong about. Naming one raises it — and never lowers it.
TEST(TiiahSharedRuleSix, TheFrozenClueFrameIsCorrected) {
  Game g = setup(opts_for());
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  park_a_frame(g, static_cast<int>(TestPlayer::ALICE), {0, 0, 0, 0, 0});
  park_a_frame(g, static_cast<int>(TestPlayer::BOB), {3, 0, 0, 0, 0});

  g = pre_clue(std::move(g), TestPlayer::ALICE, /*slot=*/1, {"1"});
  g = hidden_action(std::move(g), TestPlayer::ALICE, /*slot=*/1,
                    /*reached_the_hole=*/true);
  g = hidden_action(std::move(g), TestPlayer::BOB, /*slot=*/1,
                    /*reached_the_hole=*/true, "y1");
  ASSERT_FALSE(g.meta[mine].superposed()) << "guard: the settle happened";

  ASSERT_TRUE(g.pending_reactions[static_cast<int>(TestPlayer::ALICE)].has_value());
  EXPECT_EQ(g.pending_reactions[static_cast<int>(TestPlayer::ALICE)]
                ->clue_play_stacks[0],
            1)
      << "the giver's frame always counted our r1; ours can now say so too";
  ASSERT_TRUE(g.pending_reactions[static_cast<int>(TestPlayer::BOB)].has_value());
  EXPECT_EQ(g.pending_reactions[static_cast<int>(TestPlayer::BOB)]->clue_play_stacks[0],
            3)
      << "and a frame already ahead of the card is left alone -- this only ever raises";
}
