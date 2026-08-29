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

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
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
  // 7, not the harness default of 8: at 8 tokens a clue to Bob is STABLE
  // (v11.2.0), and everything below is about the reactive regime.
  opts.clue_tokens = 7;
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

// --- the receiver is not the clued seat -----------------------------------
//
// The one place in the convention where `action.target` and the receiver are
// different players, and therefore the one place where a site that re-derives
// the receiver for itself goes wrong. Replay 1973971 T15 is what that cost:
// five sites had, and a reactive discard clue to Bob read as a MISTAKE.

TEST(Reactor0TargetParity, TheDispatchPredicateOwnsTheRule) {
  Game alt = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  const State& a = alt.state;
  const int alice = a.our_player_index;
  const int bob = a.next_player_index(alice);
  const int cathy = a.next_player_index(bob);

  for (int target : {bob, cathy}) {
    ClueAction act{alice, target, {}, BaseClue{ClueKind::RANK, 1}};
    EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(a, act, bob))
        << "every clue is reactive under target parity, target " << target;
    EXPECT_EQ(hanabi::reactor0::reactive_receiver(a, act, bob), cathy)
        << "and Cathy always receives, target " << target;
  }

  // Positional everywhere else, which is what keeps this inert in the 2324
  // variants that are not one of these two.
  Game plain = setup(target_parity_opts("No Variant"));
  const State& p = plain.state;
  ClueAction to_bob{alice, bob, {}, BaseClue{ClueKind::RANK, 1}};
  ClueAction to_cathy{alice, cathy, {}, BaseClue{ClueKind::RANK, 1}};
  EXPECT_FALSE(hanabi::reactor0::clue_is_reactive(p, to_bob, bob));
  EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(p, to_cathy, bob));
  EXPECT_EQ(hanabi::reactor0::reactive_receiver(p, to_cathy, bob), cathy)
      << "outside target parity the receiver IS the clued seat";
}

// The interpretation itself, read from THE REACTER'S OWN SEAT.
//
// That is what makes this discriminate. A branch that re-derives
// `receiver = action.target` walks the clued seat's hand -- and when the clued
// seat is us, every deck id there is nullopt, so the pool comes back empty and
// the clue reads as a MISTAKE. Read from any other seat the wrong hand is still
// visible and the branch quietly finds the wrong answer instead of failing, so
// a fixture giving the clue from our own seat would pass either way.
//
// The harness fixes `our_player_index` at 0, so the giver has to be the THIRD
// seat: Cathy gives, which makes seat 0 her Bob and seat 1 her Cathy.
SetupOptions clued_at_our_seat_opts(std::string variant_name) {
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.play_stacks = {0, 0, 0, 0, 0};
  // 7, not the harness default of 8: at 8 tokens a clue to Bob is STABLE
  // (v11.2.0), and everything below is about the reactive regime.
  opts.clue_tokens = 7;
  opts.starting = TestPlayer::CATHY;  // the giver
  opts.hands = {
      // `xx` is the harness's genuinely HIDDEN card (test_harness.cpp:96); a
      // named card gets a real deck id even in our own hand, which no real game
      // gives us. Naming these made the fixture stop discriminating: a branch
      // reading the wrong hand could still see it.
      {"xx", "xx", "xx", "xx", "xx"},  // seat 0 -- US, the reacter and clued
      {"g1", "y3", "b2", "p2", "r4"},  // seat 1 -- the receiver
      {"p4", "p5", "b4", "b5", "y5"},  // seat 2 -- the giver
  };
  use_reactor0(opts);
  return opts;
}

