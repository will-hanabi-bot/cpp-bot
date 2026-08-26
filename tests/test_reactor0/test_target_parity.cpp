// Target parity: Alternating Clues and Synesthesia have NO stable clues.
//
// Reactor0 normally reads a clue's PARITY off its KIND -- rank is the even
// family (double play / double discard), colour the odd one (exactly one play),
// and Odds and Evens swaps the two. Both of these variants take the choice of
// kind away from the giver: Synesthesia offers colour clues only
// (`clueRanks: []`), and Alternating Clues forces the kind to alternate, so on
// any given turn at most one kind is even legal. A signal cannot ride on a
// choice the giver does not have.
//
// So the parity moves to the TARGET:
//
//     clue to Bob    -> ODD   (exactly one play)
//     clue to Cathy  -> EVEN  (double play / double discard)
//
// and the roles are fixed regardless: BOB IS ALWAYS THE REACTER, being the seat
// that acts next, and CATHY IS ALWAYS THE RECEIVER. A clue to Bob therefore
// touches the reacter's own hand while still identifying a slot in Cathy's --
// Bob acts, Cathy reads which slot he chose, and the turn order works out.
//
// The reactive VALUES are untouched: 1=1..5=5 and Red=1, Yellow=2, Green=3,
// Blue=4, Purple=5, exactly as everywhere else.
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"
#include "hanabi/conventions/variants/predicates.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::reactive_assignment;
using hanabi::reactor0::reactive_assignment_for;
using hanabi::reactor::variants::uses_target_parity;

namespace {

SetupOptions target_parity_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"p4", "p5", "b4", "b5", "y5"},  // Alice (giver) -- nothing playable
      {"r1", "y4", "g4", "b3", "p3"},  // Bob (reacter) -- r1 playable on slot 1
      {"g1", "y3", "b2", "p2", "r4"},  // Cathy (receiver) -- g1 playable slot 1
  };
  use_reactor0(opts);
  return opts;
}

const ReactorWC* wc_of(const Game& g) {
  return g.waiting.empty() ? nullptr : &g.waiting.front();
}

}  // namespace

// --- the predicate --------------------------------------------------------

TEST(Reactor0TargetParity, ThePredicateCoversBothFamiliesAndNothingElse) {
  EXPECT_TRUE(uses_target_parity(get_variant("Alternating Clues (5 Suits)")));
  EXPECT_TRUE(uses_target_parity(get_variant("Synesthesia (5 Suits)")));
  EXPECT_FALSE(uses_target_parity(get_variant("No Variant")));
  EXPECT_FALSE(uses_target_parity(get_variant("Odds and Evens (5 Suits)")))
      << "Odds and Evens swaps which KIND carries which parity -- it does not "
         "move the parity onto the target";
}

// --- the assignment -------------------------------------------------------

// The load-bearing property, and the discriminator against the old rule: the
// parity follows the TARGET and does not move when the kind changes.
TEST(Reactor0TargetParity, ParityFollowsTheTargetNotTheKind) {
  const Variant& v = get_variant("Alternating Clues (5 Suits)");
  const std::vector<ReactiveOverride> none;

  for (ClueKind kind : {ClueKind::RANK, ClueKind::COLOUR}) {
    EXPECT_FALSE(
        reactive_assignment_for(v, none, kind, 1, /*target_is_bob=*/true).even)
        << "a clue to Bob is ODD whatever its kind";
    EXPECT_TRUE(
        reactive_assignment_for(v, none, kind, 1, /*target_is_bob=*/false).even)
        << "a clue to Cathy is EVEN whatever its kind";
  }
}

// Outside these variants the target argument must be inert.
TEST(Reactor0TargetParity, TheTargetIsIgnoredInAnOrdinaryVariant) {
  const Variant& v = get_variant("No Variant");
  const std::vector<ReactiveOverride> none;
  for (bool to_bob : {true, false}) {
    EXPECT_TRUE(reactive_assignment_for(v, none, ClueKind::RANK, 3, to_bob).even)
        << "rank is the even family here regardless of who is clued";
    EXPECT_FALSE(
        reactive_assignment_for(v, none, ClueKind::COLOUR, 0, to_bob).even);
  }
}

// The VALUES are the ones the user specified and are untouched by any of this.
TEST(Reactor0TargetParity, ReactiveValuesAreUnchanged) {
  const Variant& v = get_variant("Alternating Clues (5 Suits)");
  const std::vector<ReactiveOverride> none;
  for (int rank = 1; rank <= 5; ++rank) {
    EXPECT_EQ(reactive_assignment_for(v, none, ClueKind::RANK, rank, true).value,
              rank)
        << "1=1 .. 5=5";
  }
  // Red=1, Yellow=2, Green=3, Blue=4, Purple=5.
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(reactive_assignment_for(v, none, ClueKind::COLOUR, i, true).value,
              i + 1)
        << "colour index " << i << " (" << v.clue_colour_names[i] << ")";
  }
}

// --- dispatch -------------------------------------------------------------

