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

// --- the ditch-target rule ------------------------------------------------
//
// One ordering decides WHICH of Bob's cards a clue makes him throw, wherever the
// question is asked (rungs 3.8 / 4.8, rung 3.9, section 4's floor): the largest
// `ditch_connectors`, then the highest rank, then the leftmost card. These pin
// the three keys in turn.

// Key 1. Basic trash scores 999, so it always outranks a card the team might
// still play, however many of its connectors are invisible. Replay 1981749 T17
// is the position that needed it: `missing_connectors` alone scored Bob's trash
// b3 at ZERO and spent an r3 the team still wanted instead.
TEST(Reactor0CluePriority, DitchTargetSpendsTrashAheadOfAnythingPlayable) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"b2", "b4", "y3", "g3", "p3"},
      {"r3", "r4", "y5", "g5", "p5"},
  };
  opts.play_stacks = {0, 0, 0, 3, 0};  // blue on 3: b2 is dead, b4 is next
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int trash = order_of(g, TestPlayer::BOB, 1);   // b2, blue already on 3
  const int wanted = order_of(g, TestPlayer::BOB, 3);  // y3, yellow on 0
  EXPECT_EQ(hanabi::reactor0::ditch_connectors(g, trash), 999)
      << "a card nobody can ever need is the one to spend";
  EXPECT_EQ(hanabi::reactor0::missing_connectors(g, trash), 0)
      << "the plain metric is unchanged -- rungs 4.4 and 3.7 still read it";
  EXPECT_TRUE(hanabi::reactor0::better_ditch_target(
      g, 1 /* Bob */, trash, wanted));
  EXPECT_FALSE(hanabi::reactor0::better_ditch_target(
      g, 1 /* Bob */, wanted, trash));
}

// Key 2. Equal connectors -> the HIGHER rank goes.
TEST(Reactor0CluePriority, DitchTargetBreaksEqualConnectorsOnRank) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y2", "g4", "b3", "p3", "r3"},
      {"g3", "g2", "y5", "b5", "p5"},
  };
  opts.play_stacks = {0, 1, 2, 0, 0};  // yellow on 1, green on 2
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int y2 = order_of(g, TestPlayer::BOB, 1);  // yellow on 1 -> playable
  const int g4 = order_of(g, TestPlayer::BOB, 2);  // green on 2, g3 in Cathy's
  ASSERT_EQ(hanabi::reactor0::ditch_connectors(g, y2),
            hanabi::reactor0::ditch_connectors(g, g4))
      << "guard: the fixture only separates these two on rank";
  EXPECT_TRUE(hanabi::reactor0::better_ditch_target(
      g, 1 /* Bob */, g4, y2))
      << "same connectors, so the 4 goes before the 2";
}

