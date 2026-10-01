// `variants::rank_played_in`: is a rank already played in a suit, judged in the
// suit's direction? The pink trash clue's condition (reactor0 CONVENTION.md §1c
// priority 0, v19.0.0). No variant pairs a pinkish suit with a reversed one yet
// -- the only candidates are the unsupported Up or Down ones -- so the
// direction rule is tested on a plain reversed suit; it is direction logic
// only.
#include <gtest/gtest.h>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/variants/pinkish.h"

using namespace hanabi;
namespace variants = hanabi::reactor::variants;

namespace {

// "Reversed (5 Suits)": Red, Yellow, Green, Blue, Purple Reversed (index 4).
State reversed_state() {
  const Variant& v = get_variant("Reversed (5 Suits)");
  TableOptions opts;
  opts.num_players = 3;
  opts.variant_name = "Reversed (5 Suits)";
  return State::create({"Alice", "Bob", "Cathy"}, /*our_player_index=*/0, v,
                       std::move(opts));
}

}  // namespace

// Ascending: a rank is down once the stack has reached it.
TEST(RankPlayedIn, AnAscendingSuitHasPlayedEveryRankUpToItsStack) {
  State s = reversed_state();
  EXPECT_FALSE(variants::rank_played_in(s, 0, 1)) << "nothing played yet";
  s = s.with_play(Identity(0, 1)).with_play(Identity(0, 2));
  EXPECT_TRUE(variants::rank_played_in(s, 0, 1));
  EXPECT_TRUE(variants::rank_played_in(s, 0, 2));
  EXPECT_FALSE(variants::rank_played_in(s, 0, 3));
}

// Descending (user ruling): `value >= stack`. The reversed stack starts at 6,
// so nothing counts as played until the 5 goes down.
TEST(RankPlayedIn, ADescendingSuitHasPlayedEveryRankDownToItsStack) {
  State s = reversed_state();
  EXPECT_FALSE(variants::rank_played_in(s, 4, 5)) << "nothing played yet";
  s = s.with_play(Identity(4, 5)).with_play(Identity(4, 4));
  EXPECT_TRUE(variants::rank_played_in(s, 4, 5));
  EXPECT_TRUE(variants::rank_played_in(s, 4, 4));
  EXPECT_FALSE(variants::rank_played_in(s, 4, 3));
  EXPECT_FALSE(variants::rank_played_in(s, 4, 1));
}
