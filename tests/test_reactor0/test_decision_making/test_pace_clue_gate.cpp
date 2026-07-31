// Reactor0's pace-clue tier gate (src/conventions/reactor0/state_eval.cpp,
// CONVENTION.md §2a) — the window it fires in, and which tier it demands.
//
//   window: pace() >= 3 && clue_tokens <= 3
//   required tier: HIGH when Alice holds a card stamped CALLED_TO_PLAY, or —
//                  in a variant containing an inverted suit — a card stamped
//                  CALLED_TO_DISCARD. Otherwise NOT-LOW (HIGH or MEDIUM).
//
// A clue below its required tier scores a flat -1.0, reactor's rejection
// signature. Unlike reactor's low-clue-count gate this window is one token
// wider (`<= 3`, not `< 3`) and has NO "we hold a real play" conjunct, so it
// also fires when Alice has nothing queued.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor/state_eval.h"
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

}  // namespace

// --- the window ----------------------------------------------------------

// The headline delta from reactor: reactor's gate is `clue_tokens < 3`, so it
// never fires at exactly 3. Reactor0's `<= 3` does.
TEST(Reactor0PaceClueGate, FiresAtThreeClueTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 3;
  Game g = setup(std::move(opts));

  ASSERT_GE(g.state.pace(), 3) << "guard: window needs pace >= 3";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: this clue must be LOW";

  EXPECT_EQ(hanabi::reactor0::eval_action(g, clue), -1.0)
      << "at exactly 3 clues the reactor0 gate is active and rejects a LOW "
         "clue; reactor's `< 3` window would have let it through.";
}

// One token above the window the gate is silent and the clue scores normally.
TEST(Reactor0PaceClueGate, SilentAtFourClueTokens) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 4;
  Game g = setup(std::move(opts));

  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_NE(hanabi::reactor0::eval_action(g, clue), -1.0)
      << "clue_tokens == 4 is outside the window; the gate must not fire.";
}

// Below pace 3 the gate is silent even at a low clue count — with no pace to
// spare, spending a token beats burning a card.
TEST(Reactor0PaceClueGate, SilentBelowPaceThree) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = kTenOnes;
  opts.discarded.push_back("r2");  // one more, pushing pace to 2
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 2) << "guard: fixture must sit just below the window";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_NE(hanabi::reactor0::eval_action(g, clue), -1.0)
      << "pace == 2 is outside the window; the gate must not fire.";
}

// ...and fires again at exactly pace 3.
TEST(Reactor0PaceClueGate, FiresAtPaceThree) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = kTenOnes;
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 3) << "guard: fixture must sit on the boundary";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_EQ(hanabi::reactor0::eval_action(g, clue), -1.0)
      << "pace == 3 is inside the window; a LOW clue must be rejected.";
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
