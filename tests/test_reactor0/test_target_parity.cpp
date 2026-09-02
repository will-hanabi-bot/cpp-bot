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

// --- the pace switch (v14.0.0; was a score fraction from v11.0.0) ---------
//
// Target parity is a mid-game rule. The reactive reading of a clue to Bob is the
// ODD bucket -- a reactive DISCARD -- and late in a game that forces a discard
// nobody needed while crowding out the two things that matter near the end:
// saving a good card, and getting a play clue out in time.
//
// "Late" is now counted in PACE rather than in points banked: at `pace() <= 1` a
// clue to Bob is stable again. Clues to Cathy are untouched and keep the even
// bucket -- the kind still cannot carry parity, so nothing else moves.

namespace {

// `target_parity_opts` dialled to a chosen pace. With empty stacks and no
// discards a five-suit fixture sits at pace 13, and every discard costs exactly
// one: `score + cards_left` is `35 - discards`, so the score cancels out. All
// the pool entries are spare copies of a rank-1 or rank-2, so `max_score` --
// which pace also reads -- never moves.
constexpr int kBasePace = 13;

SetupOptions paced_opts(std::string variant_name, int target_pace) {
  static const std::vector<std::string> kPool = {
      "r1", "r1", "y1", "y1", "g1", "g1", "b1", "b1", "p1", "p1",
      "y2", "g2", "b2", "p2"};
  SetupOptions opts = target_parity_opts(std::move(variant_name));
  for (int i = 0; i < kBasePace - target_pace && i < (int)kPool.size(); ++i) {
    opts.discarded.push_back(kPool[i]);
  }
  return opts;
}

}  // namespace

TEST(Reactor0TargetParity, BobClueGoesStableAtPaceOne) {
  // Bracketed from both sides. An off-by-one here means two builds disagree
  // about what a past clue meant -- the one failure this convention cannot
  // survive.
  struct Row { int pace; bool reactive; };
  const Row rows[] = {{3, true}, {2, true}, {1, false}, {0, false}};
  for (const Row& r : rows) {
    Game g = setup(paced_opts("Alternating Clues (5 Suits)", r.pace));
    ASSERT_EQ(g.state.pace(), r.pace) << "guard: fixture paces what it claims";
    EXPECT_EQ(hanabi::reactor0::bob_clue_is_reactive(g.state), r.reactive)
        << "pace " << r.pace << ": a clue to Bob should be "
        << (r.reactive ? "reactive" : "STABLE");
  }
}

// The v11.4.0 rule went out of its way to read a CONSTANT `5 * suits` cap rather
// than `max_score()`, so that discarding a critical could not move the switch
// mid-game. `pace()` reads `max_score()`, so v14.0.0 gives that property up --
// deliberately, and this pins it so the trade is recorded rather than discovered.
//
// The direction is the counter-intuitive part. Capping a suit at rank k removes
// ONE card from the deck but `5 - k` points from the target, so
// `pace = score + cards_left + n - max_score` RISES by `4 - k`: losing a low
// critical makes the team less pressed for turns, not more. Only a 5 is neutral.
//
// It is safe because a clue's reading is fixed when `interpret_clue` runs and is
// never recomputed: two seats read the same clue at the same moment off the same
// state, so they cannot land on different sides of a threshold that later moves.
TEST(Reactor0TargetParity, TheThresholdMovesWithMaxScore) {
  SetupOptions opts = paced_opts("Alternating Clues (5 Suits)", 1);
  Game before = setup(opts);
  ASSERT_EQ(before.state.pace(), 1);
  EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(before.state))
      << "guard: pace 1 is the stable side";

  // Both r3s: red caps at 2, so max_score loses r3, r4 and r5 for two cards.
  opts.discarded.push_back("r3");
  opts.discarded.push_back("r3");
  Game after = setup(std::move(opts));
  ASSERT_EQ(after.state.max_score(), before.state.max_score() - 3)
      << "guard: the discards must actually cap red at 2";
  EXPECT_EQ(after.state.pace(), 2) << "-2 cards_left, -3 max_score";
  EXPECT_TRUE(hanabi::reactor0::bob_clue_is_reactive(after.state))
      << "losing red's 3s put the switch back on the reactive side -- movement "
         "the constant-cap score rule could not have produced";
}