// Key 3. Equal on both -> the LEFTMOST (newest) card goes.
TEST(Reactor0CluePriority, DitchTargetBreaksAnExactTieOnLeftmost) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "g5", "b5", "p5", "y3"},
      {"r3", "r4", "g2", "b2", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int left = order_of(g, TestPlayer::BOB, 1);
  const int right = order_of(g, TestPlayer::BOB, 5);
  ASSERT_EQ(hanabi::reactor0::ditch_connectors(g, left),
            hanabi::reactor0::ditch_connectors(g, right))
      << "guard: two copies of the same identity in the same hand";
  EXPECT_TRUE(hanabi::reactor0::better_ditch_target(
      g, 1 /* Bob */, left, right))
      << "nothing else separates them, so slot 1 goes";
  EXPECT_FALSE(hanabi::reactor0::better_ditch_target(
      g, 1 /* Bob */, right, left));
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

// --- VERY HIGH and Precedence ---------------------------------------------

// `choose_very_high_clue` is Precedence step 1, and it must NOT apply section
// 4's floor: a VERY HIGH clue outranks a pending reaction, an arbitrary one does
// not. There is no finesse available here at all, so it must decline even at 8
// tokens, where `choose_clue` would happily return the floor's pick.
TEST(Reactor0CluePriority, VeryHighDeclinesWithoutOneEvenAtEightTokens) {
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
  for (const auto& c : cands) {
    ASSERT_NE(c.tier, hanabi::reactor0::ClueTier::VERY_HIGH)
        << "guard: the fixture must offer no VERY HIGH clue at all";
  }
  EXPECT_FALSE(hanabi::reactor0::choose_very_high_clue(g, cands).has_value())
      << "nothing here is VERY HIGH, so nothing may outrank a pending reaction";
}

// And the positive case: the Phase B finesse fixture from test_clue_tier.cpp,
// which VH1 marks VERY HIGH and `choose_very_high_clue` therefore offers.
TEST(Reactor0CluePriority, VeryHighOffersTheFinesse) {
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
  bool any_vh = false;
  for (const auto& c : cands) {
    any_vh = any_vh || c.tier == hanabi::reactor0::ClueTier::VERY_HIGH;
  }
  ASSERT_TRUE(any_vh) << "guard: the fixture must offer a finesse";

  auto pick = hanabi::reactor0::choose_very_high_clue(g, cands);
  ASSERT_TRUE(pick.has_value())
      << "an available VERY HIGH clue is Precedence step 1";
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

// --- 3.3 / 4.2's safe-discard condition ----------------------------------

// "Bob already has a safe discard" is NOT the same as "Bob has known trash",
// and the difference is the whole reason the condition needs its own predicate.
// In an inverted variant Discard is a play ATTEMPT, so a card the holder knows
// is a dead Orange 1 is known trash and still has no safe discard button:
// chucking it strikes. Only trash on a PLAIN suit can simply be thrown away.
TEST(Reactor0CluePriority, KnownDeadOrangeIsNotASafeDiscard) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"r3", "b4", "g4", "r5", "b5"},
      {"o1", "r4", "g3", "b3", "r2"},
      {"g2", "b2", "r3", "g5", "o4"},
  };
  opts.play_stacks = {0, 0, 0, 2};  // orange past 1, so o1 is dead
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 3;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted);
  ASSERT_TRUE(g.state.is_basic_trash(Identity{orange, 1}))
      << "guard: the Orange 1 really is trash";
  ASSERT_FALSE(g.state.is_playable(Identity{orange, 1}))
      << "guard: and not playable, so a chuck would strike";

  // Bob's slot 1, known to everyone to be exactly that dead Orange 1.
  const int o1 = order_at(g, TestPlayer::BOB, 1);
  g.with_thought(o1, [orange](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(Identity{orange, 1});
    return out;
  });

  EXPECT_FALSE(hanabi::reactor0::has_safe_discard(g, 1))
      << "known trash on an INVERTED suit has no safe discard button";

  // Give him a dead card on a plain suit instead, and he does.
  g.with_thought(o1, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(Identity{0, 1});  // r1, plain
    return out;
  });
  g.state.play_stacks[0] = 2;  // r1 now dead
  EXPECT_TRUE(hanabi::reactor0::has_safe_discard(g, 1))
      << "trash on a plain suit is simply thrown away";
}

// --- priority 3's precondition: the chop must actually be worth a clue -----

