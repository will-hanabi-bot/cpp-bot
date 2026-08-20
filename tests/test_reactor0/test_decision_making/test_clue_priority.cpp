// Reactor0's General Clue Evaluation List (src/conventions/reactor0/decision.cpp,
// DECISION_MAKING.md "Decision phase 1").
//
// The list is what replaced the tuned-constant argmax: Alice does the first
// thing that applies, and every rung carries its own tiebreak chain. These
// tests are what make that spec CHECKABLE rather than merely readable — one
// fixture per rung and per load-bearing tiebreak, asserted against
// `choose_clue` and the exported rung predicates rather than through
// `take_action`, whose answer also depends on the endgame fork and the urgent
// path.
//
// Fixtures are setup()-built, so the `"convention": "reactor0"` snapshot key
// does not apply — `use_reactor0(opts)` sets the convention. A replay test
// added here would need that key.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueCandidate;
using hanabi::reactor0::ClueShape;
using hanabi::reactor0::ClueTier;

namespace {

// Every clue Alice could legally give, analysed — the same set and the same
// single-simulation pass `take_action` will hand to `choose_clue`.
std::vector<std::pair<PerformAction, Action>> all_candidate_clues(const Game& g) {
  const State& s = g.state;
  std::vector<std::pair<PerformAction, Action>> out;
  for (int target = 0; target < s.num_players; ++target) {
    if (target == s.our_player_index) continue;
    for (const Clue& clue : s.all_valid_clues(target)) {
      PerformAction perform =
          clue.kind == ClueKind::COLOUR
              ? PerformAction{PerformColour{clue.target, clue.value}}
              : PerformAction{PerformRank{clue.target, clue.value}};
      ClueAction act{s.our_player_index, clue.target,
                     s.clue_touched(s.hands[target], clue.kind, clue.value),
                     clue.base()};
      out.emplace_back(perform, Action{act});
    }
  }
  return out;
}

std::vector<ClueCandidate> analysed(const Game& g) {
  return hanabi::reactor0::analyse_clues(g, all_candidate_clues(g));
}

std::optional<PerformAction> chosen(const Game& g) {
  return hanabi::reactor0::choose_clue(g, analysed(g));
}

// Describe a PerformAction well enough to read a failure without a debugger.
std::string describe(const std::optional<PerformAction>& a) {
  if (!a) return "(no clue)";
  if (auto* c = std::get_if<PerformColour>(&*a)) {
    return "colour " + std::to_string(c->value) + " -> p" +
           std::to_string(c->target);
  }
  if (auto* r = std::get_if<PerformRank>(&*a)) {
    return "rank " + std::to_string(r->value) + " -> p" +
           std::to_string(r->target);
  }
  return "(not a clue)";
}

bool is_rank_to(const std::optional<PerformAction>& a, int value, int target) {
  auto* r = a ? std::get_if<PerformRank>(&*a) : nullptr;
  return r && r->value == value && r->target == target;
}

// Does the analysed set contain a candidate of this shape at all? Several tests
// below need to know the fixture actually produced the shape they are ranking,
// so that a "chose X" assertion cannot pass because Y was never a candidate.
bool has_shape(const std::vector<ClueCandidate>& cs, ClueShape sh) {
  for (const auto& c : cs) {
    if (c.reading.shape == sh) return true;
  }
  return false;
}

int order_of(const Game& g, TestPlayer p, int slot) { return order_at(g, p, slot); }

}  // namespace

// --- the default tiebreak -------------------------------------------------

// 1.99 * (new useful) - (new trash). The 1.99 rather than 2 is the whole point:
// two useful plus one trash (2.98) must still beat one useful alone (1.99),
// while one useful plus one trash (0.99) must not.
TEST(Reactor0CluePriority, DefaultScoreCountsNewTouchesOnly) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "r3", "g2", "b1", "b1"},
      {"y3", "g3", "b3", "p3", "p4"},
  };
  opts.play_stacks = {1, 0, 0, 0, 0};  // r1 played, so r1 would be trash
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  const ClueCandidate* blue = nullptr;
  for (const auto& c : cands) {
    if (c.action.target == 1 && c.action.clue.kind == ClueKind::COLOUR &&
        c.action.clue.value == 3) {
      blue = &c;
    }
  }
  ASSERT_NE(blue, nullptr) << "guard: blue is a legal clue to Bob";
  // Blue touches b1 b1 — both newly touched, both useful (blue stack is 0).
  EXPECT_DOUBLE_EQ(blue->default_score, 2 * 1.99)
      << "two new useful touches and no trash";
}

