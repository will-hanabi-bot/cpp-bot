// Defect B (findings #3/#10/#11). reactor0's candidate walks try one target,
// and move on when it doesn't work out. But `target_play` / `target_discard`
// mutate the game EVEN ON FAILURE (documented at
// include/hanabi/conventions/reactor/interpret_clue.h), and the loops also
// abandon a candidate AFTER a successful reacter stamp when a later stage
// fails. The abandoned stamp is never rolled back.
//
// The harm is not "the reacter now holds two CTP cards" — a hand may
// legitimately carry several, actioned most-recently-stamped first. It is
// that one of them was never named by any clue: nobody agreed to it, so when
// the reacter plays it the receiver does not interpret the play, and because
// the spurious stamp is same-turn it competes with the real one for being
// actioned first.
//
// So the invariant is a diff: whatever a single clue settles on, the set of
// cards THAT CLUE stamped in the reacter's hand is exactly the one the
// waiting connection names — or empty when the clue is rejected.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Assert the clue just given stamped exactly the card it named.
void expect_stamped_exactly_the_named_card(const std::map<int, Mark>& before,
                                           const Game& g, TestPlayer reacter,
                                           const char* label) {
  std::vector<int> stamped = newly_marked(before, g, reacter);
  if (last_clue_interp(g) == ClueInterp::REACTIVE) {
    ASSERT_FALSE(g.waiting.empty()) << label << ": a reactive must leave a WC";
    int named = g.waiting.front().react_order;
    ASSERT_GE(named, 0) << label << ": the WC must record the called order";
    EXPECT_EQ(stamped, std::vector<int>{named})
        << label << ": a reactive stamps exactly the card it names; "
        << stamped.size()
        << " stamped means a candidate was abandoned but kept its mark";
  } else {
    EXPECT_TRUE(stamped.empty())
        << label << ": a rejected clue must leave the reacter unstamped, but "
        << stamped.size() << " card(s) kept a mark";
  }
}

}  // namespace

TEST(Reactor0CandidateRollback, AbandonedCandidateLeavesNoStamp) {
  SetupOptions opts;
  // Cathy has playables at slot 1 (r1) and slot 3 (g1). Anchor 2 maps
  // slot 1 -> react_slot calc_slot(2,1,5) = 1 and slot 3 -> react_slot 4.
  // Bob's slots 1 and 4 both pass the effective-possible vetting.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "g1", "y4"},   // Bob: slot 1 y1, slot 4 g1
      {"r1", "y2", "g1", "b4", "p4"},   // Cathy: playables at slots 1 and 3
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Make the FIRST candidate fail late: narrow Cathy's slot-1 common
  // inferred to a non-playable, non-connector singleton so
  // stamp_receiver_play's filter empties and Phase A abandons it -- after
  // it has already stamped Bob. (with_thought touches common only and is
  // not re-elim'd; see tests/test_basics/test_stable_ref_discard.cpp.)
  int cathy_s1 = order_at(g, TestPlayer::CATHY, 1);
  Identity p4{4, 4};
  g.with_thought(cathy_s1, [p4](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(p4);
    return out;
  });
  ASSERT_EQ(g.common.thoughts[cathy_s1].inferred.length(), 1u)
      << "fixture must leave Cathy's slot 1 narrowed to a non-playable";

  auto before = hand_marks(g, TestPlayer::BOB);
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  expect_stamped_exactly_the_named_card(before, g, TestPlayer::BOB,
                                        "abandoned first candidate");
}

TEST(Reactor0CandidateRollback, PriorStampsAreLeftAlone) {
  SetupOptions opts;
  // Bob already carries an urgent CTP on slot 3 from an earlier clue. A new
  // reactive must add exactly its own stamp and disturb nothing else --
  // several CTP cards at once is a legal state, so rollback must restore
  // the prior marks rather than clear the hand.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b1", "g1", "y4"},   // Bob
      {"r1", "y2", "g1", "b4", "p4"},   // Cathy
  };
  use_reactor0(opts);
  Game g = setup(opts);

  int bob_s3 = order_at(g, TestPlayer::BOB, 3);
  g.with_meta(bob_s3, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m = m.reason(0).signal(0);
  });

  auto before = hand_marks(g, TestPlayer::BOB);
  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  expect_stamped_exactly_the_named_card(before, g, TestPlayer::BOB,
                                        "reactive over a prior CTP");
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 3), CardStatus::CALLED_TO_PLAY)
      << "the earlier clue's CTP must survive — multiple CTP stamps are a "
         "legal state, actioned most-recently-stamped first";
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 3))
      << "and it must stay urgent";
}

TEST(Reactor0CandidateRollback, StampInvariantHoldsAcrossReactivePositions) {
  // The same invariant over the positions the rest of the suite builds, so
  // a future refactor that reintroduces stamp-then-abandon is caught even
  // if it fires on a different path.
  struct Case {
    const char* name;
    std::vector<std::vector<std::string>> hands;
    std::vector<int> stacks;
    const char* clue;
  };
  const std::vector<Case> cases = {
      {"rank double play",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"g1", "y3", "b3", "g4", "y4"},
        {"r1", "y2", "g3", "b4", "p5"}},
       {0, 0, 0, 0, 0},
       "Alice clues 2 to Cathy"},
      {"colour dc+play",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"y2", "y3", "b3", "g4", "y4"},
        {"r1", "y2", "g3", "b4", "p2"}},
       {0, 0, 0, 0, 0},
       "Alice clues red to Cathy"},
      {"rank double discard",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"y2", "y3", "b3", "g4", "y4"},
        {"y4", "b4", "r1", "g4", "p4"}},
       {1, 0, 0, 0, 0},
       "Alice clues 4 to Cathy"},
  };

  for (const auto& c : cases) {
    SetupOptions opts;
    opts.hands = c.hands;
    opts.play_stacks = c.stacks;
    use_reactor0(opts);
    Game g = setup(opts);

    auto before = hand_marks(g, TestPlayer::BOB);
    g = take_turn(std::move(g), c.clue);

    expect_stamped_exactly_the_named_card(before, g, TestPlayer::BOB, c.name);
  }
}