TEST(Reactor0TargetParity, AClueToUsDesignatesACardInTheThirdSeatsHand) {
  Game g = setup(clued_at_our_seat_opts("Alternating Clues (5 Suits)"));
  g = take_turn(std::move(g), "Cathy clues 1 to Alice (slot 1)");

  ASSERT_NE(last_clue_interp(g), ClueInterp::MISTAKE)
      << "the branch must walk the RECEIVER's hand; walking ours finds nothing "
         "because we cannot see our own cards";
  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr);
  EXPECT_EQ(wc->reacter, 0) << "we react";
  EXPECT_EQ(wc->receiver, 1) << "and seat 1 receives, though seat 0 was clued";
  EXPECT_EQ(wc->receiver_hand, g.state.hands[1]);
  // The exact pairing, which is the assertion that discriminates. The receiver's
  // only playable is the g1 in his slot 1, and anchor 1 gives
  // `calc_slot(1, 1, 5) = 5` -- so we are called on OUR slot 5, to discard,
  // because odd parity means exactly one play and it is the receiver's.
  //
  // A branch reading the wrong hand cannot land here: from our own seat every
  // card is nullopt, so it has nothing to choose a target from and falls
  // through to a lock on a different slot.
  EXPECT_EQ(wc->react_order, g.state.hands[0][4]) << "our slot 5";
  EXPECT_EQ(g.meta[wc->react_order].status, CardStatus::CALLED_TO_DISCARD)
      << "the receiver plays, so we discard";
  EXPECT_TRUE(urgent_at(g, TestPlayer::ALICE, 5))
      << "and a reaction is urgent";
}

// Synesthesia reaches the same place with a colour clue, its only kind.
TEST(Reactor0TargetParity, SynesthesiaAClueToUsAlsoDesignatesTheThirdSeat) {
  Game g = setup(clued_at_our_seat_opts("Synesthesia (5 Suits)"));
  g = take_turn(std::move(g), "Cathy clues Red to Alice (slot 1)");

  ASSERT_NE(last_clue_interp(g), ClueInterp::MISTAKE);
  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr);
  EXPECT_EQ(wc->reacter, 0);
  EXPECT_EQ(wc->receiver, 1);
  // Red is colour value 0, so its anchor is 1 -- the same pairing as the rank
  // case above, and pinned just as specifically so the fixture can fail.
  EXPECT_EQ(wc->react_order, g.state.hands[0][4]) << "our slot 5";
  EXPECT_EQ(g.meta[wc->react_order].status, CardStatus::CALLED_TO_DISCARD);
}

