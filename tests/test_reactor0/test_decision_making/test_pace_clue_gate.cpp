// Reactor0's pace-clue tier gate (`clue_is_admissible`,
// src/conventions/reactor0/decision.cpp; DECISION_MAKING.md "Decision phase 1"
// items 1 and 2) — the window it fires in, and which tier it demands.
//
//   window: pace() >= 1, and clue_tokens <= 3, or clue_tokens < 8 when Alice
//           is occupied
//   required tier: HIGH when Alice holds a card stamped CALLED_TO_PLAY, or —
//                  in a variant containing an inverted suit — a card stamped
//                  CALLED_TO_DISCARD. Otherwise NOT-LOW (HIGH or MEDIUM).
//
// Unlike reactor's low-clue-count gate this window is one token wider
// (`<= 3`, not `< 3`) and has NO "we hold a real play" conjunct, so it also
// fires when Alice has nothing queued.
//
// v7.0.0 NOTE. These assertions used to read the gate off a SCORE: reactor0's
// `eval_action` returned a flat -1.0 for a clue below its required tier, so
// `EXPECT_EQ(eval_action(...), -1.0)` meant "rejected". That scorer is deleted,
// and the gate is now a predicate the priority walk consults directly, so the
// same boundaries are asserted as `admissible(...)` — false where the score
// used to be -1.0, true where it used to be anything else. Every boundary
// pinned before is pinned here; only the observable changed. The one call to
// REACTOR's `eval_action` further down is untouched, because reactor keeps its
// scorer.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/state_eval.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueTier;

namespace {

Action make_clue(const Game& g, int giver, int target, ClueKind kind,
                 int value) {
  auto touched = g.state.clue_touched(g.state.hands[target], kind, value);
  return Action{
      ClueAction{giver, target, std::move(touched), BaseClue{kind, value}}};
}

ClueTier tier_of(const Game& g, const Action& clue) {
  Game hypo = g.simulate(clue);
  return hanabi::reactor0::clue_tier(g, hypo, std::get<ClueAction>(clue));
}

// Does the gate let this clue through? Built through `analyse_clues` rather
// than by hand, so the test exercises the same construction `take_action`
// hands to the priority walk.
bool admissible(const Game& g, const Action& clue) {
  const auto& ca = std::get<ClueAction>(clue);
  PerformAction perform =
      ca.clue.kind == ClueKind::COLOUR
          ? PerformAction{PerformColour{ca.target, ca.clue.value}}
          : PerformAction{PerformRank{ca.target, ca.clue.value}};
  auto cands = hanabi::reactor0::analyse_clues(g, {{perform, clue}});
  // `analyse_clues` drops a MISTAKE outright, which is a stronger rejection
  // than the gate's; no fixture here produces one.
  EXPECT_EQ(cands.size(), 1u) << "guard: the candidate survived analysis";
  if (cands.size() != 1) return false;
  return hanabi::reactor0::clue_is_admissible(g, cands.front());
}

// A position where every hand is inert: Bob's chop is duplicated in his own
// hand and Cathy's likewise, so H1 and N2/N3 are all dead and any clue that
// generates no plays classifies LOW. `clue_tokens` and `discarded` are left
// for the caller to set.
SetupOptions inert_position() {
  SetupOptions opts;
  opts.hands = {
      {"y1", "g1", "b1", "p1", "y5"},
      {"p4", "p4", "g4", "b4", "y4"},  // Bob chop duped
      {"b3", "b3", "g3", "y3", "p3"},  // Cathy chop duped
  };
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  return opts;
}

// Ten rank-1 discards; every 1 has three copies, so this drops `pace` by 10
// without lowering `max_score`.
const std::vector<std::string> kTenOnes = {"r1", "r1", "y1", "y1", "g1",
                                           "g1", "b1", "b1", "p1", "p1"};

// `inert_position` starts at pace 13, so kTenOnes lands on 3. Each rank-2 has
// two copies, and discarding ONE of a suit leaves a copy, so max_score does not
// move and pace falls by exactly one per card.
std::vector<std::string> discards_for_pace(int target_pace) {
  static const std::vector<std::string> kExtras = {"r2", "y2", "g2", "b2", "p2"};
  std::vector<std::string> out = kTenOnes;
  for (int i = 0; i < 3 - target_pace; ++i) out.push_back(kExtras[i]);
  return out;
}

}  // namespace

// --- the window ----------------------------------------------------------

