// Game::chop selection rules (CONVENTION.md §2.3).
//
// Two passes: an explicit CALLED_TO_DISCARD wins, and among several the one
// with the largest signal_turn is the chop -- not the newest by hand position.
// Falling through, the chop is the newest unclued status-NONE card, gated by
// zcs_turn. Anything with a non-NONE status is ineligible, so a hand whose
// unclued cards are all CHOP_MOVED (a LOCK) has no chop at all.
#include <gtest/gtest.h>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

using namespace hanabi;

namespace {

Game make_game() {
  const Variant& v = get_variant("No Variant");
  TableOptions opts;
  opts.num_players = 3;
  opts.variant_name = "No Variant";
  State s = State::create({"Alice", "Bob", "Cathy"}, /*our_player_index=*/0, v,
                          std::move(opts));
  return Game::create(/*table_id=*/0, std::move(s));
}

// Give Alice five cards at orders 0..4. Slot 1 (newest) ends up at order 4.
void deal_alice(Game& g) {
  for (int order = 0; order < 5; ++order) {
    g.handle_action(DrawAction{0, order, /*suit_index=*/0, /*rank=*/1});
  }
}

void mark_ctd(Game& g, int order, int signal_turn) {
  g.with_meta(order, [signal_turn](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.signal_turn = signal_turn;
  });
}

}  // namespace

TEST(Chop, NewestUncluedWhenNoCtd) {
  Game g = make_game();
  deal_alice(g);
  // Hand is [4, 3, 2, 1, 0] newest-first; order 4 is slot 1.
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 4);
}

TEST(Chop, ExplicitCtdBeatsPositionalChop) {
  Game g = make_game();
  deal_alice(g);
  mark_ctd(g, /*order=*/1, /*signal_turn=*/7);
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 1) << "an explicit CTD outranks the newest unclued card";
}

// The rule this file exists for: hand position and signal order disagree.
// Order 3 sits nearer slot 1 than order 1, but order 1 was signalled later.
TEST(Chop, MostRecentCtdBySignalTurnWinsOverHandPosition) {
  Game g = make_game();
  deal_alice(g);
  mark_ctd(g, /*order=*/3, /*signal_turn=*/5);
  mark_ctd(g, /*order=*/1, /*signal_turn=*/9);
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 1)
      << "the later-signalled CTD is the chop even though it sits further right";
}

TEST(Chop, HandOrderBreaksSignalTurnTies) {
  Game g = make_game();
  deal_alice(g);
  mark_ctd(g, /*order=*/3, /*signal_turn=*/5);
  mark_ctd(g, /*order=*/1, /*signal_turn=*/5);
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 3) << "equal signals fall back to the newer slot";
}

TEST(Chop, MissingSignalTurnLosesToASignalledCtd) {
  Game g = make_game();
  deal_alice(g);
  // Order 3 is nearer slot 1 but carries no signal_turn at all.
  g.with_meta(3, [](ConvData& m) { m.status = CardStatus::CALLED_TO_DISCARD; });
  mark_ctd(g, /*order=*/1, /*signal_turn=*/2);
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 1);
}

TEST(Chop, NonNoneStatusIsSkipped) {
  Game g = make_game();
  deal_alice(g);
  g.with_meta(4, [](ConvData& m) { m.status = CardStatus::CALLED_TO_PLAY; });
  auto chop = g.chop(0);
  ASSERT_TRUE(chop.has_value());
  EXPECT_EQ(*chop, 3) << "a CTP'd slot 1 is not the chop";
}

TEST(Chop, LockedHandHasNoChop) {
  Game g = make_game();
  deal_alice(g);
  for (int order = 0; order < 5; ++order) {
    g.with_meta(order, [](ConvData& m) { m.status = CardStatus::CHOP_MOVED; });
  }
  EXPECT_FALSE(g.chop(0).has_value());
}