// --- the tier gate --------------------------------------------------------

// Lifted verbatim from the deleted `eval_action`, so its boundaries must not
// move: occupied Alice needs HIGH while pace >= 1 and tokens < 8; an unoccupied
// Alice needs only MEDIUM, and only while tokens <= 3.
TEST(Reactor0CluePriority, GateIsSilentOutsideItsWindow) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "y4", "g4", "b4", "p4"},
      {"y3", "g3", "b3", "p3", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 5;  // unoccupied window is tokens <= 3
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_FALSE(cands.empty());
  for (const auto& c : cands) {
    EXPECT_TRUE(hanabi::reactor0::clue_is_admissible(g, c))
        << "at 5 tokens with an unoccupied Alice the gate must not fire";
  }
}

TEST(Reactor0CluePriority, GateDemandsMediumAtThreeTokens) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "y4", "g4", "b4", "p4"},
      {"y3", "g3", "b3", "p3", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 3;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  ASSERT_GE(g.state.pace(), 1) << "guard: the gate only runs above pace zero";

  for (const auto& c : analysed(g)) {
    EXPECT_EQ(hanabi::reactor0::clue_is_admissible(g, c), c.tier >= ClueTier::MEDIUM)
        << "at 3 tokens an unoccupied Alice admits exactly the not-LOW clues";
  }
}

// At 8 tokens neither window applies. This is what lets section 4 rank clues
// the gate would otherwise flatten — and it is the reason the old scorer
// carried an explicit `< 8` exemption.
TEST(Reactor0CluePriority, GateNeverFiresAtEightTokens) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "y4", "g4", "b4", "p4"},
      {"y3", "g3", "b3", "p3", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 8;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  for (const auto& c : analysed(g)) {
    EXPECT_TRUE(hanabi::reactor0::clue_is_admissible(g, c))
        << "the gate is exempt at 8 tokens, whatever the tier";
  }
}

// --- priority 2's admissibility -------------------------------------------

// `discard_is_affordable` is the condition that retires the old
// `drop_pointless_double_discards` filter: a reactive that throws away
// something real is never proposed in the first place.
TEST(Reactor0CluePriority, AffordableCoversTrashDupeAndVisibleCopy) {
  SetupOptions opts;
  opts.hands = {
      {"r1", "xx", "xx", "xx", "xx"},
      {"r1", "g3", "g3", "b4", "y2"},
      {"y2", "p3", "p4", "b5", "g5"},
  };
  opts.play_stacks = {1, 0, 0, 0, 0};  // r1 is now basic trash
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int bob = 1;
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, bob, order_of(g, TestPlayer::BOB, 1)))
      << "r1 is basic trash once the red stack is at 1";
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, bob, order_of(g, TestPlayer::BOB, 2)))
      << "g3 is duplicated inside Bob's own hand";
  EXPECT_TRUE(hanabi::reactor0::discard_is_affordable(
      g, bob, order_of(g, TestPlayer::BOB, 5)))
      << "Alice can see the other y2 in Cathy's hand";
  EXPECT_FALSE(hanabi::reactor0::discard_is_affordable(
      g, bob, order_of(g, TestPlayer::BOB, 4)))
      << "b4 is useful, unduplicated and invisible elsewhere — losing it costs "
         "the team a card, so no rung may propose throwing it away";
}

// --- the rung 3.6 / 4.8 tiebreak ------------------------------------------

// DECISION_MAKING.md's worked example, transcribed. Empty stacks, Bob holds
// `r2 g3 r4 g4 b4` and Cathy `r3 r3 b5 g5 p5`: b4 needs b1 b2 b3, none visible
// → 3; g4 needs g1 g2 g3 and g3 is in Bob's hand → 2; r4 needs r1 r2 r3, with
// r2 in Bob's hand and r3 in Cathy's → 1.
TEST(Reactor0CluePriority, MissingConnectorsMatchesTheSpecExample) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r2", "g3", "r4", "g4", "b4"},
      {"r3", "r3", "b5", "g5", "p5"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  EXPECT_EQ(hanabi::reactor0::missing_connectors(g, order_of(g, TestPlayer::BOB, 5)), 3)
      << "b4: b1 b2 b3 are all invisible";
  EXPECT_EQ(hanabi::reactor0::missing_connectors(g, order_of(g, TestPlayer::BOB, 4)), 2)
      << "g4: g3 is in Bob's own hand, so only g1 g2 are missing";
  EXPECT_EQ(hanabi::reactor0::missing_connectors(g, order_of(g, TestPlayer::BOB, 3)), 1)
      << "r4: r2 is Bob's and r3 is Cathy's, so only r1 is missing";
}

