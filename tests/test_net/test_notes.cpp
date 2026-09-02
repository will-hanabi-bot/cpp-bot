// `compute_note_segments` -- the diff that turns a state change into a note.
//
// Four segment kinds announce a CALL and carry a bracket saying which:
// `[f]` play, `[d]` discard, `[reset]`, `[?]`. The fifth, added in v14.2.0,
// announces no call at all: an UNSTAMPED card of our own whose candidate set has
// narrowed to something a reader can hold in their head gets the ids alone, as
// `turn 11: r2,r5`.
//
// Nothing else records that. The other four all wait for a call, so without this
// a replay reader cannot reconstruct what we knew about a card nobody ever told
// us anything about -- which is exactly the hand they cannot see for themselves.
//
// The fixtures build the two Games and edit thoughts directly rather than going
// through a convention. What is under test is the DIFF, and driving it with real
// clues would make each case depend on whichever ladder happened to stamp.
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/net/notes.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

SetupOptions base_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"r1", "r2", "r3", "r4", "r5"},   // Alice -- ours, and hidden from us
      {"g1", "g2", "g3", "g4", "g5"},   // Bob
      {"b1", "b2", "b3", "b4", "b5"},   // Cathy
  };
  return opts;
}

// No Variant, so the suits are r y g b p at indices 0-4.
IdentitySet ids_of(std::initializer_list<Identity> list) {
  IdentitySet out;
  for (Identity i : list) out = out.union_with(IdentitySet::single(i));
  return out;
}
constexpr Identity kR1{0, 1}, kR2{0, 2}, kR3{0, 3}, kR4{0, 4}, kR5{0, 5};
constexpr Identity kG1{2, 1}, kG2{2, 2}, kG3{2, 3};

// Narrow one card in our own hand, from our own POV, and return the segments.
std::vector<std::pair<int, std::string>> narrow(Game g, int slot,
                                                 std::initializer_list<Identity> to,
                                                 CardStatus status = CardStatus::NONE) {
  const int me = g.state.our_player_index;
  const int order = g.state.hands[me][slot - 1];
  Game prev = g;
  g.players[me].thoughts[order].inferred = ids_of(to);
  g.meta[order].status = status;
  return hanabi::net::compute_note_segments(prev, g);
}

const std::string* segment_for(
    const std::vector<std::pair<int, std::string>>& segs, int order) {
  for (const auto& [o, seg] : segs) {
    if (o == order) return &seg;
  }
  return nullptr;
}

}  // namespace

TEST(Notes, ANarrowedUnstampedCardOfOursIsNoted) {
  Game g = setup(base_opts());
  const int order = g.state.hands[g.state.our_player_index][2];
  auto segs = narrow(g, 3, {kR2, kR5});

  const std::string* seg = segment_for(segs, order);
  ASSERT_NE(seg, nullptr) << "a two-candidate card must be noted";
  EXPECT_EQ(*seg, "turn " + std::to_string(g.state.turn_count) + ": r2,r5")
      << "the ids alone -- no bracket, because no call was made";
}

TEST(Notes, SixCandidatesAreNotedAndSevenAreNot) {
  Game g = setup(base_opts());
  const int order = g.state.hands[g.state.our_player_index][2];

  auto six = narrow(g, 3, {kR1, kR2, kR3, kR4, kR5, kG1});
  EXPECT_NE(segment_for(six, order), nullptr)
      << "six is the boundary and is inside it";

  auto seven = narrow(g, 3, {kR1, kR2, kR3, kR4, kR5, kG1, kG2});
  EXPECT_EQ(segment_for(seven, order), nullptr)
      << "seven candidates is more than a reader can use, and is where the "
         "note traffic would start to matter";
}

// The four call segments still own a stamped card: they say WHICH call was made,
// which a bare id list cannot.
TEST(Notes, AStampedCardKeepsItsCallSegment) {
  Game g = setup(base_opts());
  const int order = g.state.hands[g.state.our_player_index][2];

  auto segs = narrow(g, 3, {kR2, kR5}, CardStatus::CALLED_TO_PLAY);
  const std::string* seg = segment_for(segs, order);
  ASSERT_NE(seg, nullptr);
  EXPECT_NE(seg->find("[f]"), std::string::npos)
      << "a newly stamped card notes as a play call, not as bare empathy: " << *seg;
}

// Our own hand only. A partner's card is face-up to a replay reader, and
// `me_new.thoughts` for one is our SIGHT of it rather than that seat's empathy --
// so a note there would be both redundant and mislabelled. It also holds the
// traffic to a third.
TEST(Notes, APartnersCardIsNotNoted) {
  Game g = setup(base_opts());
  const int bob = static_cast<int>(TestPlayer::BOB);
  const int order = g.state.hands[bob][2];
  Game prev = g;
  g.players[g.state.our_player_index].thoughts[order].inferred =
      ids_of({kG3});

  auto segs = hanabi::net::compute_note_segments(prev, g);
  EXPECT_EQ(segment_for(segs, order), nullptr)
      << "however far a partner's card narrows, it is not ours to note";
}

// A card that has left the hand cannot narrow in any sense a reader cares about,
// and `compute_note_segments` walks every order in `meta` -- including played and
// discarded ones -- so the hand check is what rules it out.
TEST(Notes, ACardThatHasLeftTheHandIsNotNoted) {
  Game g = setup(base_opts());
  const int me = g.state.our_player_index;
  const int order = g.state.hands[me][0];

  Game prev = g;
  g.players[me].thoughts[order].inferred = ids_of({kR1});
  g.state.hands[me].erase(std::remove(g.state.hands[me].begin(),
                                       g.state.hands[me].end(), order),
                          g.state.hands[me].end());

  auto segs = hanabi::net::compute_note_segments(prev, g);
  EXPECT_EQ(segment_for(segs, order), nullptr);
}

// No change, no note: the segment fires on a NARROWING, not on every turn.
TEST(Notes, AnUnchangedCardIsNotRenoted) {
  Game g = setup(base_opts());
  const int me = g.state.our_player_index;
  const int order = g.state.hands[me][2];
  g.players[me].thoughts[order].inferred = ids_of({kR2, kR5});

  Game prev = g;
  auto segs = hanabi::net::compute_note_segments(prev, g);
  EXPECT_EQ(segment_for(segs, order), nullptr);
}