// A clue to Cathy: the ordinary reactive shape, and the parity is even.
TEST(Reactor0TargetParity, AClueToCathyIsEvenReactive) {
  Game g = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr) << "a clue here must read as reactive";
  EXPECT_EQ(wc->reacter, static_cast<int>(TestPlayer::BOB));
  EXPECT_EQ(wc->receiver, static_cast<int>(TestPlayer::CATHY));
  ASSERT_TRUE(wc->even_parity.has_value());
  EXPECT_TRUE(*wc->even_parity) << "a clue to Cathy is EVEN";
}

// A clue to Bob: still reactive, still Bob reacting and Cathy receiving -- but
// odd. This is the case that has no counterpart outside these variants, where a
// clue to the next player is always stable.
TEST(Reactor0TargetParity, AClueToBobIsOddReactiveWithCathyStillTheReceiver) {
  Game g = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr) << "there are no stable clues in this variant";
  EXPECT_EQ(wc->reacter, static_cast<int>(TestPlayer::BOB))
      << "Bob reacts -- he is the seat that acts next";
  EXPECT_EQ(wc->receiver, static_cast<int>(TestPlayer::CATHY))
      << "Cathy receives even though the clue touched BOB's hand";
  ASSERT_TRUE(wc->even_parity.has_value());
  EXPECT_FALSE(*wc->even_parity) << "a clue to Bob is ODD";
}

// The WC's clue must remember who was CLUED, not who receives -- every later
// parity lookup keys on it, and with the receiver pinned to Cathy the two come
// apart exactly here.
TEST(Reactor0TargetParity, TheWaitingConnectionRecordsTheCluedSeat) {
  Game g = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr);
  EXPECT_EQ(wc->clue.target, static_cast<int>(TestPlayer::BOB))
      << "the clue went to Bob; rewriting this to the receiver would flip the "
         "parity every later lookup computes";
  EXPECT_EQ(wc->receiver_hand, g.state.hands[static_cast<int>(TestPlayer::CATHY)])
      << "while the hand the arithmetic addresses is Cathy's";
}

// Synesthesia reaches the same place by the other route: colour is the only
// kind it has, so a colour clue to Bob must be odd rather than taking colour's
// usual odd/even role by kind.
TEST(Reactor0TargetParity, SynesthesiaDispatchesOnTheTargetToo) {
  Game g = setup(target_parity_opts("Synesthesia (5 Suits)"));
  g = take_turn(std::move(g), "Alice clues Green to Cathy");

  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr);
  EXPECT_EQ(wc->receiver, static_cast<int>(TestPlayer::CATHY));
  ASSERT_TRUE(wc->even_parity.has_value());
  EXPECT_TRUE(*wc->even_parity)
      << "a colour clue would be ODD by kind; to Cathy it is EVEN by target";
}

// The control: in a plain variant a clue to Bob is stable, so no waiting
// connection is created at all. This is what the change is a departure from.
TEST(Reactor0TargetParity, APlainVariantStillTreatsAClueToBobAsStable) {
  Game g = setup(target_parity_opts("No Variant"));
  g = take_turn(std::move(g), "Alice clues 1 to Bob");

  EXPECT_EQ(wc_of(g), nullptr)
      << "outside a target-parity variant a clue to the next player is stable";
}

// --- the tier consequence -------------------------------------------------

// H1c and N2 both ask "could Bob have handled Cathy himself, with a stable
// colour play clue?". There are no stable clues here, so the answer is never
// yes -- and Cathy's g1 is playable, so the predicate would say yes if it were
// not gated.
TEST(Reactor0TargetParity, BobHasNoStableColourPlayClueForCathy) {
  const int bob = static_cast<int>(TestPlayer::BOB);
  const int cathy = static_cast<int>(TestPlayer::CATHY);

  Game plain = setup(target_parity_opts("No Variant"));
  ASSERT_TRUE(hanabi::reactor0::has_colour_play_clue_for(plain, bob, cathy))
      << "guard: the fixture DOES offer one in an ordinary variant, so the "
         "assertion below is about the gate and not about the position";

  Game alt = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  EXPECT_FALSE(hanabi::reactor0::has_colour_play_clue_for(alt, bob, cathy))
      << "there are no stable clues in a target-parity variant";
}

// --- /settings ------------------------------------------------------------

TEST(Reactor0TargetParity, SettingsReportsTheTargetRuleRatherThanBuckets) {
  const std::string s = hanabi::reactor0::format_settings(
      get_variant("Alternating Clues (5 Suits)"), {}, /*rlocks=*/true);
  EXPECT_NE(s.find("to Bob = odd"), std::string::npos) << s;
  EXPECT_NE(s.find("to Cathy = even"), std::string::npos) << s;
  EXPECT_EQ(s.find("even reactive values"), std::string::npos)
      << "the two-bucket split does not describe this game: " << s;
  // The values the user asked to see, still present.
  EXPECT_NE(s.find("1=1"), std::string::npos) << s;
  EXPECT_NE(s.find("Purple=5"), std::string::npos) << s;
}

TEST(Reactor0TargetParity, SettingsIsUnchangedForOrdinaryVariants) {
  const std::string s = hanabi::reactor0::format_settings(
      get_variant("No Variant"), {}, /*rlocks=*/true);
  EXPECT_NE(s.find("even reactive values"), std::string::npos) << s;
  EXPECT_EQ(s.find("to Bob = odd"), std::string::npos) << s;
}