// v14.0.0 DELETED the 8-clue arm, and this is the direct consequence: eight
// tokens no longer confer stability.
//
// The arm was added as a DEADLOCK GUARD (replay 1977786 T35): at 8 tokens a
// discard is illegal, so the turn must produce a clue, and under target parity
// every clue is reactive -- so if every candidate reads MISTAKE the clue phase
// declines with an empty set. Removing it does NOT bring the deadlock back,
// because phase 2 closed the same hole variant-independently: `choose_action`
// empties the chuck list at 8 tokens (`calls.cpp`) and rungs 12/13 answer with a
// PITCH, which is legal at any token count. The three two-colour Synesthesia
// variants have relied on exactly that since v13.0.0, having no stable clue at
// all.
//
// What is lost is the READABLE CLUE, not the legal move: Alice may now pitch
// where she would have clued. A worse turn, not an illegal one.
TEST(Reactor0TargetParity, EightCluesNoLongerMakesAClueToBobStable) {
  SetupOptions opts = paced_opts("Alternating Clues (5 Suits)", 3);
  opts.clue_tokens = 8;
  Game g = setup(std::move(opts));
  ASSERT_EQ(g.state.clue_tokens, 8);
  ASSERT_EQ(g.state.pace(), 3) << "guard: well clear of the pace switch";
  EXPECT_TRUE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "through v13.5.0 eight tokens made this STABLE whatever the score";
}

TEST(Reactor0TargetParity, PlainVariantsAreUnaffectedAtEveryPace) {
  for (int pace : {13, 2, 1, 0}) {
    Game g = setup(paced_opts("No Variant", pace));
    EXPECT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
        << "outside a target-parity variant a clue to Bob was never reactive";
  }
}

// The dispatch either side of the switch, and -- the load-bearing half -- that
// a clue to CATHY does not move with it.
TEST(Reactor0TargetParity, OnlyBobsClueCrossesTheThreshold) {
  const auto probe = [](int pace) {
    return setup(paced_opts("Alternating Clues (5 Suits)", pace));
  };
  Game below = probe(2);   // still reactive
  Game above = probe(1);   // stable

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

  // v13.0.0. `/settings` is the convention as a human reads it, so it must not
  // still promise a stable clue that these variants no longer have.
  EXPECT_NE(s.find("a COLOUR clue is NEVER stable"), std::string::npos)
      << "Alternating Clues keeps a rank channel, so the line has to say which "
         "kind stands down and which does not: " << s;
  EXPECT_NE(s.find("RANK clue to Bob is STABLE"), std::string::npos) << s;
  EXPECT_NE(syn.find("NO clue is ever stable"), std::string::npos)
      << "Synesthesia has no ranks, so here the stable channel is gone "
         "entirely and a reader must be told outright: " << syn;
  EXPECT_EQ(syn.find("RANK clue to Bob is STABLE"), std::string::npos)
      << "and must NOT be offered a rank fallback that cannot be clued: " << syn;
}

// --- v13.0.0: with two clue colours, a COLOUR clue is never stable ----------
//
// Two colours leave only two colour anchors, which is why a colour clue to Bob
// became an EVEN-bucket clue with its own anchor (above). The stable exemptions
// -- now `pace() <= 1` (v14.0.0) -- cut straight across that second bucket, so
// in these nine variants a colour clue stays reactive whatever the pace.
//
// RANK clues are untouched, which is what leaves the six Alternating Clues
// variants a stable channel. The three Synesthesia ones carry `clueRanks: []`,
// so they have none at all -- see the last test.

namespace {

// Three suits: 30 cards, 15 dealt, so with stacks of 3 the fixture sits at
// pace 3 and each discard costs one. Both entries in the pool are spare 1s --
// three copies, one on the stack and one in Bob's hand.
SetupOptions two_colour_opts(std::string variant_name, std::vector<int> stacks,
                             int clue_tokens, int discards = 0) {
  static const std::vector<std::string> kPool = {"r1", "b1"};
  SetupOptions opts;
  opts.variant_name = std::move(variant_name);
  opts.play_stacks = std::move(stacks);
  opts.clue_tokens = clue_tokens;
  for (int i = 0; i < discards && i < (int)kPool.size(); ++i) {
    opts.discarded.push_back(kPool[i]);
  }
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r4", "b4", "r3", "b3", "r2"},  // Alice (giver)
      {"r1", "b1", "r5", "b5", "r4"},  // Bob
      {"b2", "r2", "b3", "r3", "b4"},  // Cathy
  };
  use_reactor0(opts);
  return opts;
}

