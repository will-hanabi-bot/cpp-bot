// Under reactor0 a strike resets nothing.
//
// `Game::interpret_discard`'s bombed branch (basics/decide.cpp:249) wipes every
// card in every hand that is not CALLED_TO_PLAY: `inferred` goes back to
// `possible`, `info_lock` and `old_inferred` are dropped, the whole ConvData is
// cleared, and `waiting` is emptied. That is right for reactor, where a strike
// means a finesse or dupe chain was misread — and wrong for reactor0, where a
// strike is a NORMAL outcome of the convention.
//
// "The stamp is the instruction": a card stamped CTD is chucked, and on an
// inverted suit a chuck that turns out unplayable simply bombs. Nothing was
// miscommunicated, so the promises other clues made about OTHER cards still
// stand. Replay 1966687: will-bot67 chucked an orange 4 on T14 and the strike
// wiped will-bot69's slot 1 from {o2} back to {o1,o2,o3}, taking its CTD stamp
// with it.
#include <gtest/gtest.h>

#include <string>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Cathy bombs her slot 1. Orange (suit 2) is inverted here, so a DISCARD of an
// unplayable orange is the failed play the engine strikes on — which is
// exactly how the strike arose in 1966687.
Game bomb_cathys_slot_one(Game g) {
  const int cathy = static_cast<int>(TestPlayer::CATHY);
  const int order = g.state.hands[cathy][0];
  const int turn_before = g.state.turn_count;
  g.catchup = true;
  g.handle_action(DiscardAction{cathy, order, 2, 4, /*failed=*/true});
  g.handle_action(TurnAction{turn_before, g.state.next_player_index(cathy)});
  g.catchup = false;
  return g;
}

SetupOptions strike_opts() {
  SetupOptions opts;
  opts.variant_name = "White-Fives & Orange (3 Suits)";
  opts.starting = TestPlayer::CATHY;
  opts.play_stacks = {4, 1, 1};
  opts.hands = {
      {"r3", "b3", "r2", "b2", "r1"},
      {"o2", "o3", "r5", "b5", "o5"},
      {"o4", "r4", "b4", "o4", "b4"},
  };
  use_reactor0(opts);
  return opts;
}

}  // namespace

TEST(Reactor0StrikeNoReset, BombLeavesAnotherPlayersInferredSetIntact) {
  Game g = setup(strike_opts());
  const int bob_s1 = order_at(g, TestPlayer::BOB, 1);
  const IdentitySet promised = IdentitySet::single(Identity{2, 2});  // {o2}
  g.with_thought(bob_s1, [promised](const Thought& t) {
    Thought out = t;
    out.inferred = promised;
    return out;
  });
  g.with_meta(bob_s1, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m = m.reason(0).signal(0);
  });

  g = bomb_cathys_slot_one(std::move(g));

  ASSERT_EQ(g.state.strikes, 1) << "fixture must actually take a strike";
  EXPECT_EQ(g.common.thoughts[bob_s1].inferred, promised)
      << "a bomb elsewhere is not evidence against what a clue promised about "
         "this card (replay 1966687)";
  EXPECT_EQ(g.meta[bob_s1].status, CardStatus::CALLED_TO_DISCARD)
      << "the CTD stamp survives the strike too — the stamp is the instruction";
}

// The companion: reactor still resets, so the existing behaviour that
// tests/test_basics/test_strike_preserves_ctp.cpp pins is untouched. Only the
// convention flag differs between this fixture and the one above.
TEST(Reactor0StrikeNoReset, ReactorStillResetsOnABomb) {
  SetupOptions opts = strike_opts();
  opts.init = nullptr;  // leave the convention at reactor
  Game g = setup(std::move(opts));
  const int bob_s1 = order_at(g, TestPlayer::BOB, 1);
  const IdentitySet promised = IdentitySet::single(Identity{2, 2});
  g.with_thought(bob_s1, [promised](const Thought& t) {
    Thought out = t;
    out.inferred = promised;
    return out;
  });
  g.with_meta(bob_s1, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m = m.reason(0).signal(0);
  });

  g = bomb_cathys_slot_one(std::move(g));

  ASSERT_EQ(g.state.strikes, 1);
  EXPECT_NE(g.meta[bob_s1].status, CardStatus::CALLED_TO_DISCARD)
      << "reactor's reset is deliberate and must stay — a strike there really "
         "does mean a chain was misread";
}
