// A reactive clue must not call TWO COPIES of the same card to play.
//
// The reacter acts first, so their copy stacks the identity and the receiver's
// is trash by the time they act -- but the receiver still reads their card as
// "the playable one", which by then is the NEXT rank, and bombs.
//
// Replay 1967363 T1 (Odds and Evens & Orange): both halves of a double play
// were an Orange 1. will-bot67 chucked theirs onto the stack and yagami_black
// chucked theirs for a strike, "thinking it is orange 2".
//
// The veto is giver-side and a candidate FILTER, not a reading: it needs the
// giver's sight of two hands, which no other seat has, so making it a reading
// would have observers decode the clue differently from the giver. It lives in
// `analyse_clues`, beside the MISTAKE drop.
//
// It is variant-independent -- the hazard is the double-play SHAPE, not the
// inverted suit -- and covers PLAYS only.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::ClueCandidate;

namespace {

// The same enumeration `take_action` performs before handing the set to
// `analyse_clues` (basics/decide.cpp).
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

bool offers_colour(const std::vector<ClueCandidate>& cs, int target, int value) {
  for (const auto& c : cs) {
    if (auto* pc = std::get_if<PerformColour>(&c.perform)) {
      if (pc->target == target && pc->value == value) return true;
    }
  }
  return false;
}

// Replay 1967363's shape. Odds and Evens makes a COLOUR clue the even-parity
// family, so blue to Cathy is a double play; both the reacter and the receiver
// hold a playable o1.
SetupOptions dup_opts(std::string cathy_slot2) {
  SetupOptions opts;
  opts.variant_name = "Odds and Evens & Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 0, 0};
  opts.hands = {
      {"r2", "r3", "b3", "b4", "r4"},              // Alice (giver, us)
      {"o1", "o1", "o3", "r1", "o4"},              // Bob (reacter)
      {"b2", std::move(cathy_slot2), "r5", "o3", "b1"},  // Cathy (receiver)
  };
  use_reactor0(opts);
  return opts;
}

constexpr int kBlue = 1;
constexpr int kCathy = 2;

}  // namespace

TEST(Reactor0DuplicatePlayVeto, AClueCallingTwoCopiesToPlayIsNotOffered) {
  Game g = setup(dup_opts("o1"));  // Cathy holds the third o1
  EXPECT_FALSE(offers_colour(analysed(g), kCathy, kBlue))
      << "the reacter's o1 stacks first, so the receiver's o1 would bomb";
}

// The mirror on the same fixture with one card changed: without the duplicate
// the very same clue is offered, so what is pinned is the DUPLICATION and not
// something incidental about the position.
TEST(Reactor0DuplicatePlayVeto, TheSameClueIsOfferedWithoutTheDuplicate) {
  Game g = setup(dup_opts("b5"));  // Cathy's slot 2 is no longer an o1
  EXPECT_TRUE(offers_colour(analysed(g), kCathy, kBlue))
      << "with no second copy in play there is nothing to veto";
}

// The carve-out: a receiver who already knows exactly what they hold cannot be
// fooled into pressing the button on a copy that has just died.
TEST(Reactor0DuplicatePlayVeto, AReceiverWhoKnowsTheCardIsNotVetoed) {
  Game g = setup(dup_opts("o1"));
  ASSERT_FALSE(offers_colour(analysed(g), kCathy, kBlue)) << "guard: vetoed first";

  // Pin Cathy's o1 in common knowledge, the way a resolving clue history would.
  const int order = order_at(g, TestPlayer::CATHY, 2);
  const IdentitySet one = IdentitySet::single(Identity{2, 1});  // o1
  g.with_thought(order, [one](const Thought& t) {
    Thought out = t;
    out.inferred = one;
    out.possible = one;
    return out;
  });

  EXPECT_TRUE(offers_colour(analysed(g), kCathy, kBlue))
      << "she will see the o1 land and know hers is dead";
}

// The hazard is the double-play SHAPE, not the inverted suit. In a plain
// variant a RANK reactive is the even-parity family, and two copies of a plain
// playable fail exactly the same way -- pitch rather than chuck.
TEST(Reactor0DuplicatePlayVeto, PlainSuitDuplicatesAreVetoedToo) {
  auto opts_for = [](std::string cathy_slot1) {
    SetupOptions o;
    o.variant_name = "No Variant";
    o.starting = TestPlayer::ALICE;
    o.play_stacks = {0, 0, 0, 0, 0};
    o.hands = {
        {"y3", "g3", "b3", "p3", "y4"},
        {"r1", "y2", "g2", "b2", "p2"},                 // Bob (reacter): r1
        {std::move(cathy_slot1), "y5", "g5", "b5", "p5"},  // Cathy (receiver)
    };
    use_reactor0(o);
    return o;
  };

  // Both hold the playable r1: every offered clue must avoid pairing them.
  Game dup = setup(opts_for("r1"));
  int reactive_candidates = 0;
  for (const auto& c : analysed(dup)) {
    const Game hypo = dup.simulate(Action{c.action});
    if (hypo.waiting.empty()) continue;
    ++reactive_candidates;
    const ReactorWC& wc = hypo.waiting.front();
    auto called_play = [&](int player) -> std::optional<Identity> {
      for (int o : dup.state.hands[player]) {
        if (hypo.meta[o].status == dup.meta[o].status) continue;
        auto id = dup.state.deck[o].id();
        if (!id) continue;
        if (hypo.meta[o].status == CardStatus::CALLED_TO_PLAY &&
            dup.state.is_playable(*id)) {
          return id;
        }
      }
      return std::nullopt;
    };
    auto a = called_play(wc.reacter);
    auto b = called_play(wc.receiver);
    EXPECT_FALSE(a && b && *a == *b)
        << "a candidate still pairs two copies of the same playable";
  }
  // Without this the loop could be empty and the test would pass vacuously.
  EXPECT_GT(reactive_candidates, 0)
      << "the fixture must actually offer reactive clues to be worth checking";
}