// The decision layer has to agree with the interpreter, or the bot would read
// these clues but never give one: `read_clue` sent every clue to Bob to the
// stable reader, and `wc_is_fresh` was handed the clued seat as the receiver.
TEST(Reactor0TargetParity, ReadClueClassifiesAClueToBobAsReactive) {
  Game g = setup(target_parity_opts("Alternating Clues (5 Suits)"));
  const State& s = g.state;
  const int bob = static_cast<int>(TestPlayer::BOB);

  ClueAction to_bob{s.our_player_index, bob,
                    s.clue_touched(s.hands[bob], ClueKind::RANK, 1),
                    BaseClue{ClueKind::RANK, 1}};
  const Game hypo = g.simulate(Action{to_bob});
  const auto reading = hanabi::reactor0::read_clue(g, hypo, to_bob);

  EXPECT_NE(reading.shape, hanabi::reactor0::ClueShape::STABLE_PLAY)
      << "there are no stable clues in this variant";
  EXPECT_NE(reading.shape, hanabi::reactor0::ClueShape::STABLE_DISCARD);
  EXPECT_NE(reading.shape, hanabi::reactor0::ClueShape::STABLE_LOCK);
  EXPECT_EQ(reading.stable_subject, -1)
      << "and so nothing is designated on a stable side";
  EXPECT_GE(reading.reacter_side.order, 0)
      << "the reacter side is what carries this clue's meaning";
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

// N2 asks whether the clue is REACTIVE. That was written as
// `action.target != bob`, which is only the same question while dispatch is
// positional -- under target parity a clue to Bob is reactive too, so it should
// reach N2 and could not.
TEST(Reactor0TargetParity, N2ReachesAClueToBob) {
  SetupOptions opts;
  opts.variant_name = "Alternating Clues (5 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0, 0, 0};
  // 7, not the harness default of 8: at 8 tokens a clue to Bob is STABLE
  // (v11.2.0), and everything below is about the reactive regime.
  opts.clue_tokens = 7;
  opts.hands = {
      {"r4", "y4", "g4", "b4", "p4"},  // Alice (giver, us)
      {"r2", "y2", "g2", "b2", "p2"},  // Bob -- nothing playable, chop is safe
      {"r5", "y3", "g3", "b3", "p3"},  // Cathy -- chop r5 is critical
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const State& s = g.state;
  const int bob = static_cast<int>(TestPlayer::BOB);
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  ASSERT_TRUE(hanabi::reactor0::at_risk_chop(g, s.our_player_index, cathy))
      << "guard: N2 requires Cathy's chop to be endangered";
  ASSERT_FALSE(hanabi::reactor0::has_colour_play_clue_for(g, bob, cathy))
      << "guard: vacuously false in a target-parity variant (v10.1.0)";

  ClueAction to_bob{s.our_player_index, bob,
                    s.clue_touched(s.hands[bob], ClueKind::RANK, 2),
                    BaseClue{ClueKind::RANK, 2}};
  const Game hypo = g.simulate(Action{to_bob});
  EXPECT_GE(hanabi::reactor0::clue_tier(g, hypo, to_bob), hanabi::reactor0::ClueTier::MEDIUM)
      << "a clue to Bob IS reactive here, so N2 applies to it";
}

TEST(Reactor0TargetParity, SettingsIsUnchangedForOrdinaryVariants) {
  const std::string s = hanabi::reactor0::format_settings(
      get_variant("No Variant"), {}, /*rlocks=*/true);
  EXPECT_NE(s.find("even reactive values"), std::string::npos) << s;
  EXPECT_EQ(s.find("to Bob = odd"), std::string::npos) << s;
}

// --- the 50% switch (v11.0.0 at 60%, moved in v11.4.0) --------------------
//
// Target parity is a mid-game rule. The reactive reading of a clue to Bob is the
// ODD bucket -- a reactive DISCARD -- and late in a game that forces a discard
// nobody needed while crowding out the two things that matter near the end:
// saving a good card, and getting a play clue out in time.
//
// So once the score reaches 50% of the VARIANT maximum, a clue to Bob is stable
// again. Clues to Cathy are untouched and keep the even bucket -- the kind still
// cannot carry parity, so nothing else about target parity moves.

namespace {

// `target_parity_opts` with the stacks dialled to a chosen score. Five suits, so
// the cap is 25 and the switch is at 13 -- half of 25 is 12.5, and the rule is
// "at or above", which `2 * score >= cap` gives exactly.
SetupOptions scored_opts(std::string variant_name, std::vector<int> stacks) {
  SetupOptions opts = target_parity_opts(std::move(variant_name));
  opts.play_stacks = std::move(stacks);
  return opts;
}

}  // namespace

TEST(Reactor0TargetParity, BobClueGoesStableAtHalfTheMaximum) {
  // 0.5 * 25 = 12.5, and the rule is "at or above", so 13 is the first score
  // that switches. Bracketed from both sides, because an off-by-one here means
  // two builds disagree about what a past clue meant -- the one failure this
  // convention cannot survive.
  struct Row { std::vector<int> stacks; int score; bool reactive; };
  const Row rows[] = {
      {{3, 3, 2, 2, 2}, 12, true},
      {{3, 3, 3, 2, 2}, 13, false},
      {{3, 3, 3, 3, 2}, 14, false},
  };
  for (const Row& r : rows) {
    Game g = setup(scored_opts("Alternating Clues (5 Suits)", r.stacks));
    ASSERT_EQ(g.state.score(), r.score) << "guard: fixture scores what it claims";
    EXPECT_EQ(hanabi::reactor0::bob_clue_is_reactive(g.state), r.reactive)
        << "score " << r.score << " of 25: a clue to Bob should be "
        << (r.reactive ? "reactive" : "STABLE");
  }
}

TEST(Reactor0TargetParity, TheCapIsTheVariantMaximumNotMaxScore) {
  // Three suits -> cap 15 -> switch at 8 (half is 7.5). If the cap were
  // `max_score()` it would
  // shrink as criticals died and the switch point would move mid-game, so two
  // seats could place the same past clue on different sides of it.
  // Its own hands: r/g/b only, so the five-suit fixture's purples do not exist.
  const auto three_suit = [](std::vector<int> stacks) {
    SetupOptions opts;
    opts.variant_name = "Alternating Clues (3 Suits)";
    opts.play_stacks = std::move(stacks);
    opts.clue_tokens = 7;  // see target_parity_opts
    opts.starting = TestPlayer::ALICE;
    opts.hands = {
        {"r5", "g5", "b5", "r4", "g4"},
        {"b4", "r3", "g3", "b3", "r2"},
        {"g2", "b2", "r1", "g1", "b1"},
    };
    use_reactor0(opts);
    return setup(std::move(opts));
  };
  Game g = three_suit({3, 3, 1});
  ASSERT_EQ(g.state.score(), 7);
  EXPECT_TRUE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "7 of 15 is below half";

  Game h = three_suit({3, 3, 2});
  ASSERT_EQ(h.state.score(), 8);
  EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(h.state))
      << "8 of 15 is the first score at or above half, so a clue to Bob is "
         "stable";
}

TEST(Reactor0TargetParity, PlainVariantsAreUnaffectedAtEveryScore) {
  for (const std::vector<int>& stacks :
       {std::vector<int>{0, 0, 0, 0, 0}, std::vector<int>{5, 5, 5, 5, 5}}) {
    Game g = setup(scored_opts("No Variant", stacks));
    EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
        << "outside a target-parity variant a clue to Bob was never reactive";
  }
}

// The dispatch either side of the switch, and -- the load-bearing half -- that
// a clue to CATHY does not move with it.
TEST(Reactor0TargetParity, OnlyBobsClueCrossesTheThreshold) {
  const auto probe = [](const std::vector<int>& stacks) {
    return setup(scored_opts("Alternating Clues (5 Suits)", stacks));
  };
  Game below = probe({3, 3, 2, 2, 2});   // 12
  Game above = probe({3, 3, 3, 2, 2});   // 13

  const int alice = below.state.our_player_index;
  const int bob = below.state.next_player_index(alice);
  const int cathy = below.state.next_player_index(bob);
  ClueAction to_bob{alice, bob, {}, BaseClue{ClueKind::RANK, 1}};
  ClueAction to_cathy{alice, cathy, {}, BaseClue{ClueKind::RANK, 1}};

  EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(below.state, to_bob, bob))
      << "below the threshold a clue to Bob is still reactive";
  EXPECT_FALSE(hanabi::reactor0::clue_is_reactive(above.state, to_bob, bob))
      << "above it a clue to Bob is STABLE -- the whole change";

  for (const Game* g : {&below, &above}) {
    EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(g->state, to_cathy, bob))
        << "a clue to Cathy is reactive on both sides";
    EXPECT_EQ(hanabi::reactor0::reactive_receiver(g->state, to_cathy, bob), cathy);
    // The assertion that catches a wholesale `uses_target_parity` flip. If the
    // gate had been applied to `reactive_assignment_for` too, a clue to Cathy
    // would stop taking its parity from the target and start taking it from the
    // KIND -- silently changing what a rank clue to Cathy means.
    EXPECT_TRUE(reactive_assignment_for(*g->state.variant, g->reactive_overrides,
                                        ClueKind::RANK, 1,
                                        /*target_is_bob=*/false)
                    .even)
        << "a clue to Cathy stays EVEN on both sides of the threshold";
  }
}