// --- the walk itself ------------------------------------------------------

// The whole point of the ordering: a reactive PLAY clue is rung 1, so it is
// taken ahead of anything the lower rungs could offer.
TEST(Reactor0CluePriority, ReactivePlayOutranksEverythingBelowIt) {
  SetupOptions opts;
  // Cathy's leftmost playable is r1 at slot 2; rank 3 to Cathy makes anchor 3,
  // react_slot = (3 + 5 - 2) % 5 = 1, and Bob's slot 1 holds a playable g1.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "y4", "b4", "p4", "y3"},
      {"y3", "r1", "g3", "b3", "p4"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_TRUE(has_shape(cands, ClueShape::REACTIVE_PLAY))
      << "guard: the fixture must actually offer a reactive play clue";

  auto pick = hanabi::reactor0::choose_clue(g, cands);
  ASSERT_TRUE(pick.has_value()) << "a reactive play clue is always worth giving";
  EXPECT_TRUE(is_rank_to(pick, 3, 2))
      << "rung 1 takes the reactive play clue; got " << describe(pick);
}

// Section 4's floor. At 8 tokens a discard is illegal, so the branch must
// return SOMETHING — otherwise `take_action` falls to its last-resort branch
// and blind-plays slot 1, which is worse than any decodable clue.
TEST(Reactor0CluePriority, EightTokensAlwaysReturnsAClue) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "g4", "b4", "p4", "y3"},
      {"g3", "b3", "p3", "y2", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 8;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_FALSE(cands.empty()) << "guard: Alice has legal clues here";
  EXPECT_TRUE(hanabi::reactor0::choose_clue(g, cands).has_value())
      << "section 4 always returns a clue at 8 tokens";
}

// The counterpart guarantee: the floor is about RANKING, not about conjuring a
// clue that does not exist. With no candidates the walk declines, and
// `take_action`'s existing play/discard path runs.
TEST(Reactor0CluePriority, NoCandidatesDeclines) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "g4", "b4", "p4", "y3"},
      {"g3", "b3", "p3", "y2", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  EXPECT_FALSE(hanabi::reactor0::choose_clue(g, {}).has_value())
      << "an empty candidate set means Alice cannot clue at all";
}

// A clue whose reading predicts a misplay is inadmissible everywhere except
// rung 4.7, which allows a strike explicitly. Nothing the walk returns outside
// that rung may carry a STRIKE outcome.
TEST(Reactor0CluePriority, ChosenClueNeverPredictsAStrikeBelowEightTokens) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r4", "y4", "g4", "b4", "p4"},
      {"r3", "y3", "g3", "b3", "p3"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 4;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  auto pick = hanabi::reactor0::choose_clue(g, cands);
  if (!pick) return;  // declining is a legal answer here
  for (const auto& c : cands) {
    if (describe(pick) != describe(std::optional<PerformAction>{c.perform})) continue;
    EXPECT_NE(c.reading.reacter_side.outcome, hanabi::reactor0::Outcome::STRIKE);
    EXPECT_NE(c.reading.receiver_side.outcome, hanabi::reactor0::Outcome::STRIKE);
    EXPECT_NE(c.reading.stable_outcome, hanabi::reactor0::Outcome::STRIKE);
  }
}

// --- H4 and Precedence ----------------------------------------------------

// `choose_h4_clue` is Precedence step 1, and it must NOT apply section 4's
// floor: an H4 clue outranks a pending reaction, an arbitrary one does not.
// Here there is no finesse available at all, so it must decline even at 8
// tokens, where `choose_clue` would happily return the floor's pick.
TEST(Reactor0CluePriority, H4DeclinesWithoutAFinesseEvenAtEightTokens) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "g4", "b4", "p4", "y3"},
      {"g3", "b3", "p3", "y2", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 8;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  EXPECT_TRUE(hanabi::reactor0::choose_clue(g, cands).has_value())
      << "guard: the floor does fire here";
  EXPECT_FALSE(hanabi::reactor0::choose_h4_clue(g, cands).has_value())
      << "no finesse is available, so nothing may outrank a pending reaction";
}

