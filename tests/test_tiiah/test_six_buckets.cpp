// EXPERIMENTAL (branch exp/six-buckets): six suit buckets with an epoch shift.
//
// A TIIAH variant with six non-inverted suits gives every suit its own bucket,
// numbered by suit index (red 0 ... black 5). The clue's EPOCH is ceil(turn / 3),
// turn 1-based: on an odd epoch a rank clue puts the receiver's target one bucket
// UP from the reacter's card and a colour clue one DOWN; on an even epoch two.
// Every other TIIAH variant keeps three buckets and +-1 (`buckets.h`).
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/tiiah/buckets.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;
using hanabi::tiiah::bucket_count;
using hanabi::tiiah::bucket_of;
using hanabi::tiiah::bucket_shift;
using hanabi::tiiah::named_bucket;
using hanabi::tiiah::reacter_bucket_for;
using hanabi::tiiah::six_suit_buckets;
using hanabi::tiiah::variant_buckets;

namespace {

constexpr const char* kBlack = "Throw It in a Hole & Black (6 Suits)";

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

// TIIAH, with Alice to act on `turn` (1-based) rather than turn 1.
void use_tiiah_at_turn(SetupOptions& opts, int turn) {
  opts.init = [turn](Game& g) {
    g.convention = Convention::TIIAH;
    g.state.turn_count = turn;
    if (static_cast<int>(g.state.action_list.size()) < turn) {
      g.state.action_list.resize(turn);
    }
  };
}

}  // namespace

TEST(TiiahSixBuckets, BlackSixSuitsHasOneBucketPerSuit) {
  const Variant& v = get_variant(kBlack);
  ASSERT_TRUE(six_suit_buckets(v));
  EXPECT_EQ(bucket_count(v), 6);
  EXPECT_EQ(variant_buckets(v),
            (std::vector<std::vector<int>>{{0}, {1}, {2}, {3}, {4}, {5}}));
  for (int s = 0; s < 6; ++s) EXPECT_EQ(bucket_of(v, s), s);
}

TEST(TiiahSixBuckets, OtherTiiahVariantsKeepThreeBuckets) {
  for (const char* name : {"Throw It in a Hole (5 Suits)",
                           "Throw It in a Hole & Orange (6 Suits)"}) {
    const Variant& v = get_variant(name);
    EXPECT_FALSE(six_suit_buckets(v)) << name;
    EXPECT_EQ(bucket_count(v), 3) << name;
    EXPECT_EQ(variant_buckets(v).size(), 3u) << name;
    // +-1 on every turn, whatever the epoch.
    for (int t : {1, 4, 7}) {
      EXPECT_EQ(bucket_shift(v, ClueKind::RANK, t), 1) << name;
      EXPECT_EQ(bucket_shift(v, ClueKind::COLOUR, t), -1) << name;
    }
  }
  // A six-suit variant OUTSIDE Throw It in a Hole is not a TIIAH variant at all.
  EXPECT_FALSE(six_suit_buckets(get_variant("Black (6 Suits)")));
}

TEST(TiiahSixBuckets, TheShiftFollowsTheEpoch) {
  const Variant& v = get_variant(kBlack);
  // Epoch 1 = turns 1-3, epoch 2 = turns 4-6, epoch 3 = turns 7-9.
  for (int t : {1, 2, 3, 7, 8, 9}) {
    EXPECT_EQ(bucket_shift(v, ClueKind::RANK, t), 1) << t;
    EXPECT_EQ(bucket_shift(v, ClueKind::COLOUR, t), -1) << t;
  }
  for (int t : {4, 5, 6, 10}) {
    EXPECT_EQ(bucket_shift(v, ClueKind::RANK, t), 2) << t;
    EXPECT_EQ(bucket_shift(v, ClueKind::COLOUR, t), -2) << t;
  }
}

TEST(TiiahSixBuckets, TheRelationWrapsModSix) {
  const Variant& v = get_variant(kBlack);
  // Rank from black on an odd epoch is red; colour from red on an even epoch is blue.
  EXPECT_EQ(named_bucket(v, ClueKind::RANK, 1, 5), 0);
  EXPECT_EQ(named_bucket(v, ClueKind::COLOUR, 4, 0), 4);
  EXPECT_EQ(named_bucket(v, ClueKind::RANK, 4, 5), 1);
  EXPECT_EQ(named_bucket(v, ClueKind::COLOUR, 1, 0), 5);
  // The inverse undoes it, in every epoch, from every bucket.
  for (int t : {1, 4}) {
    for (ClueKind k : {ClueKind::RANK, ClueKind::COLOUR}) {
      for (int b = 0; b < 6; ++b) {
        EXPECT_EQ(reacter_bucket_for(v, k, t, named_bucket(v, k, t, b)), b);
        EXPECT_NE(named_bucket(v, k, t, b), b) << "never the reacter's own suit";
      }
    }
  }
}

