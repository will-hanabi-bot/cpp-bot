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
#include "hanabi/conventions/reactor0/calls.h"
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

// Pace 0 is the floor for BOTH rules: there, every remaining turn must produce
// a play or the game cannot finish, so hoarding a token for a better clue is
// pointless. This fixture is unoccupied, so it is 2a -- which since v8.2.0 also
// stands down at pace 1 and 2 (see the split below).
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

// --- the split: 1a and 2a take DIFFERENT pace thresholds -------------------

// Stamp a CTP in Alice's hand, which is what `requires_high_tier` keys on.
void make_occupied(Game& g) {
  g.meta[order_at(g, TestPlayer::ALICE, 1)].status = CardStatus::CALLED_TO_PLAY;
}

// 1a keeps `pace() >= 1`, and pace 1 is its boundary. This is the rule replay
// 1966119 T5 was about: an OCCUPIED Alice at low pace has a call she can
// action, so a LOW clue is never the better use of the turn.
TEST(Reactor0PaceClueGate, OccupiedGateFiresAtPaceOne) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(1);
  Game g = setup(std::move(opts));
  make_occupied(g);

  ASSERT_EQ(g.state.pace(), 1) << "guard: fixture must sit on the boundary";
  ASSERT_TRUE(hanabi::reactor0::requires_high_tier(g)) << "guard: rule 1a";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_LT(tier_of(g, clue), ClueTier::HIGH) << "guard: below 1a's bar";

  EXPECT_FALSE(admissible(g, clue))
      << "pace == 1 is inside 1a's window; the clue must be rejected.";
}

// The position replay 1966119 T5 actually sat at: occupied, pace 2, a LOW
// reactive discard admitted ahead of the pending call. Still refused.
TEST(Reactor0PaceClueGate, OccupiedGateFiresAtPaceTwo) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(2);
  Game g = setup(std::move(opts));
  make_occupied(g);

  ASSERT_EQ(g.state.pace(), 2) << "guard: 1966119 T5's pace";
  ASSERT_TRUE(hanabi::reactor0::requires_high_tier(g)) << "guard: rule 1a";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_LT(tier_of(g, clue), ClueTier::HIGH) << "guard: below 1a's bar";

  EXPECT_FALSE(admissible(g, clue))
      << "pace == 2 is inside 1a's window; the clue must be rejected.";
}

// 2a is the other half, and since v8.2.0 it needs `pace() >= 3`. An UNOCCUPIED
// Alice at low pace has no call to fall back on -- holding her to the MEDIUM
// bar with the deck nearly out just sends her to the discard pile. So the same
// LOW clue that 1a refuses above is admissible here.
TEST(Reactor0PaceClueGate, UnoccupiedGateStandsDownBelowPaceThree) {
  for (int pace : {1, 2}) {
    SetupOptions opts = inert_position();
    opts.clue_tokens = 2;
    opts.discarded = discards_for_pace(pace);
    Game g = setup(std::move(opts));

    ASSERT_EQ(g.state.pace(), pace);
    ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g)) << "guard: rule 2a";
    Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
    ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

    EXPECT_TRUE(admissible(g, clue))
        << "pace " << pace << " is below 2a's window; the bar does not apply.";
  }
}

// ...and pace 3 is 2a's boundary, where it does.
TEST(Reactor0PaceClueGate, UnoccupiedGateFiresAtPaceThree) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(3);
  Game g = setup(std::move(opts));

  ASSERT_EQ(g.state.pace(), 3) << "guard: fixture must sit on the boundary";
  ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g)) << "guard: rule 2a";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_FALSE(admissible(g, clue))
      << "pace == 3 is inside 2a's window; a LOW clue must be rejected.";
}

// The locked exemption's new home. Replay 1966653 sits at pace 1, so since
// v8.2.0 the gate stands down there on PACE before the exemption is reached --
// that test still pins its outcome, but no longer this rule. At pace 3 the
// window is live, and a locked Alice is exempt from it.
TEST(Reactor0PaceClueGate, LockedAliceIsExemptInsideTheWindow) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 2;
  opts.discarded = discards_for_pace(3);
  // Every card Alice holds is critical and clued, so she has no chop.
  opts.hands[0] = {"y5", "g5", "b5", "p5", "r5"};
  Game g = setup(std::move(opts));
  for (int slot = 1; slot <= 5; ++slot) {
    g = pre_clue(std::move(g), TestPlayer::ALICE, slot, {"5"});
  }

  ASSERT_EQ(g.state.pace(), 3) << "guard: inside 2a's window";
  ASSERT_FALSE(hanabi::reactor0::requires_high_tier(g)) << "guard: rule 2a";
  ASSERT_TRUE(g.common.thinks_locked(g, g.state.our_player_index))
      << "guard: Alice is locked";
  Action clue = make_clue(g, 0, 1, ClueKind::RANK, 4);
  ASSERT_EQ(tier_of(g, clue), ClueTier::LOW) << "guard: same LOW clue";

  EXPECT_TRUE(admissible(g, clue))
      << "a locked Alice has no chop to discard, so cluing is all she has.";
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

// ...but only a call she CAN ACTION counts (v13.3.0).
//
// "Occupied" exists to say Alice has something better to do than spend a token
// on a LOW clue, and 2a's locked exemption is written `!occupied && locked`
// because "an OCCUPIED Alice holds a call SHE CAN ACTION". A dead call makes
// that false: she is out of alternatives and still gated to HIGH.
//
// Replay 1981703 T19 is the cost. A dupe killed yagami_green's called card, its
// own sight of the real br4 elsewhere dropped the call from the pitch list, but
// the STAMP survived -- so it stayed "occupied", the gate flattened every
// candidate at pace 1, section 4 never got a pool, and phase 2 blind-pitched a
// br5 into a strike that ended the game.
//
// `CtpStampForcesHighTier` above is the control: a LIVE stamp still forces HIGH.
TEST(Reactor0PaceClueGate, ADeadCtpDoesNotForceHighTier) {
  SetupOptions opts = inert_position();
  opts.clue_tokens = 3;
  Game g = setup(std::move(opts));

  // Alice's slot 5 is the y5 and every stack is on 0, so pitching it could only
  // strike. `fully_known` pins it -- and `apply_pre_clue` writes COMMON only,
  // while `call_is_actionable` reads the HOLDER's view, so the pin has to be
  // mirrored there or the call still looks alive.
  g = fully_known(std::move(g), TestPlayer::ALICE, /*slot=*/5, "y5");
  g.elim();
  const int order = order_at(g, TestPlayer::ALICE, 5);
  {
    const Thought& t = g.common.thoughts[order];
    for (Player& p : g.players) {
      p.thoughts[order].inferred = t.inferred;
      p.thoughts[order].possible = t.possible;
    }
  }
  g.meta[order].status = CardStatus::CALLED_TO_PLAY;

  ASSERT_FALSE(hanabi::reactor0::call_is_actionable(g, 0, order))
      << "guard: the call is dead -- a y5 pitched at stack 0 only strikes";
  EXPECT_FALSE(hanabi::reactor0::requires_high_tier(g))
      << "a call she cannot action leaves her out of alternatives, which is "
         "the one thing 'occupied' is supposed to deny";
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