// And the positive case: the Phase B finesse fixture from test_clue_tier.cpp,
// which `clue_is_h4` marks and `choose_h4_clue` therefore offers.
TEST(Reactor0CluePriority, H4OffersTheFinesse) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "r1", "b4", "g4", "y4"},
      {"y5", "g3", "r2", "b3", "p4"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  bool any_h4 = false;
  for (const auto& c : cands) any_h4 = any_h4 || c.is_h4;
  ASSERT_TRUE(any_h4) << "guard: the fixture must offer a finesse";

  auto pick = hanabi::reactor0::choose_h4_clue(g, cands);
  ASSERT_TRUE(pick.has_value()) << "an available H4 clue is Precedence step 1";
  EXPECT_TRUE(is_rank_to(pick, 5, 2))
      << "rank 5 to Cathy is the finesse; got " << describe(pick);
}

// --- priority 3's double-discard rungs (3.2 and 3.4) ---------------------

// A double discard that throws away two cards nobody needs appears TWICE in
// priority 3, and the difference between the two positions is Cathy's chop.
//
// At 3.2 — above the stable discard clue — it is doing two jobs: clearing two
// unwanted cards, and redirecting Cathy off a chop she could not afford to
// lose. At 3.4, below the stable discard, only the first job is left, because
// Cathy's chop was expendable anyway.
//
// One fixture, one card changed, so the ordering is what moves.
//
// Stacks r=1. Bob `g3 r1 b3 g4 y4`, chop g3 on slot 1 — non-trash and with no
// safe action, which is priority 3's precondition. Rank 5 to Cathy is a Phase C
// double discard calling Bob's r1 (slot 2) and Cathy's r1 (slot 3), both basic
// trash, so neither costs the team anything.
namespace {

Game double_discard_position(std::vector<std::string> cathy_hand) {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g3", "r1", "b3", "g4", "y4"},
      std::move(cathy_hand),
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return setup(std::move(opts));
}

}  // namespace

TEST(Reactor0CluePriority, DoubleDiscardOutranksStableDiscardWhenCathyChopIsWorthKeeping) {
  Game g = double_discard_position({"y4", "b4", "r1", "g4", "p5"});
  ASSERT_FALSE(hanabi::reactor0::chop_is_expendable(g, 2))
      << "guard: Cathy's y4 chop is neither trash nor a same-hand-dupe";

  auto cands = analysed(g);
  ASSERT_TRUE(has_shape(cands, ClueShape::DOUBLE_DISCARD))
      << "guard: the fixture must offer a double discard";

  auto pick = hanabi::reactor0::choose_clue(g, cands);
  ASSERT_TRUE(pick.has_value());
  EXPECT_TRUE(is_rank_to(pick, 5, 2))
      << "rung 3.2 takes the double discard, which also saves Cathy's chop; got "
      << describe(pick);
}