// "Non-trash" was too weak. Priority 3 spends a clue on Bob's chop, and there
// are exactly two reasons to: the card is in DANGER (`at_risk_chop`, the same
// test H1a uses), or it is a PLAY the team should collect (`has_playable_chop`,
// N5's test). A chop that is neither earns nothing.
//
// Replay 1966745 T5 is the case that was wrong: Bob's chop was an r2 with red
// on 0 -- unplayable -- and Cathy held the other r2, so throwing it cost the
// team nothing. Priority 3 fired anyway and spent a clue.
//
// One fixture, one card changed, so the precondition is what moves. Bob's chop
// is a b2; Cathy either holds the second copy (nothing at stake) or does not.
namespace {

Game bob_chop_position(std::string cathy_slot_1) {
  SetupOptions opts;
  opts.play_stacks = {1, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob: chop b2 on slot 1, and no safe action anywhere else.
      {"b2", "g3", "b3", "g4", "y4"},
      {std::move(cathy_slot_1), "p3", "p4", "y3", "p5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return setup(std::move(opts));
}

}  // namespace

TEST(Reactor0CluePriority, PriorityThreeSkipsAChopDuplicatedInCathysHand) {
  Game g = bob_chop_position("b2");
  const int bob = static_cast<int>(TestPlayer::BOB);

  ASSERT_FALSE(hanabi::reactor0::at_risk_chop(g, 0, bob))
      << "guard: Cathy holds the other b2, so Bob's chop is in no danger";
  ASSERT_FALSE(hanabi::reactor0::has_playable_chop(g, bob))
      << "guard: b2 is not playable with blue on 0, so there is no play to "
         "collect either";

  EXPECT_FALSE(hanabi::reactor0::priority_3_applies(g))
      << "neither endangered nor playable -- priority 3 has no business here";
}

// The positive it is carved out of: remove the duplicate and the very same chop
// IS endangered, so priority 3 applies again.
TEST(Reactor0CluePriority, PriorityThreeAppliesWhenTheChopIsGenuinelyAtRisk) {
  Game g = bob_chop_position("p2");
  const int bob = static_cast<int>(TestPlayer::BOB);

  ASSERT_TRUE(hanabi::reactor0::at_risk_chop(g, 0, bob))
      << "guard: the only other b2 is gone, so Bob's chop is now at risk";

  EXPECT_TRUE(hanabi::reactor0::priority_3_applies(g));
}

// And the other arm: a chop that is safe but PLAYABLE still earns a clue, which
// is why the precondition is a disjunction rather than `at_risk_chop` alone.
// Replay 1942330 T33 turns on this -- Bob's playable Navy 2 was duplicated in
// Cathy's hand, and the Blue play clue that collects it is still right.
TEST(Reactor0CluePriority, PriorityThreeAppliesToASafeButPlayableChop) {
  SetupOptions opts;
  opts.play_stacks = {1, 1, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      // Bob's chop y2 is playable on a yellow stack of 1.
      {"y2", "g3", "b3", "g4", "p4"},
      // Cathy holds the other y2, so the chop is NOT at risk.
      {"y2", "p3", "b4", "y3", "p5"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  const int bob = static_cast<int>(TestPlayer::BOB);

  ASSERT_FALSE(hanabi::reactor0::at_risk_chop(g, 0, bob))
      << "guard: the duplicate in Cathy's hand keeps it out of danger";
  ASSERT_TRUE(hanabi::reactor0::has_playable_chop(g, bob))
      << "guard: but it is playable";

  EXPECT_TRUE(hanabi::reactor0::priority_3_applies(g))
      << "a play the team should collect is a reason of its own";
}

// --- the endgame stall list -----------------------------------------------
//
// `choose_endgame_clue` is a DIFFERENT ordering from `choose_clue`, used when
// the endgame fork has already decided the turn is a clue. The ordinary rungs
// are tuned for a game still being played and put a reactive discard at rung 2,
// above any stable play; once there is no long run left to feed, a legal stable
// play is worth more. Replay 1971808 T59 is the case that forced the split.
namespace {

std::optional<PerformAction> endgame_chosen(const Game& g) {
  return hanabi::reactor0::choose_endgame_clue(g, analysed(g));
}

bool is_colour_to(const std::optional<PerformAction>& a, int value, int target) {
  auto* c = a ? std::get_if<PerformColour>(&*a) : nullptr;
  return c && c->value == value && c->target == target;
}

// The analysed candidate a chooser returned, matched by its rendered form --
// the same trick `ChosenClueNeverPredictsAStrikeBelowEightTokens` uses.
const ClueCandidate* candidate_for(const std::vector<ClueCandidate>& cs,
                                   const std::optional<PerformAction>& pick) {
  if (!pick) return nullptr;
  for (const auto& c : cs) {
    if (describe(std::optional<PerformAction>{c.perform}) == describe(pick)) {
      return &c;
    }
  }
  return nullptr;
}

bool reading_strikes(const hanabi::reactor0::ClueReading& r) {
  return r.reacter_side.outcome == hanabi::reactor0::Outcome::STRIKE ||
         r.receiver_side.outcome == hanabi::reactor0::Outcome::STRIKE ||
         r.stable_outcome == hanabi::reactor0::Outcome::STRIKE;
}

}  // namespace

// Rung 1 is shared with `choose_clue`: two cards down beats everything.
TEST(Reactor0CluePriority, EndgameTakesTheDoubleReactiveFirst) {
  SetupOptions opts;
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
  ASSERT_TRUE(has_shape(cands, ClueShape::REACTIVE_PLAY)) << "guard";
  EXPECT_TRUE(is_rank_to(endgame_chosen(g), 3, 2))
      << "got " << describe(endgame_chosen(g));
}

// The ordering that DIFFERS from `choose_clue`: a legal stable play to Bob
// outranks a reactive discard. `choose_clue` would take the reactive discard at
// its rung 2.
TEST(Reactor0CluePriority, EndgameStablePlayOutranksReactiveDiscard) {
  SetupOptions opts;
  // Green to Bob names his playable g1 -- a legal stable play. Green to Cathy
  // is the ODD parity in a plain variant ("exactly one play"), and the pairing
  // `react_slot + target_slot = anchor` with Green's value of 3 sends Bob to
  // his slot 1 (the g1) and Cathy to her slot 2, a trash p1 she throws.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "y4", "b4", "p4", "y3"},
      {"y3", "p1", "g3", "b3", "y4"},
  };
  opts.play_stacks = {0, 0, 0, 0, 1};  // p1 played, so Cathy's slot 2 is trash
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_TRUE(has_shape(cands, ClueShape::STABLE_PLAY))
      << "guard: the fixture must offer a legal stable play to Bob";
  ASSERT_TRUE(has_shape(cands, ClueShape::REACTIVE_DISCARD))
      << "guard: and a reactive discard, or there is no ordering to test -- "
         "`choose_clue` would take the reactive discard at ITS rung 2";

  auto pick = endgame_chosen(g);
  ASSERT_TRUE(pick.has_value());
  const ClueCandidate* won = candidate_for(cands, pick);
  ASSERT_NE(won, nullptr);
  EXPECT_EQ(won->reading.shape, ClueShape::STABLE_PLAY)
      << "the stall list puts a legal stable play above a reactive discard; got "
      << describe(pick);
  EXPECT_EQ(won->action.target, 1) << "...and a stable clue goes to Bob";
}

// The veto, end to end. `select` drops anything predicting a strike at every
// rung, so a stable clue whose named card is NOT the playable one can never
// come back -- which is exactly what red-to-Bob was at 1971808 T59.
TEST(Reactor0CluePriority, EndgameNeverReturnsAStrikePredictingClue) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "y4", "g4", "b4", "r5"},   // Bob: leftmost red is the trash r1
      {"y3", "g3", "b3", "p3", "y4"},
  };
  opts.play_stacks = {4, 0, 0, 0, 0};   // red on 4, so r1 is trash and r5 plays
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  int strikers = 0;
  for (const auto& c : cands) {
    if (reading_strikes(c.reading)) ++strikers;
  }
  ASSERT_GT(strikers, 0)
      << "guard: the fixture must offer at least one clue whose reading makes "
         "someone bomb, or this proves nothing";

