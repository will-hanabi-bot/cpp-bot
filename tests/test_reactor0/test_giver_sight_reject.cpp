// §1g in its sharpest form: giver-only knowledge REJECTS, it does not
// reinterpret.
//
// A stable clue's promise is a claim about what the RECEIVER can work out. The
// giver can see the receiver's hand and so routinely knows more -- and when
// that extra knowledge is the only thing making the promise empty, the giver
// must not give the clue. Reading it privately as a "stall" is how the bot ends
// up handing over a promise it has itself decided is false: replay 1973575 T62.
//
// The same predicate keeps its old meaning for a seat READING somebody else's
// clue (replay 1967478 T42), which `test_provably_trash_focus.cpp` pins from
// both sides. This file is about the giver, and about the two things that
// follow from the reject: the rank half of it under Odds and Evens, and the
// fact that `analyse_clues` then removes the clue from every rung.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Purple on 4, so p5 is the only useful purple. Bob's slot 1 is a dead p3 and
// his slot 5 holds the last p5 -- visible to us, invisible to him. A Purple
// clue touches exactly those two, so its promise lands on slot 1.
//
// `bob_slot5` is parameterised only so the card that makes the proof possible
// is named at the call site rather than buried in the hand.
SetupOptions purple_opts(std::string bob_slot5) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 4};  // purple on 4
  opts.hands = {
      {"r4", "y4", "g4", "b4", "r5"},              // Alice (giver, us)
      {"p3", "r2", "r1", "g3", std::move(bob_slot5)},
      {"y1", "y2", "y3", "b1", "b2"},              // Cathy
  };
  use_reactor0(opts);
  return opts;
}

std::vector<std::pair<PerformAction, Action>> all_clues_from(const Game& g) {
  const State& s = g.state;
  std::vector<std::pair<PerformAction, Action>> out;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == s.our_player_index) continue;
    for (const Clue& c : s.all_valid_clues(target)) {
      PerformAction perform =
          c.kind == ClueKind::COLOUR
              ? PerformAction{PerformColour{c.target, c.value}}
              : PerformAction{PerformRank{c.target, c.value}};
      out.emplace_back(
          perform,
          Action{ClueAction{s.our_player_index, c.target,
                            s.clue_touched(s.hands[target], c.kind, c.value),
                            c.base()}});
    }
  }
  return out;
}