// --- the 8-token escape hatch (v11.2.0) -----------------------------------
//
// At 8 tokens a discard is illegal, so the turn MUST produce a clue. Under
// target parity every clue is reactive, so every candidate has to survive a
// reactive reading or it is dropped as a MISTAKE and never offered -- and when
// none does, the clue phase declines and the play/discard phase answers with a
// discard the server will not accept. A DEADLOCK, not a bad choice.
//
// So at 8 tokens a clue to Bob is STABLE whatever the score. A stable clue names
// a card outright and is far easier to read cleanly, which is what a forced turn
// needs. Replay 1977786 T35 is the deadlock this closes.

TEST(Reactor0TargetParity, AtEightCluesAClueToBobIsStableWhateverTheScore) {
  // Score 0 of 25 -- as far below the score switch as it gets, so the ONLY thing
  // that can make this stable is the token count. That is what keeps this test
  // isolating the 8-clue rule even as the score threshold moves.
  SetupOptions opts = target_parity_opts("Alternating Clues (5 Suits)");
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.score(), 0) << "guard: nowhere near the score switch";

  EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "at 8 tokens the turn is forced, so a clue to Bob must be readable "
         "as a stable one";

  const int alice = g.state.our_player_index;
  const int bob = g.state.next_player_index(alice);
  const int cathy = g.state.next_player_index(bob);
  ClueAction to_bob{alice, bob, {}, BaseClue{ClueKind::RANK, 1}};
  ClueAction to_cathy{alice, cathy, {}, BaseClue{ClueKind::RANK, 1}};
  EXPECT_FALSE(hanabi::reactor0::clue_is_reactive(g.state, to_bob, bob));
  EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(g.state, to_cathy, bob))
      << "a clue to Cathy is untouched -- the rule is about the seat that is "
         "forced to be readable, not about the parity";
}