TEST(Reactor0CluePriority, DoubleDiscardDropsBelowStableDiscardWhenCathyChopIsExpendable) {
  // The only change: Cathy's chop becomes a same-hand-dupe (g4 on slots 1 and
  // 4), so rung 3.2's condition fails and the stable discard at 3.3 goes first.
  // Bob's g4 becomes p4 to keep the deck legal.
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g3", "r1", "b3", "p4", "y4"},
      {"g4", "b4", "r1", "g4", "p5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  ASSERT_TRUE(hanabi::reactor0::chop_is_expendable(g, 2))
      << "guard: Cathy's g4 chop is duplicated in her own hand";

  auto cands = analysed(g);
  ASSERT_TRUE(has_shape(cands, ClueShape::DOUBLE_DISCARD))
      << "guard: the double discard is still on offer — it just ranks lower";

  auto pick = hanabi::reactor0::choose_clue(g, cands);
  ASSERT_TRUE(pick.has_value());
  auto* rank = std::get_if<PerformRank>(&*pick);
  ASSERT_NE(rank, nullptr) << "got " << describe(pick);
  EXPECT_EQ(rank->target, 1)
      << "rung 3.3's stable discard to Bob now goes first; got " << describe(pick);
}

// The gate on rung 3.2, pinned directly. It is H1c's "expendable chop" clause
// reused, so both readings of "Cathy can afford to lose it" are covered.
TEST(Reactor0CluePriority, ChopIsExpendableCoversTrashAndSameHandDupe) {
  Game trash_chop = double_discard_position({"r1", "b4", "y3", "g4", "p5"});
  EXPECT_TRUE(hanabi::reactor0::chop_is_expendable(trash_chop, 2))
      << "r1 is basic trash on a red stack of 1";

  Game duped_chop = double_discard_position({"b4", "y3", "r1", "g4", "b4"});
  EXPECT_TRUE(hanabi::reactor0::chop_is_expendable(duped_chop, 2))
      << "b4 is duplicated inside Cathy's own hand";

  Game keeper = double_discard_position({"y4", "b4", "r1", "g4", "p5"});
  EXPECT_FALSE(hanabi::reactor0::chop_is_expendable(keeper, 2))
      << "y4 is useful and unduplicated in her hand";
}

// --- section 4's stall rungs (4.4 fill-in, 4.5 safe stall) ---------------

// A fill-in matters exactly when Bob's hand is FULLY CLUED: there is nothing
// left to call, but his cards are still ambiguous, so the forced token can buy
// him information instead of committing his hand.
//
// This is also why 4.4 and 4.5 sit above the lock. A re-clue of already-clued
// cards reads as a LOCK in reactor0, so a lock candidate exists in essentially
// every 8-token position; with the lock above them, neither rung could ever
// run. The fixture below is the proof — `rank 4` here IS a lock candidate.
namespace {

Game fully_clued_bob_at_eight_tokens() {
  SetupOptions opts;
  // Every 1 is played, so a card Bob knows is a 1 is known TRASH -- that is his
  // safe discard, and it is what stops priority 3 applying. Without it, rung
  // 3.9's lock fires and section 4 is never reached at all.
  opts.play_stacks = {1, 1, 1, 1, 1};
  opts.clue_tokens = 8;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r3", "y3", "g4", "b4", "r1"},
      {"g3", "b3", "p3", "r4", "y4"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  g = pre_clue(std::move(g), TestPlayer::BOB, 5, {"1"});  // his known trash
  g = pre_clue(std::move(g), TestPlayer::BOB, 1, {"3"});  // slots 1-2
  g = pre_clue(std::move(g), TestPlayer::BOB, 3, {"4"});  // slots 3-4
  return g;
}

}  // namespace

TEST(Reactor0CluePriority, FillInOutranksTheLockAtEightTokens) {
  Game g = fully_clued_bob_at_eight_tokens();
  auto cands = analysed(g);

  // Both rungs really are on offer, which is what makes the ordering the
  // subject of this test rather than an accident of the fixture.
  ASSERT_TRUE(has_shape(cands, ClueShape::STABLE_LOCK))
      << "guard: a lock candidate exists, as it does in almost every 8-token "
         "position";
  bool any_fill_in = false;
  for (const auto& c : cands) any_fill_in = any_fill_in || !c.fill_ins.empty();
  ASSERT_TRUE(any_fill_in) << "guard: a fill-in is available too";

  auto pick = hanabi::reactor0::choose_clue(g, cands);
  ASSERT_TRUE(pick.has_value());
  // The chosen clue must be one that fills in, not the lock.
  const ClueCandidate* chosen = nullptr;
  for (const auto& c : cands) {
    if (describe(std::optional<PerformAction>{c.perform}) == describe(pick)) {
      chosen = &c;
    }
  }
  ASSERT_NE(chosen, nullptr);
  EXPECT_FALSE(chosen->fill_ins.empty())
      << "rung 4.4 takes a fill-in ahead of the lock at 4.6; got "
      << describe(pick) << " (" << hanabi::reactor0::shape_name(chosen->reading.shape)
      << ")";
  EXPECT_NE(chosen->reading.shape, ClueShape::STABLE_LOCK);
}

// Fill-ins only count ALREADY-CLUED, UNPLAYABLE cards. A clue that merely
// touches a fresh card is a normal clue, not a stall, and the rungs above have
// first refusal on it.
TEST(Reactor0CluePriority, FillInsCountOnlyCluedUnplayableCards) {
  Game g = fully_clued_bob_at_eight_tokens();
  for (const auto& c : analysed(g)) {
    for (int o : c.fill_ins) {
      EXPECT_TRUE(g.state.deck[o].clued)
          << "a fill-in narrows a card that was already clued";
      auto id = g.state.deck[o].id();
      ASSERT_TRUE(id.has_value());
      EXPECT_FALSE(g.state.is_playable(*id))
          << "a playable card is not a fill-in subject — it is a play clue";
    }
  }
}

// Rung 4.5's safe stall designates nothing at all, which is what makes it safe:
// a clue that stamps no call cannot be read as an instruction to play or
// discard, so it cannot strike or lose a critical card.
TEST(Reactor0CluePriority, SafeStallDesignatesNothing) {
  Game g = fully_clued_bob_at_eight_tokens();
  for (const auto& c : analysed(g)) {
    if (c.reading.shape != ClueShape::OTHER) continue;
    EXPECT_LT(c.reading.reacter_side.order, 0);
    EXPECT_LT(c.reading.receiver_side.order, 0);
    EXPECT_LT(c.reading.stable_subject, 0)
        << "an OTHER reading names no card, which is the whole of its safety";
  }
}
