// Section 4's gate: three separate triggers (v13.2.0).
//
//   1. Alice is locked
//   2. Alice is at 8 clues
//   3. `pace() <= 1` AND any of
//        4a  Alice has no known playable card
//        4b  Bob holds a playable he does not know about, and Cathy has none
//        4c  some candidate gets TWO cards playing
//
// The first two are UNQUALIFIED -- in both, every alternative to cluing burns a
// card -- and 4a-4c hang off the pace arm alone. Through v13.1.0 that arm read
// `pace() == 0 && Alice has no known playable`; v13.2.0 widened the pace and
// added 4b and 4c as further ways in.
//
// Why 4a survives the widening: section 4 sits in decision phase 1, ABOVE the
// play phase, and its floor returns SOME clue regardless of tier. Opening the
// arm unconditionally therefore makes Alice clue instead of playing a card she
// can already see -- measured at pace 0 as 272 moved turns of 1187, 133 of them
// play -> clue. Replay 1943094 T19 pins that; these tests pin the structure.
//
// `priority_4_applies` is asserted directly, the way `priority_3_applies` is,
// so each alternative can be isolated from the rung's own ordering. 4c is
// exercised by handing the predicate a candidate carrying `new_plays = 2`,
// which tests the gate rather than whatever reading a fixture happens to
// produce.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

constexpr int kBasePace = 13;

// Drops pace without lowering max_score: each rank-2 has a spare copy, and the
// 1s are all on the stacks so two copies each are free.
std::vector<std::string> discards_for_pace(int target_pace) {
  static const std::vector<std::string> kPool = {
      "y1", "y1", "g1", "g1", "b1", "b1", "p1", "p1",
      "y2", "g2", "b2", "p2"};
  std::vector<std::string> out;
  for (int i = 0; i < kBasePace - target_pace && i < (int)kPool.size(); ++i) {
    out.push_back(kPool[i]);
  }
  return out;
}

// Every stack on 1, so "playable" is exactly "a 2" -- which is what lets a bare
// rank-2 clue make a card a KNOWN playable, and what makes a hand with no 2 in
// it a hand with nothing to play.
SetupOptions base(int target_pace, std::vector<std::string> alice,
                  std::vector<std::string> bob, std::vector<std::string> cathy) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {1, 1, 1, 1, 1};
  opts.clue_tokens = 4;  // not 8: the 8-clue arm is tested on its own
  opts.hands = {std::move(alice), std::move(bob), std::move(cathy)};
  opts.discarded = discards_for_pace(target_pace);
  use_reactor0(opts);
  return opts;
}

// `pre_clue` writes only `common` (test_harness.cpp), and 4a is read from
// Alice's OWN view -- `g.me().thinks_playables` -- because that is what the play
// phase below section 4 will read. So a pre-clue has to be mirrored into the
// player layer or Alice never learns what common knowledge just told her.
// `test_no_chuck_at_eight_clues.cpp` needs the same idiom for the same reason.
Game& mirror_to_players(Game& g, TestPlayer who, int slot) {
  const int order = g.state.hands[(int)who][slot - 1];
  const Thought& t = g.common.thoughts[order];
  for (Player& p : g.players) {
    p.thoughts[order].inferred = t.inferred;
    p.thoughts[order].possible = t.possible;
  }
  return g;
}

// Alice holds nothing playable; Bob hides a playable r2; Cathy has no 2.
Game alice_blind(int pace) {
  return setup(base(pace, {"r4", "g4", "b4", "p4", "r3"},
                    {"r2", "p4", "b4", "g4", "y4"},
                    {"p3", "g3", "b3", "y4", "y3"}));
}

// Alice KNOWS she holds a playable (a bare rank-2 clue, and every 2 plays here),
// and Cathy has a playable of her own, so 4a and 4b are both false.
Game alice_sees_a_play(int pace) {
  Game g = setup(base(pace, {"r2", "g4", "b4", "p4", "r3"},
                      {"y4", "p4", "b4", "g4", "r3"},
                      {"p2", "g3", "b3", "y4", "y3"}));
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"2"});
  return mirror_to_players(g, TestPlayer::ALICE, 1);
}