// One token down and the ordinary rule is back, with the score unchanged. This
// is the pair that shows the 8-token arm is doing the work rather than some
// other property of the fixture.
TEST(Reactor0TargetParity, AtSevenCluesTheSameClueIsReactiveAgain) {
  SetupOptions opts = target_parity_opts("Alternating Clues (5 Suits)");
  opts.clue_tokens = 7;
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.score(), 0) << "guard: the same score as the test above";

  EXPECT_TRUE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "below 8 the turn is not forced, so target parity binds as before";
}

// Synesthesia takes the same hatch -- the rule is `uses_target_parity`, not one
// family. There its stable clues read off the §1f colour table.
TEST(Reactor0TargetParity, SynesthesiaAlsoGoesStableAtEightClues) {
  SetupOptions opts = target_parity_opts("Synesthesia (5 Suits)");
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state));

  SetupOptions seven = target_parity_opts("Synesthesia (5 Suits)");
  seven.clue_tokens = 7;
  Game h = setup(std::move(seven));
  EXPECT_TRUE(hanabi::reactor0::bob_clue_is_reactive(h.state));
}

// Plain variants are untouched at any token count: a clue to Bob was never
// reactive there, so there is no hatch to take.
TEST(Reactor0TargetParity, PlainVariantsAreUnaffectedByTheTokenRule) {
  for (int tokens : {8, 7, 1}) {
    SetupOptions opts = target_parity_opts("No Variant");
    opts.clue_tokens = tokens;
    Game g = setup(std::move(opts));
    EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
        << "tokens " << tokens;
  }
}

// --- two clue colours: a COLOUR clue to Bob joins the even bucket (v11.5.0) --
//
// Target parity normally takes the bucket from the target -- Bob odd, Cathy
// even -- which with only two clue colours leaves the whole game just TWO colour
// anchors. Putting colour-to-Bob in the even bucket with its own values doubles
// that to four:
//
//     to Cathy   Red 1, Blue 4
//     to Bob     Red 2, Blue 5
//
// All nine such variants are Red + Blue: six Alternating Clues (Rainbow, White,
// Omni, Null, Muddy Rainbow, Light Pink) and three Synesthesia (Rainbow, White,
// Null). Their third suit is `allClueColors` or `noClueColors`, which is what
// leaves only two.