// The two clue kinds aimed at Bob, ready for `clue_is_reactive`.
struct BobClues { ClueAction colour; ClueAction rank; int bob; };

BobClues bob_clues(const Game& g) {
  const int alice = g.state.our_player_index;
  const int bob = g.state.next_player_index(alice);
  return BobClues{ClueAction{alice, bob, {}, BaseClue{ClueKind::COLOUR, 0}},
                  ClueAction{alice, bob, {}, BaseClue{ClueKind::RANK, 1}}, bob};
}

}  // namespace

// The other exemption, in the same variant. Asserted separately so a regression
// names which of the two arms it broke.
TEST(Reactor0TwoColours, ColourToBobIsNeverStableBelowPaceTwo) {
  Game g = setup(two_colour_opts("Alternating Clues & Rainbow (3 Suits)",
                                 {3, 3, 3}, /*clue_tokens=*/7, /*discards=*/2));
  ASSERT_EQ(g.state.pace(), 1) << "guard: on the stable side of the switch";
  ASSERT_FALSE(hanabi::reactor0::bob_clue_is_reactive(g.state))
      << "guard: the pace has stood the general rule down";

  const BobClues c = bob_clues(g);
  EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(g.state, c.colour, c.bob))
      << "a COLOUR clue to Bob stays reactive however low the pace";
  EXPECT_FALSE(hanabi::reactor0::clue_is_reactive(g.state, c.rank, c.bob))
      << "and a RANK clue to Bob is stable again, exactly as before v13.0.0";
}

// The negative that pins the rule to the COLOUR COUNT rather than to target
// parity generally: three colours and the stable switch works as it always did.
TEST(Reactor0TwoColours, ThreeColoursKeepsColourToBobStableBelowPaceTwo) {
  Game g = setup(two_colour_opts("Alternating Clues (3 Suits)", {3, 3, 3},
                                 /*clue_tokens=*/7, /*discards=*/2));
  ASSERT_EQ(g.state.pace(), 1) << "guard: on the stable side of the switch";
  ASSERT_EQ(g.state.variant->clue_colour_names.size(), 3u)
      << "guard: Red/Green/Blue, so the two-colour rule must not fire";
  EXPECT_FALSE(hanabi::reactor0::colour_is_never_stable(*g.state.variant));

  const BobClues c = bob_clues(g);
  EXPECT_FALSE(hanabi::reactor0::clue_is_reactive(g.state, c.colour, c.bob))
      << "with three colours a colour clue to Bob goes stable past the switch, "
         "unchanged by v13.0.0";
}

// SYNESTHESIA has no rank clues, so in its two-colour variants this leaves NO
// stable clue of any kind -- at any pace. That is the consequence the rule was
// accepted with, and `synesthesia_stable` is unreachable there as a result.
TEST(Reactor0TwoColours, TwoColourSynesthesiaHasNoStableClueAtAll) {
  for (int discards : {0, 2}) {
    for (const std::vector<int>& stacks :
         {std::vector<int>{0, 0, 0}, std::vector<int>{3, 3, 3}}) {
      Game g = setup(two_colour_opts("Synesthesia & Null (3 Suits)", stacks,
                                     /*clue_tokens=*/7, discards));
      ASSERT_TRUE(g.state.variant->clue_ranks.empty())
          << "guard: Synesthesia carries clueRanks: [], so colour is the only "
             "kind and there is nothing else to fall back on";
      const BobClues c = bob_clues(g);
      EXPECT_TRUE(hanabi::reactor0::clue_is_reactive(g.state, c.colour, c.bob))
          << "pace=" << g.state.pace() << " score=" << g.state.score()
          << ": every clue here is reactive, so nothing is ever stable";
    }
  }
}
