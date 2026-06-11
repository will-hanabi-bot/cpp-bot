// Hanab.live replay 1885899, variant "Orange (3 Suits)". Orig players:
// P0=will-bot69, P1=yagami_black, P2=will-bot67.
//
// At T1 will-bot69 gave a rank-5 clue to will-bot67. The convention's
// `interpret_reactive_rank` finesse path picked react_slot=1 (yagami's
// slot 1) via `effective_possible.contains(o1)`. From the giver's POV,
// the existing `would_lose_inverted_reacter` then *skipped* slot 1
// (yagami's actual slot 1 is o4, not playable) and fell through to
// slot 3 (= yagami's o1). The giver's simulation thought yagami would
// PerformDiscard slot 3 → orange stack 0→1.
//
// But yagami's own POV can't see her own slot 1's identity, so
// `would_lose_inverted_reacter` returns false at the suit_index<0
// guard and her convention picks slot 1. She PerformDiscards slot 1
// (= o4); on_discard(inverted, !failed) attempts with_play(o4); o4
// isn't playable on stack 0 → strike.
//
// The fix in `interpret_reactive_rank`'s finesse loop adds a POV-
// invariant guard: when the picked slot's *actual* identity is known
// to the giver and doesn't equal `prev_id` (= the prereq the reacter's
// POV believes the slot to hold), the whole clue interprets as
// MISTAKE — the giver won't give it.
#include <gtest/gtest.h>

#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

namespace {

// Build from will-bot69's perspective (orig P0 = ALICE = observer).
// Identity / cycle is trivial: orig P_i = MY P_i. Initial hands are
// the deal, newest-first.
Game build_from_bot69_perspective() {
  SetupOptions opts;
  opts.hands = {
      // ALICE = will-bot69 (observer). Initial hand orig 0..4 = MY 0..4
      // newest-first: 4=(0,1)=r1, 3=(0,4)=r4, 2=(2,3)=o3, 1=(2,1)=o1,
      // 0=(1,1)=b1.
      {"r1", "r4", "o3", "o1", "b1"},
      // BOB = yagami. Initial orig 5..9, newest-first:
      // 9=(2,4)=o4, 8=(0,1)=r1, 7=(2,1)=o1, 6=(1,2)=b2, 5=(0,3)=r3.
      // Slot 1 = o4 (the buggy mismatched-prereq slot).
      {"o4", "r1", "o1", "b2", "r3"},
      // CATHY = will-bot67. Initial orig 10..14, newest-first:
      // 14=(1,3)=b3, 13=(2,2)=o2, 12=(2,5)=o5, 11=(1,4)=b4,
      // 10=(2,2)=o2.
      // Slot 2 = o2 (finesse target); slot 3 = o5 (rank-5 focus).
      {"b3", "o2", "o5", "b4", "o2"},
  };
  opts.variant_name = "Orange (3 Suits)";
  opts.starting = TestPlayer::ALICE;
  return setup(std::move(opts));
}

}  // namespace

// At T1 the live will-bot69 issued `PerformRank{target=will-bot67=
// CATHY, value=5}`. After the prereq-mismatch guard, the bot must
// pick anything else.
TEST(EndgameReplay1885899, Turn1RejectsRank5ToBot67) {
  Game g = build_from_bot69_perspective();
  ASSERT_EQ(g.state.current_player_index, static_cast<int>(TestPlayer::ALICE));

  PerformAction perform = g.take_action();
  if (std::holds_alternative<PerformRank>(perform)) {
    auto p = std::get<PerformRank>(perform);
    EXPECT_FALSE(p.target == static_cast<int>(TestPlayer::CATHY) && p.value == 5)
        << "regression: rank-5 to will-bot67 would force yagami (reacter) "
           "to PerformDiscard slot 1 (= o4 actual) and strike, since the "
           "convention's slot search picks slot 1 from yagami's POV";
  }
}