const std::vector<hanabi::reactor0::ClueCandidate> kNoCandidates;

// A real candidate, with its play count forced to two. Built from the position's
// own legal clues so every other field is what `analyse_clues` would produce.
std::vector<hanabi::reactor0::ClueCandidate> candidate_with_two_plays(
    const Game& g) {
  const State& s = g.state;
  const int bob = s.next_player_index(s.our_player_index);
  std::vector<std::pair<PerformAction, Action>> all;
  for (const Clue& clue : s.all_valid_clues(bob)) {
    PerformAction perform =
        clue.kind == ClueKind::COLOUR
            ? PerformAction{PerformColour{clue.target, clue.value}}
            : PerformAction{PerformRank{clue.target, clue.value}};
    ClueAction act{s.our_player_index, clue.target,
                   s.clue_touched(s.hands[clue.target], clue.kind, clue.value),
                   clue.base()};
    all.emplace_back(perform, Action{act});
  }
  auto cands = hanabi::reactor0::analyse_clues(g, all);
  if (cands.empty()) return {};
  std::vector<hanabi::reactor0::ClueCandidate> out{cands.front()};
  out.front().new_plays = 2;
  return out;
}

}  // namespace

// --- the pace arm ---------------------------------------------------------

TEST(Reactor0SectionFourGate, FourAOpensThePaceArm) {
  Game g = alice_blind(1);
  ASSERT_EQ(g.state.pace(), 1);
  ASSERT_TRUE(g.me().thinks_playables(g, (int)TestPlayer::ALICE).empty())
      << "guard: 4a holds -- Alice knows of nothing to play";
  EXPECT_TRUE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "an empty candidate list rules 4c out, so this is 4a alone";
}

TEST(Reactor0SectionFourGate, AKnownPlayableClosesThePaceArm) {
  Game g = alice_sees_a_play(1);
  ASSERT_EQ(g.state.pace(), 1);
  ASSERT_FALSE(g.me().thinks_playables(g, (int)TestPlayer::ALICE).empty())
      << "guard: 4a is false -- she can see a play of her own";
  EXPECT_FALSE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "section 4 must not out-rank a play Alice already has: its floor "
         "returns a clue regardless of tier, and it sits above the play phase";
}

// 4b: Bob sits on a playable he cannot see and Cathy cannot cover -- but that
// is only a reason to spend the turn if a clue ACTUALLY MOVES HIM. Whenever 4b
// is the sole opener, 4a is false, so Alice is giving up a play of her own; a
// clue that does not deliver trades a certain point for nothing.
//
// Measured: without the third conjunct, 800 low-pace turns had three
// `play -> clue` movers and every one abandoned a SCORING play, at pace 0, 0
// and -1 (replays 1972664 T17, 1971981 T18, 1972704 T17).
namespace {

// The 4b position: Alice knows a play of her own (4a false), Cathy has no 2, and
// Bob hides a playable y2.
Game bob_hides_a_playable() {
  Game g = setup(base(1, {"r2", "g4", "b4", "p4", "r3"},
                      {"y2", "p4", "b4", "g4", "r3"},
                      {"p3", "g3", "b3", "y4", "y3"}));
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"2"});
  return mirror_to_players(g, TestPlayer::ALICE, 1);
}

// A real candidate from the position, marked as one that gets Bob playing. The
// flag is what 4b reads; forcing it here tests the GATE rather than whatever
// reading a particular fixture happens to produce.
std::vector<hanabi::reactor0::ClueCandidate> candidate_that_moves_bob(
    const Game& g) {
  auto out = candidate_with_two_plays(g);
  if (out.empty()) return out;
  out.front().new_plays = 0;      // isolate 4b from 4c
  out.front().bob_plays_now = true;
  return out;
}

}  // namespace