// The headline delta from reactor: reactor's gate is `clue_tokens < 3`, so it
// never fires at exactly 3. Reactor0's `<= 3` does.
TEST(Reactor0PaceClueGate, FiresAtThreeClueTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 3;
  Game g = setup(std::move(opts));

  ASSERT_GE(g.state.pace(), 1) << "guard: window needs pace >= 1";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: this clue must be LOW";

  EXPECT_FALSE(admissible(g, clue))
      << "at exactly 3 clues the reactor0 gate is active and rejects a LOW "
         "clue; reactor's `< 3` window would have let it through.";
}

// One token above the window the gate is silent and every clue is admissible.
TEST(Reactor0PaceClueGate, SilentAtFourClueTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));

  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "clue_tokens == 4 is outside the window; the gate must not fire.";
}

// Only at pace 0 is the gate silent. The threshold was `pace() >= 3` through
// v7.3.0, inherited from reactor's low-clue-count gate; replay 1966119 T5 showed
// the hole that left, so it is now `pace() >= 1`. Pace 0 is the true floor:
// there, every remaining turn must produce a play or the game cannot finish, so
// hoarding a token for a better clue is pointless.
TEST(Reactor0PaceClueGate, SilentAtPaceZero) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(0);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 0) << "guard: fixture must sit just below the window";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "pace == 0 is outside the window; the gate must not fire.";
}

// ...and fires at exactly pace 1, the new boundary.
TEST(Reactor0PaceClueGate, FiresAtPaceOne) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(1);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 1) << "guard: fixture must sit on the boundary";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_FALSE(admissible(g, clue))
      << "pace == 1 is inside the window; a LOW clue must be rejected.";
}

// The pace that used to be the boundary is now well inside the window. Kept
// because it is the position replay 1966119 T5 sat at (pace 2, occupied, a LOW
// reactive discard admitted) and the whole point of the change is that it is
// now refused.
TEST(Reactor0PaceClueGate, FiresAtPaceTwoWhichUsedToBeOutsideTheWindow) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(2);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 2) << "guard: the old gate stood down here";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_FALSE(admissible(g, clue))
      << "pace == 2 is inside the window now; a LOW clue must be rejected.";
}

// --- reactor is untouched ------------------------------------------------

// Insurance for the "reactor's behaviour is unchanged" contract: the same
// position at 3 clue tokens, scored by reactor, must NOT be rejected —
// reactor's window is `< 3`. Pins the contract without editing reactor's own
// tests.
TEST(Reactor0PaceClueGate, ReactorGateStillStopsShortOfThreeTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 3;
  opts.init = [](Game&) {};  // stay on reactor
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.convention, Convention::REACTOR) << "guard: reactor fixture";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  EXPECT_NE(hanabi::reactor::eval_action(g, clue), -1.0)
      << "reactor's low-clue-count gate is `clue_tokens < 3` and must stay "
         "that way.";
}

// --- which tier is required ----------------------------------------------

// Decision #3, the "no" half: an empathy-known playable that carries NO
// convention stamp must NOT raise the bar to HIGH. This is the assertion most
// likely to catch an implementation that reaches for `obvious_playables`
// (which is what reactor's gate keys on).
TEST(Reactor0PaceClueGate, ObviousPlayableWithoutStampDoesNotRequireHigh) {
  SetupOptions opts = inert_position();
  opts.hands[2] = {"r3", "y3", "g3", "b3", "p3"};  // Cathy chop r3, endangered
  opts.clue_tokens = 3;
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/1, "y1");
  g.elim();

  ASSERT_FALSE(g.common.obvious_playables(g, 0).empty())
      << "guard: Alice holds a known playable y1 carrying no stamp";
  ASSERT_EQ(g.meta[order_at(g, TestPlayer::ALICE, 1)].status, CardStatus::NONE)
      << "guard: that playable must be unstamped";
  ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g))
      << "an unstamped playable must not force the HIGH tier";
}

// The "yes" half: a literal CALLED_TO_PLAY stamp does force HIGH, and a
// MEDIUM clue is then rejected.
TEST(Reactor0PaceClueGate, CtpStampForcesHighTier) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 3;
  Game g = setup(std::move(opts));

  ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g))
      << "guard: no stamps yet";
  g.meta[order_at(g, TestPlayer::ALICE, 1)].status =
      CardStatus::CALLED_TO_PLAY;
  EXPECT_TRUE(hanabi::reactor0::requires_high_tier(g))
      << "a CALLED_TO_PLAY stamp in Alice's hand forces the HIGH-only tier.";
}