int colour_of(const Game& g, const std::string& name) {
  const auto& names = g.state.variant->clue_colour_names;
  for (size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace

// --- the reject -----------------------------------------------------------

TEST(Reactor0GiverSightReject, APromiseOnlyTheGiverKnowsIsEmptyIsRejected) {
  Game g = setup(purple_opts("p5"));
  g = take_turn(std::move(g), "Alice clues purple to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "Purple promises Bob's slot 1, which we can prove is the dead p3 only "
         "because we can see the p5 in his own slot 5 -- he cannot, and would "
         "read the play";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY));
}

// The discriminator, varying ONLY who gives the clue.
//
// Same shape, one seat over: the giver is Bob and his next player is Cathy, who
// holds the dead p3 newest and the last p5 oldest. We are seat 0, watching. The
// sight that proves Cathy's slot 1 dead is now shared by us and the giver but
// still not by the holder -- so it reinterprets, exactly as replay 1967478
// requires, and does not reject.
//
// Moving the p5 out of the receiver's hand would NOT be a clean control here:
// that leaves the receiver's slot 1 a card the giver can see is not playable,
// which `target_play` already refuses for a separate reason that
// `test_provably_trash_focus.cpp` documents. Who gives it is the only variable
// this change touches.
TEST(Reactor0GiverSightReject, TheSameClueFromAnotherSeatIsAStall) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::BOB;
  opts.play_stacks = {0, 0, 0, 0, 4};  // purple on 4
  opts.hands = {
      {"r4", "y4", "g4", "b4", "r5"},  // Alice -- us, watching
      {"r1", "y1", "g1", "b1", "r2"},  // Bob -- giver
      {"p3", "r3", "y3", "g3", "p5"},  // Cathy -- dead p3 newest, last p5 oldest
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Bob clues purple to Cathy");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL)
      << "we did not give this clue, so our sight reinterprets rather than "
         "rejects";
  EXPECT_FALSE(any_status(g, TestPlayer::CATHY, CardStatus::CALLED_TO_PLAY));
}

// --- the consequence for the decision layer -------------------------------

// A MISTAKE is dropped by `analyse_clues`, which is what removes the clue from
// every rung of both `choose_clue` and `choose_endgame_clue` at once. Without
// that the endgame's rung 3 picks it up again -- exactly replay 1973575 T62.
TEST(Reactor0GiverSightReject, ARejectedClueIsNotOfferedToAnyRung) {
  Game g = setup(purple_opts("p5"));
  const int purple = colour_of(g, "Purple");
  ASSERT_GE(purple, 0);
  const int bob = static_cast<int>(TestPlayer::BOB);

  bool purple_is_legal = false;
  for (const Clue& c : g.state.all_valid_clues(bob)) {
    if (c.kind == ClueKind::COLOUR && c.value == purple) purple_is_legal = true;
  }
  ASSERT_TRUE(purple_is_legal)
      << "guard: the clue IS legal by the rules of the game -- it is the "
         "convention that refuses it";

  auto cands = hanabi::reactor0::analyse_clues(g, all_clues_from(g));
  for (const auto& c : cands) {
    EXPECT_FALSE(c.action.target == bob &&
                 c.action.clue.kind == ClueKind::COLOUR &&
                 c.action.clue.value == purple)
        << "the rejected clue must not survive into the candidate pool";
  }

  auto pick = hanabi::reactor0::choose_endgame_clue(g, cands);
  if (pick) {
    const auto* colour = std::get_if<PerformColour>(&*pick);
    EXPECT_FALSE(colour != nullptr && colour->target == bob &&
                 colour->value == purple)
        << "and no endgame rung may select it";
  }
}

// --- the rank side --------------------------------------------------------

// Odds and Evens promises the RIGHTMOST newly touched card, so the same defect
// reaches `stable_rank` by a different route.
//
// Every other stack is finished, so g5 is the ONLY useful odd identity in the
// game -- which is what makes `odd` a direct play clue here rather than a lock.
// Bob holds that g5 in slot 1 and a dead g3 in slot 5. Odds and Evens promises
// the RIGHTMOST newly touched card, so the promise lands on the g3; and we can
// prove it is the g3 only because we can see the real g5 in his own slot 1.
TEST(Reactor0GiverSightReject, TheRankHalfIsRejectedTooUnderOddsAndEvens) {
  SetupOptions opts;
  opts.variant_name = "Odds and Evens (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {5, 5, 4, 5, 5};  // only green is live; g5 is playable
  opts.hands = {
      {"r1", "y1", "b1", "p1", "r2"},  // Alice (giver, us) -- all trash
      {"g5", "r3", "y3", "b3", "g3"},  // Bob -- the last g5 newest, dead g3 oldest
      {"g1", "g2", "g4", "y2", "b2"},  // Cathy
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.deck[order_at(g, TestPlayer::BOB, 1)].id(), (Identity{2, 5}))
      << "guard: Bob holds the last g5, where we can see it and he cannot";
  ASSERT_TRUE(g.state.is_playable(Identity{2, 5})) << "guard: and it plays";

  g = take_turn(std::move(g), "Alice clues 1 to Bob");  // 1 == "odd"

  EXPECT_EQ(last_clue_interp(g), ClueInterp::MISTAKE)
      << "odd promises the rightmost newly touched card -- Bob's slot 5 -- and "
         "we can prove that is the dead g3 only from sight he does not have";
}

// `unnecessary_focus` sits next to the sight test in `stable_rank` and must NOT
// have been swept into the reject: it reads `common.thoughts`, so every seat
// computes it alike and a stall there is a stall for everybody.
TEST(Reactor0GiverSightReject, ACommonKnowledgeStallIsStillAStall) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  // Every rank-4 identity is already played, so a rank 4 clue teaches nothing
  // that could be a play -- and that is visible to everyone.
  opts.play_stacks = {5, 5, 5, 5, 5};
  opts.hands = {
      {"r1", "y1", "g1", "b1", "p1"},
      {"r4", "y4", "g4", "b4", "p4"},
      {"r3", "y3", "g3", "b3", "p3"},
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 4 to Bob");

  EXPECT_NE(last_clue_interp(g), ClueInterp::MISTAKE)
      << "a stall everybody can see is a stall, not a clue we must not give";
}