TEST(Reactor0TwoColours, ColourToBobIsEvenWithItsOwnAnchor) {
  const std::vector<ReactiveOverride> none;
  for (const char* name : {"Alternating Clues & Rainbow (3 Suits)",
                           "Synesthesia & Null (3 Suits)"}) {
    const Variant& v = get_variant(name);
    ASSERT_EQ(v.clue_colour_names.size(), 2u)
        << name << " should offer exactly two clue colours";
    ASSERT_EQ(v.clue_colour_names[0], "Red");
    ASSERT_EQ(v.clue_colour_names[1], "Blue");
    EXPECT_TRUE(hanabi::reactor0::bob_colour_joins_even(v)) << name;

    struct Row { int idx; int cathy; int bob; };
    for (const Row& r : {Row{0, 1, 2}, Row{1, 4, 5}}) {
      const auto to_cathy = reactive_assignment_for(v, none, ClueKind::COLOUR,
                                                    r.idx, /*target_is_bob=*/false);
      const auto to_bob = reactive_assignment_for(v, none, ClueKind::COLOUR,
                                                  r.idx, /*target_is_bob=*/true);
      EXPECT_TRUE(to_cathy.even) << name << " colour " << r.idx << " to Cathy";
      EXPECT_EQ(to_cathy.value, r.cathy) << name << " colour " << r.idx;
      EXPECT_TRUE(to_bob.even)
          << name << ": with only two colours a colour clue to Bob is EVEN, not "
                     "odd -- that is the whole rule";
      EXPECT_EQ(to_bob.value, r.bob)
          << name << ": and it carries its own anchor, one past Cathy's";
    }
  }
}

// The negative that pins the rule to the COLOUR COUNT rather than to target
// parity generally. Three clue colours and a clue to Bob is odd again.
TEST(Reactor0TwoColours, ThreeColoursKeepsColourToBobOdd) {
  const std::vector<ReactiveOverride> none;
  const Variant& v = get_variant("Alternating Clues (3 Suits)");
  ASSERT_EQ(v.clue_colour_names.size(), 3u)
      << "guard: Red/Green/Blue, so the rule must not fire";
  EXPECT_FALSE(hanabi::reactor0::bob_colour_joins_even(v));

  for (int i = 0; i < 3; ++i) {
    const auto to_bob = reactive_assignment_for(v, none, ClueKind::COLOUR, i,
                                                /*target_is_bob=*/true);
    EXPECT_FALSE(to_bob.even) << "colour " << i << " to Bob stays odd";
    EXPECT_EQ(to_bob.value,
              reactive_assignment_for(v, none, ClueKind::COLOUR, i, false).value)
        << "and keeps Cathy's value, since only the two-colour rule shifts it";
  }
}

// RANK clues are untouched, which is what leaves Alternating Clues a one-play
// bucket. (Synesthesia has no ranks, so its two-colour variants have no odd
// bucket at all below the stable switch -- intended, and stated in §1f.)
TEST(Reactor0TwoColours, RankToBobIsStillOdd) {
  const std::vector<ReactiveOverride> none;
  const Variant& v = get_variant("Alternating Clues & Rainbow (3 Suits)");
  for (int rank = 1; rank <= 5; ++rank) {
    const auto to_bob = reactive_assignment_for(v, none, ClueKind::RANK, rank,
                                                /*target_is_bob=*/true);
    EXPECT_FALSE(to_bob.even) << "rank " << rank << " to Bob is still ODD";
    EXPECT_EQ(to_bob.value, rank) << "with its ordinary value";
  }
}