  auto pick = endgame_chosen(g);
  ASSERT_TRUE(pick.has_value());
  const ClueCandidate* won = candidate_for(cands, pick);
  ASSERT_NE(won, nullptr);
  EXPECT_FALSE(reading_strikes(won->reading))
      << "a clue that makes a partner bomb must never be the answer; got "
      << describe(pick);
}

// Rung 3 counts NEGATIVE information: a colour clue can make a card Bob's
// empathy reads as entirely useful without touching it at all.
TEST(Reactor0CluePriority, EndgameRungThreeCountsNegativeInformation) {
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

  auto cands = analysed(g);

  // The load-bearing claim is the DEFINITION: `newly_useful` may only be set
  // when the target's own view actually changed. That is what makes negative
  // information count -- the test does not care whether the clue touched the
  // card, only that the target learned something about it.
  for (const auto& c : cands) {
    if (!c.newly_useful) continue;
    const Game hypo = g.simulate(Action{c.action});
    bool differs = false;
    for (int o : g.state.hands[c.action.target]) {
      const Thought& a = g.players[c.action.target].thoughts[o];
      const Thought& b = hypo.players[c.action.target].thoughts[o];
      if (!(a.possible == b.possible && a.inferred == b.inferred)) differs = true;
    }
    EXPECT_TRUE(differs)
        << "newly_useful claims the target learned something, so his view must "
           "have changed: " << describe(c.perform);
  }
}

// The chooser is a floor as well as a ladder: while any candidate avoids a
// strike, it returns one. Declining would hand the turn back to a clue nobody
// vetted.
TEST(Reactor0CluePriority, EndgameAlwaysAnswersWhenSomethingIsSafe) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y4", "g4", "b4", "p4", "y3"},
      {"g3", "b3", "p3", "y2", "p2"},
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 1;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_FALSE(cands.empty()) << "guard: there are legal clues here";
  EXPECT_TRUE(endgame_chosen(g).has_value())
      << "the stall list must not decline while a safe clue exists";
}

