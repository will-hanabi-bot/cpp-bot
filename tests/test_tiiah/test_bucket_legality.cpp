// The bucket relation is a LEGALITY test on the giver, not a filter on the walk
// (tiiah/CONVENTION.md §1d, v16.15.0).
//
// It is computed from the reacter's own card, which the reacter cannot see. So it
// may not decide which pairing a clue names: the giver would skip a pairing the
// reacter walks straight into, and the two would act on different slots. What it
// decides instead is whether the giver may give the clue at all.
//
// `test_bucket_encoding.cpp` has the single-candidate version of this — one
// pairing, relation fails, nothing read. That case cannot tell a legality test
// from a walk filter, because there is nothing to retarget TO. These two can.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "test_harness.h"
#include "test_tiiah/test_tiiah_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::tiiah;

namespace {

std::optional<ClueInterp> interp_of(const Game& g) {
  if (g.move_history.empty()) return std::nullopt;
  if (auto* c = std::get_if<ClueInterp>(&g.move_history.back())) return *c;
  return std::nullopt;
}

}  // namespace

// THE GIVER. Replay 2010246 T2 in miniature: a rank 2 with two pairings, the
// first illegal and the second a legal finesse behind it.
//
// Nothing is played, so the receiver's only direct playable is the `r1` on slot
// 5, and rank 2 anchors the reaction on the reacter's slot 2 — a `y1`. Both are
// bucket 0, and a rank clue wants the target one bucket HIGHER, so that pairing
// is illegal. One slot further on lies a legal one: the `y2` on the receiver's
// slot 1 is one away, which anchors slot 1 and makes the `y1` there its finesse
// connector.
//
// The giver may not reach past the illegal pairing to the legal one. It sees the
// reacter's card and the reacter does not, so a clue given on that basis names
// one slot to the giver and another to the reacter -- which is what happened in
// 2010246, where the reacter played the slot the giver had skipped.
TEST(TiiahBucketLegality, TheGiverMayNotReachPastAnIllegalPairing) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // us, the giver
      {"y1", "y1", "b4", "p4", "g4"},  // the reacter: a y1 at slots 1 AND 2
      {"y2", "b4", "p4", "g4", "r1"},  // the receiver: r1 direct, y2 one away
  };
  opts.starting = TestPlayer::ALICE;
  use_tiiah(opts);
  Game g = setup(std::move(opts));

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  EXPECT_NE(interp_of(g), ClueInterp::REACTIVE)
      << "the first pairing breaks the relation, so the clue is illegal -- not "
         "retargeted onto the finesse behind it. Before v16.15.0 this read as "
         "REACTIVE and named the reacter's slot 1, while the reacter itself, "
         "unable to see its own card, would have played slot 2";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 1), CardStatus::NONE);
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::NONE)
      << "and an illegal clue stamps nothing at all";
}

// THE READER, same shape from the other side. The giver is Cathy, so its Bob --
// the reacter -- is us, and its Cathy is the seat holding the targets.
//
// We cannot see our own card, so the relation is not ours to evaluate, and the
// walk is pure shared information: we take the FIRST pairing, the direct `r1`.
// That is the whole point of moving the test to the giver. Whatever the giver
// could see, every reader lands where the reacter lands.
TEST(TiiahBucketLegality, AReaderTakesTheFirstPairingRegardless) {
  SetupOptions opts;
  opts.variant_name = "Throw It in a Hole (5 Suits)";
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},  // us, the reacter
      {"y2", "b4", "p4", "g4", "r1"},  // the receiver: r1 direct, y2 one away
      {"y1", "y1", "b4", "p4", "g4"},  // the giver
  };
  opts.starting = TestPlayer::CATHY;
  use_tiiah(opts);
  Game g = setup(std::move(opts));
  const int direct = order_at(g, TestPlayer::BOB, 5);  // the receiver's r1

  g = take_turn(std::move(g), "Cathy clues 2 to Bob");

  ASSERT_EQ(interp_of(g), ClueInterp::REACTIVE)
      << "read, because nothing a reader can see refuses it";
  ASSERT_FALSE(g.waiting.empty());
  EXPECT_EQ(g.waiting.front().receiver_target_order, direct)
      << "and it is the FIRST pairing -- the direct r1 -- not the finesse a "
         "bucket-checking giver would have skipped to";
  EXPECT_EQ(status_at(g, TestPlayer::ALICE, 2), CardStatus::CALLED_TO_PLAY)
      << "so our slot 2 is what we are called to play, which is the slot the "
         "reacter in 2010246 actually played";
}