// `/set` is taken VERBATIM: the +1 is a DEFAULT, not a transformation imposed on
// what the player asked for.
TEST(Reactor0TwoColours, AnOverrideIsUsedWithoutTheShift) {
  const Variant& v = get_variant("Alternating Clues & Rainbow (3 Suits)");
  const std::vector<ReactiveOverride> set_red{
      ReactiveOverride{ClueKind::COLOUR, 0, /*even=*/true, /*reactive_value=*/3}};

  const auto to_bob = reactive_assignment_for(v, set_red, ClueKind::COLOUR, 0,
                                              /*target_is_bob=*/true);
  EXPECT_TRUE(to_bob.even) << "the bucket is structural and still applies";
  EXPECT_EQ(to_bob.value, 3)
      << "the player set 3, so it is 3 -- NOT 4. The default shift applies only "
         "where there is no override to respect.";
  // Blue, with no override, still takes the default.
  EXPECT_EQ(reactive_assignment_for(v, set_red, ClueKind::COLOUR, 1, true).value, 5);
}

// End to end: the shifted anchor is what lands in the waiting connection, so
// clue-time selection and reaction-time resolution cannot drift apart.
TEST(Reactor0TwoColours, TheShiftedAnchorReachesTheWaitingConnection) {
  SetupOptions opts;
  opts.variant_name = "Alternating Clues & Rainbow (3 Suits)";
  opts.play_stacks = {0, 0, 0};
  opts.clue_tokens = 7;  // below 8, so target parity still binds
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "b4", "r3", "b3", "r2"},   // Alice (giver, us)
      {"r1", "b1", "r5", "b5", "r4"},   // Bob
      {"b2", "r2", "b3", "r3", "b4"},   // Cathy
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.score(), 0) << "guard: below the stable switch";

  g = take_turn(std::move(g), "Alice clues Blue to Bob");
  const ReactorWC* wc = wc_of(g);
  ASSERT_NE(wc, nullptr) << "a clue to Bob is still REACTIVE here";
  EXPECT_EQ(wc->focus_slot, 5)
      << "Blue to Bob anchors at 5, not Cathy's 4 -- the anchor every calc_slot "
         "reads comes from the same assignment the giver used";
  ASSERT_TRUE(wc->even_parity.has_value());
  EXPECT_TRUE(*wc->even_parity)
      << "and it is bound into the connection as EVEN, so resolution a turn "
         "later cannot contradict the reading";
}

// `/settings` is what a human reads to know the convention in play, so "to Bob =
// odd" would misdescribe half the clues available in these nine variants.
TEST(Reactor0TwoColours, SettingsSpellsOutBothTables) {
  const std::string s = hanabi::reactor0::format_settings(
      get_variant("Alternating Clues & Rainbow (3 Suits)"), {}, /*rlocks=*/true);
  std::fprintf(stderr, "/settings: %s\n", s.c_str());

  EXPECT_EQ(s.find("to Bob = odd"), std::string::npos)
      << "that line is wrong here -- a COLOUR clue to Bob is even: " << s;
  EXPECT_NE(s.find("a COLOUR clue to Bob is EVEN too"), std::string::npos) << s;
  EXPECT_NE(s.find("rank to Bob stays odd"), std::string::npos)
      << "the exception has its own exception, and a reader needs both: " << s;
  EXPECT_NE(s.find("Red=1"), std::string::npos) << "Cathy's table: " << s;
  EXPECT_NE(s.find("Red=2"), std::string::npos) << "Bob's table: " << s;
  EXPECT_NE(s.find("Blue=5"), std::string::npos) << "Bob's table: " << s;
  EXPECT_NE(s.find("rank values"), std::string::npos)
      << "Alternating Clues has ranks, and they are the only odd bucket left "
         "here, so a reader needs them: " << s;

  // Synesthesia carries `clueRanks: []`, so advertising a rank table there
  // would offer clues that cannot be given -- and it is also where the odd
  // bucket disappears entirely.
  const std::string syn = hanabi::reactor0::format_settings(
      get_variant("Synesthesia & Null (3 Suits)"), {}, /*rlocks=*/true);
  EXPECT_NE(syn.find("no rank clues"), std::string::npos) << syn;
  EXPECT_EQ(syn.find("rank values"), std::string::npos) << syn;
  EXPECT_NE(syn.find("Red=2"), std::string::npos) << "Bob's table: " << syn;
}