TEST(Reactor0SectionFourGate, FourBNeedsAClueThatActuallyMovesBob) {
  Game g = bob_hides_a_playable();
  ASSERT_EQ(g.state.pace(), 1);
  ASSERT_FALSE(g.me().thinks_playables(g, (int)TestPlayer::ALICE).empty())
      << "guard: 4a is false, so 4b is the only arm that can open this";

  EXPECT_FALSE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "no candidate can move Bob, so 4b must not spend Alice's play on a "
         "clue that leaves him exactly where he was";
}

TEST(Reactor0SectionFourGate, FourBOpensWhenAClueMovesBob) {
  Game g = bob_hides_a_playable();
  auto cands = candidate_that_moves_bob(g);
  ASSERT_FALSE(cands.empty()) << "guard: the position offers a legal clue";
  ASSERT_LT(cands.front().new_plays, 2) << "guard: 4c is not what opens this";

  EXPECT_TRUE(hanabi::reactor0::priority_4_applies(g, cands))
      << "with a clue that gets Bob playing immediately, 4b is worth the turn";
}

// 4c: two plays for one clue is worth the turn even when Alice has her own.
TEST(Reactor0SectionFourGate, FourCOpensWhenACandidateGetsTwoPlays) {
  Game g = alice_sees_a_play(1);
  ASSERT_FALSE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "guard: 4a and 4b are both false in this position";

  auto cands = candidate_with_two_plays(g);
  ASSERT_FALSE(cands.empty()) << "guard: the position offers a legal clue";
  EXPECT_TRUE(hanabi::reactor0::priority_4_applies(g, cands))
      << "a candidate that gets two cards playing opens the arm on its own";
}

// The boundary. `pace() <= 1` is the whole of the widening, so pace 2 is shut
// however strongly 4a holds.
TEST(Reactor0SectionFourGate, PaceTwoIsClosedEvenWhenFourAHolds) {
  Game g = alice_blind(2);
  ASSERT_EQ(g.state.pace(), 2);
  ASSERT_TRUE(g.me().thinks_playables(g, (int)TestPlayer::ALICE).empty())
      << "guard: 4a holds, so only the pace bound can be closing this";
  EXPECT_FALSE(hanabi::reactor0::priority_4_applies(g, kNoCandidates));
}

// --- the two unqualified arms ---------------------------------------------

TEST(Reactor0SectionFourGate, EightCluesOpensWithoutFourAToFourC) {
  Game g = alice_sees_a_play(5);  // pace 5, and 4a false
  ASSERT_FALSE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "guard: closed on its own terms before the tokens are raised";

  Game h = g;
  h.state.clue_tokens = 8;
  EXPECT_TRUE(hanabi::reactor0::priority_4_applies(h, kNoCandidates))
      << "at 8 tokens a discard is illegal, so cluing is forced whatever Alice "
         "can see -- 4a-4c gate the PACE arm only";
}

TEST(Reactor0SectionFourGate, LockedOpensWithoutFourAToFourC) {
  // Alice's whole hand is 5s at stacks of 1: nothing playable, nothing safe to
  // throw, so she is locked -- and she is nowhere near low pace.
  Game g = setup(base(5, {"r5", "g5", "b5", "p5", "y5"},
                      {"y4", "p4", "b4", "g4", "r3"},
                      {"p2", "g3", "b3", "y4", "y3"}));
  for (int slot = 1; slot <= 5; ++slot) {
    g = pre_clue(std::move(g), TestPlayer::ALICE, slot, {"5"});
    mirror_to_players(g, TestPlayer::ALICE, slot);
  }
  ASSERT_GT(g.state.pace(), 1) << "guard: the pace arm cannot be what opens it";
  ASSERT_TRUE(g.common.thinks_locked(g, (int)TestPlayer::ALICE))
      << "guard: locked -- every card is a critical 5";

  EXPECT_TRUE(hanabi::reactor0::priority_4_applies(g, kNoCandidates))
      << "locked is its own trigger: with no chop, every alternative to cluing "
         "burns a card";
}