// The 1971808 counterfactual: with the reactive double play removed, rung 2
// takes over. This is the case that depends on v8.8.0's rightmost rule --
// under Odds and Evens a rank clue names a parity class, and the direct-play
// focus is the RIGHTMOST newly touched card that could be playable. Here that
// is the r5; the leftmost is a trash y3, which is what made the equivalent
// COLOUR clue illegal in the replay.
TEST(Reactor0CluePriority, EndgameOddRankToBobWhenNoReactiveExists) {
  SetupOptions opts;
  opts.variant_name = "Odds and Evens (5 Suits)";
  opts.play_stacks = {4, 5, 4, 5, 5};   // r5 and g5 are the only cards left
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y3", "r1", "g2", "r5", "p4"},   // Bob: odd touches y3, r1, r5
      {"y1", "b2", "p2", "y2", "b4"},   // Cathy: nothing that pairs into a play
  };
  opts.clue_tokens = 3;
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  auto cands = analysed(g);
  ASSERT_FALSE(has_shape(cands, ClueShape::REACTIVE_PLAY))
      << "guard: rung 1 must be empty, or it wins and this proves nothing";

  auto pick = endgame_chosen(g);
  ASSERT_TRUE(pick.has_value());
  const ClueCandidate* won = candidate_for(cands, pick);
  ASSERT_NE(won, nullptr);
  EXPECT_FALSE(reading_strikes(won->reading));
  EXPECT_TRUE(is_rank_to(pick, 1, 1))
      << "an ODD rank clue to Bob names his rightmost touched card that could "
         "be playable -- the r5. Got " << describe(pick);
}

// --- rung 3.7's Cathy clauses at two seats --------------------------------
//
// 3.7 vetoes its lock when "Bob can give a stable colour play clue to Cathy".
// At two seats there IS no Cathy: `cathy_of` is `next_player_index(bob)`, which
// wraps straight back to Alice. So the unguarded question is "could Bob colour-
// clue ALICE a playable card?" -- a real question, usually true, and entirely
// the wrong one. It suppressed the rung on every two-player position where
// Alice happened to hold something playable.
//
// The doc says both Cathy clauses are vacuous at two seats, which is also how
// H1b/H1c have always read theirs (`state_eval.cpp:537-538`).
//
// The corpus cannot see this: 1 of the 1,991 recorded games is two-player, so
// no sweep would ever have moved. This test is the only thing watching it.
TEST(Reactor0CluePriority, TwoSeatRungThreeSevenIgnoresTheAbsentCathy) {
  SetupOptions opts;
  opts.play_stacks = {0, 0, 0, 0, 0};
  // 7, not 8: at 8 tokens section 4 opens and hands back a lock of its own at
  // 4.6, which would mask the rung under test.
  opts.clue_tokens = 7;
  opts.hands = {
      // Every 1, so Bob could colour-clue Alice a playable card in five
      // different ways -- the veto's trigger, if it were asked about Alice.
      {"r1", "y1", "g1", "b1", "p1"},
      // Chop (slot 1) is the last y5. The four 2s each sit one connector from
      // playable and Alice can see every connector, so all four count toward
      // 3.7's ">= 3 cards with at most one missing connector".
      {"y5", "r2", "g2", "b2", "p2"},
  };
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  // One clued card, so a re-clue has something to read as a lock.
  g = pre_clue(std::move(g), TestPlayer::BOB, 2, {"2"});
  const int bob = static_cast<int>(TestPlayer::BOB);

  ASSERT_EQ(g.state.num_players, 2) << "guard: the whole point is two seats";
  ASSERT_TRUE(hanabi::reactor0::priority_3_applies(g))
      << "guard: Bob is stuck on an endangered chop, so section 3 runs";
  ASSERT_TRUE(hanabi::reactor0::has_colour_play_clue_for(g, bob, 0))
      << "guard: the trap is armed -- Bob CAN colour-clue seat 0 a playable "
         "card, which is what `cathy_of` wraps round to here";

  auto cands = analysed(g);
  ASSERT_TRUE(has_shape(cands, ClueShape::STABLE_LOCK))
      << "guard: a lock candidate exists, so 3.7 has something to return";
  ASSERT_TRUE(has_shape(cands, ClueShape::STABLE_DISCARD))
      << "guard: 3.9's stable discard is also on offer, so choosing the lock "
         "is a real preference and not the only option";

  auto pick = chosen(g);
  const ClueCandidate* got = nullptr;
  for (const auto& c : cands) {
    if (describe(std::optional<PerformAction>{c.perform}) == describe(pick)) {
      got = &c;
    }
  }
  ASSERT_NE(got, nullptr) << "chose " << describe(pick);
  EXPECT_EQ(got->reading.shape, ClueShape::STABLE_LOCK)
      << "3.7 must fire: with no Cathy its two Cathy clauses are vacuous. Got "
      << describe(pick) << " (" << hanabi::reactor0::shape_name(got->reading.shape)
      << ") -- stable_discard means the veto asked about Alice and 3.9 answered "
         "instead.";
}