// Red is on 2 and Bob knows his r3, so a clue to him reverses; once the r3 is
// assumed played his want is the g1 (bucket 2) and rank 5 anchors the reaction
// on Cathy's slot 4. Epoch 1, rank: the reacter sits one bucket below green, in
// yellow, whose only playable is the y1.
TEST(TiiahSixBuckets, ARankClueInAnOddEpochNamesOneBucketBelow) {
  SetupOptions opts;
  opts.variant_name = kBlack;
  opts.play_stacks = std::vector<int>{2, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "r3", "y5", "g5", "b5"},
      {"b4", "p4", "k4", "y1", "k3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah_at_turn(opts, 1);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/2, "r3");

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 4), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/4, {"y1"});
}

// The same clue on turn 4 (epoch 2) names TWO buckets below green: red, whose
// playable after the queued r3 is the r4.
TEST(TiiahSixBuckets, ARankClueInAnEvenEpochNamesTwoBucketsBelow) {
  SetupOptions opts;
  opts.variant_name = kBlack;
  opts.play_stacks = std::vector<int>{2, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "r3", "y5", "g5", "b5"},
      {"b4", "p4", "k4", "r4", "k3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah_at_turn(opts, 4);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/2, "r3");

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 4), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/4, {"r4"});
}

// On turn 4 the y1 that answered on turn 1 breaks the relation, so Alice, who
// can see it, may not give the clue as that pairing.
TEST(TiiahSixBuckets, TheOddEpochPairingIsIllegalInAnEvenEpoch) {
  SetupOptions opts;
  opts.variant_name = kBlack;
  opts.play_stacks = std::vector<int>{2, 0, 0, 0, 0, 0};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"g1", "r3", "y5", "g5", "b5"},
      {"b4", "p4", "k4", "y1", "k3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah_at_turn(opts, 4);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/2, "r3");

  g = take_turn(std::move(g), "Alice clues 5 to Bob");

  EXPECT_NE(status_at(g, TestPlayer::CATHY, 4), CardStatus::CALLED_TO_PLAY);
}

// Colour: Bob knows his r1 and the clue is for the g1 behind it (bucket 2).
// Purple anchors on 5 and the target is slot 2, so the reaction is slot 3.
// Epoch 1 puts the reacter one bucket ABOVE green, in blue: the b1.
TEST(TiiahSixBuckets, AColourClueInAnOddEpochNamesOneBucketAbove) {
  SetupOptions opts;
  opts.variant_name = kBlack;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g1", "y4", "b4", "p4"},
      {"y3", "g3", "b1", "k3", "r3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah_at_turn(opts, 2);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues purple to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/3, {"b1"});
}

// ...and epoch 2 two buckets above green, in purple: the p1.
TEST(TiiahSixBuckets, AColourClueInAnEvenEpochNamesTwoBucketsAbove) {
  SetupOptions opts;
  opts.variant_name = kBlack;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"r1", "g1", "y4", "b4", "p4"},
      {"y3", "g3", "p1", "k3", "r3"},
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah_at_turn(opts, 6);
  Game g = setup(std::move(opts));
  g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/1, "r1");

  g = take_turn(std::move(g), "Alice clues purple to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE);
  EXPECT_EQ(status_at(g, TestPlayer::CATHY, 3), CardStatus::CALLED_TO_PLAY);
  expect_infs(g, std::nullopt, TestPlayer::CATHY, /*slot=*/3, {"p1"});
}

// The giver's hypothetical (`Game::simulate`, what clue selection runs) reads the
// clue on the same turn as the real clue, at the last turn of an epoch and the
// first of the next, so the two never disagree on the shift.
TEST(TiiahSixBuckets, TheGiversSimulationReadsTheSameEpoch) {
  for (int turn : {3, 4}) {
    SetupOptions opts;
    opts.variant_name = kBlack;
    opts.play_stacks = std::vector<int>{2, 0, 0, 0, 0, 0};
    opts.hands = {
        {"xx", "xx", "xx", "xx", "xx"},
        {"g1", "r3", "y5", "g5", "b5"},
        {"b4", "p4", "k4", turn == 3 ? "y1" : "r4", "k3"},
    };
    opts.starting = TestPlayer::ALICE;
    use_tiiah_at_turn(opts, turn);
    Game g = setup(std::move(opts));
    g = fully_known(std::move(g), TestPlayer::BOB, /*slot=*/2, "r3");

    const Game hypo = g.simulate(parse_action(g.state, "Alice clues 5 to Bob"));
    const Game real = take_turn(g, "Alice clues 5 to Bob");

    ASSERT_EQ(interp_of(real), ClueInterp::REACTIVE) << turn;
    ASSERT_EQ(interp_of(hypo), ClueInterp::REACTIVE) << turn;
    const int o = order_at(real, TestPlayer::CATHY, 4);
    EXPECT_EQ(hypo.meta[o].status, real.meta[o].status) << turn;
    EXPECT_EQ(hypo.common.thoughts[o].inferred, real.common.thoughts[o].inferred) << turn;
  }
}