// A CALLED_TO_DISCARD stamp only counts where discarding can be a play —
// i.e. in a variant containing an inverted suit.
TEST(Reactor0PaceClueGate, CtdStampForcesHighTierOnlyWhenInverted) {
  SetupOptions plain = inert_position();
  plain.clue_tokens = 3;
  Game g_plain = setup(std::move(plain));
  g_plain.meta[order_at(g_plain, TestPlayer::ALICE, 1)].status =
      CardStatus::CALLED_TO_DISCARD;
  EXPECT_FALSE(hanabi::reactor0::requires_high_tier(g_plain))
      << "No Variant has no inverted suit, so a CTD is not an action.";

  // "Orange (3 Suits)" is Red / Blue / Orange. No orange card is needed —
  // `includes_inverted` asks about the variant's suits, not the cards held.
  SetupOptions orange;
  orange.hands = {
      {"r1", "r1", "r2", "r2", "r3"},
      {"b1", "b1", "b2", "b2", "b3"},
      {"r3", "r4", "r4", "b3", "b4"},
  };
  orange.variant_name = "Orange (3 Suits)";
  orange.play_stacks = {0, 0, 0};
  orange.clue_tokens = 3;
  orange.starting = TestPlayer::ALICE;
  use_reactor0(orange);
  Game g_orange = setup(std::move(orange));
  g_orange.meta[order_at(g_orange, TestPlayer::ALICE, 1)].status =
      CardStatus::CALLED_TO_DISCARD;
  EXPECT_TRUE(hanabi::reactor0::requires_high_tier(g_orange))
      << "with an inverted suit present a CTD is a queued action, so it "
         "forces the HIGH-only tier.";
}

// --- the HIGH window is wider than the MEDIUM one ------------------------
//
// The two required tiers carry different windows. When Alice holds a call she
// can fall back on, HIGH is demanded at every clue count short of the
// forced-clue case; when she does not, the original `<= 3` window and its
// "anything but LOW" requirement are unchanged.
//
// Rationale: holding something to do is exactly when the team can afford to
// wait for a clue worth the token, so capping that requirement at 3 tokens was
// leaving value on the table between 4 and 7.

// The headline delta: at 4 tokens the old window was closed entirely. With a
// CTP standing in Alice's hand it is now open, and a LOW clue is rejected.
TEST(Reactor0PaceClueGate, CtpStampFiresTheGateAboveThreeTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));
  g.meta[order_at(g, TestPlayer::ALICE, 1)].status = CardStatus::CALLED_TO_PLAY;

  ASSERT_GE(g.state.pace(), 1) << "guard: window needs pace >= 1";
  ASSERT_TRUE(hanabi::reactor0::requires_high_tier(g)) << "guard: call stands";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: this clue must be LOW";

  EXPECT_FALSE(admissible(g, clue))
      << "with a call standing the HIGH requirement reaches past 3 tokens; "
         "the old window stopped here and let this LOW clue through.";
}

// The companion negative, and what pins this as a SPLIT rather than a blanket
// widening: the identical position without the stamp keeps the old window and
// the gate stays silent at 4.
TEST(Reactor0PaceClueGate, WithoutAStampFourTokensIsStillOutsideTheWindow) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));

  ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g))
      << "guard: no call standing";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "the MEDIUM requirement still stops at 3 tokens.";
}

// The forced-clue exemption. At 8 tokens discarding is illegal, so gating would
// reject every non-HIGH clue at once and leave section 4 nothing to rank —
// losing the ordering exactly when there is no alternative to cluing.
TEST(Reactor0PaceClueGate, SilentAtEightTokensEvenWithACallStanding) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  g.meta[order_at(g, TestPlayer::ALICE, 1)].status = CardStatus::CALLED_TO_PLAY;

  ASSERT_TRUE(hanabi::reactor0::requires_high_tier(g)) << "guard: call stands";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "at 8 tokens the bot cannot discard, so the gate stands down.";
}

// The pace half of the window is untouched by the split: at pace 0 there is no
// reason to hoard tokens, call standing or not.
TEST(Reactor0PaceClueGate, SilentAtPaceZeroEvenWithACallStanding) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 5;
  opts.discarded = discards_for_pace(0);
  Game g = setup(std::move(opts));
  g.meta[order_at(g, TestPlayer::ALICE, 1)].status = CardStatus::CALLED_TO_PLAY;

  ASSERT_EQ(g.state.pace(), 0) << "guard: fixture must sit below the window";
  ASSERT_TRUE(hanabi::reactor0::requires_high_tier(g)) << "guard: call stands";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "pace < 3 keeps the gate shut regardless of the widened token range.";
}
