// A stable play clue whose focus the RECEIVER can see is trash is not a play.
//
// The empathy layer cannot notice this. `Game`'s per-player views are copied
// from `common` (basics/game.cpp) and re-elim'd, and `card_elim` accounts for a
// copy only when a CLUE pins it -- never when somebody merely looks at it. So a
// duplicate sitting UNCLUED in another hand is invisible to every Player object,
// and `common` still admits it.
//
// Private sight is `state.deck[o].id()`, nullopt for one's own cards, so this
// reading is deliberately PER-SEAT (CONVENTION.md §1g): the receiver and the
// giver both see the duplicate and read no call, while the player holding it
// cannot and still reads one. The seat that ACTS has the right reading, so no
// strike results.
//
// Replay 1967478 T42: blue on 4, so b5 was the only useful blue and will-bot67
// held it unclued. will-bot69's two clued blues were both trash, yet the
// leftmost was stamped CALLED_TO_PLAY and narrowed to {b5} -- and T42 played it.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;
using hanabi::reactor0::provably_trash;
using hanabi::reactor0::sight_narrowed;

namespace {

// Blue is on 4, so b5 is the only useful blue. Alice is the giver; Bob is the
// receiver of the blue clue. `cathy_slot1` decides whether the last b5 is
// visible somewhere Bob can see it.
SetupOptions blue_opts(std::string cathy_slot1) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  // Yellow and blue on 4; the rest untouched. b5 is therefore the only useful
  // blue, and Bob's two blues are both at or below the stack.
  opts.play_stacks = {0, 4, 0, 4, 0};
  opts.hands = {
      {"g4", "p4", "g3", "p3", "g2"},                    // Alice (giver, us)
      {"b1", "b2", "r4", "r5", "p5"},                    // Bob -- two trash blues
      {std::move(cathy_slot1), "r1", "r2", "r3", "y1"},  // Cathy
  };
  use_reactor0(opts);
  return opts;
}

int chuck_count(const Game& g, TestPlayer who) {
  auto lists = hanabi::reactor0::action_lists(g, static_cast<int>(who));
  return static_cast<int>(lists.chuck.size());
}

}  // namespace

// --- the helper -----------------------------------------------------------

TEST(Reactor0ProvablyTrashFocus, SightDropsAnIdentityWhoseCopiesAreAllVisible) {
  // Cathy holds the only b5, so Bob's blues cannot be it.
  Game g = setup(blue_opts("b5"));
  const int bob_blue = order_at(g, TestPlayer::BOB, 1);
  const IdentitySet seen = sight_narrowed(g, bob_blue);
  EXPECT_FALSE(seen.contains(Identity{3, 5}))
      << "the last b5 is visible in Cathy's hand, so Bob's card is not it";

  // With no b5 in sight it stays open.
  Game g2 = setup(blue_opts("p1"));
  EXPECT_TRUE(sight_narrowed(g2, order_at(g2, TestPlayer::BOB, 1))
                  .contains(Identity{3, 5}))
      << "a copy is unaccounted for, so it is still possible";
}

// --- the reading ----------------------------------------------------------

TEST(Reactor0ProvablyTrashFocus, StableColourDeclinesAProvablyTrashFocus) {
  Game g = setup(blue_opts("b5"));
  g = take_turn(std::move(g), "Alice clues blue to Bob");

  EXPECT_EQ(last_clue_interp(g), ClueInterp::STALL)
      << "every blue Bob could hold is trash once the b5 he can see is placed";
  EXPECT_FALSE(any_status(g, TestPlayer::BOB, CardStatus::CALLED_TO_PLAY))
      << "so nothing is called to play";
}

// The mirror: with the b5 in BOB's own hand instead, the very same blue clue is
// a direct play. Bob's slot 1 changes too, and deliberately -- leaving him a
// trash b1 there makes the clue a MISTAKE for an unrelated reason (the giver can
// see the card it would call is not playable), which would confound what this
// pair is meant to isolate.
TEST(Reactor0ProvablyTrashFocus, TheClueIsAPlayWhenTheFocusReallyCouldBeTheFive) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 4, 0, 4, 0};
  opts.hands = {
      {"g4", "p4", "g3", "p3", "g2"},   // Alice (giver, us)
      {"b5", "b2", "r4", "r5", "p5"},   // Bob -- holds the b5 himself
      {"p1", "r1", "r2", "r3", "y1"},   // Cathy -- no b5 in sight
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int focus = order_at(g, TestPlayer::BOB, 1);
  g = take_turn(std::move(g), "Alice clues blue to Bob");

  EXPECT_EQ(g.meta[focus].status, CardStatus::CALLED_TO_PLAY)
      << "no b5 is visible elsewhere, so the focus really could be it; interp="
      << (last_clue_interp(g) ? static_cast<int>(*last_clue_interp(g)) : -1);
}

// --- the chuck list -------------------------------------------------------

// With no call on them the blues are provably trash to their holder and belong
// on the chuck list -- which `is_chuckable` can only see once it is narrowed by
// sight, since empathy alone still admits b5. Alice holds them here because
// that is the seat `is_chuckable` narrows for.
TEST(Reactor0ProvablyTrashFocus, TrashBluesInOurOwnHandAreChuckable) {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::ALICE;
  opts.play_stacks = {0, 4, 0, 4, 0};
  opts.hands = {
      // Ours, face down: the harness would otherwise hand us our own identities
      // and `sight_narrowed` is about what we can see of OTHERS.
      {"xx", "xx", "xx", "xx", "xx"},
      {"b5", "r1", "r2", "r3", "y1"},     // Bob holds the last b5, in our sight
      {"g4", "p4", "g3", "p3", "g2"},     // Cathy
  };
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  // Our slot 1 is clued blue, so its empathy is the five blues.
  g = pre_clue(std::move(g), TestPlayer::ALICE, 1, {"blue"});
  const int mine = order_at(g, TestPlayer::ALICE, 1);
  EXPECT_FALSE(sight_narrowed(g, mine).contains(Identity{3, 5}))
      << "we can see the b5 in Bob's hand";
  EXPECT_TRUE(provably_trash(g, mine))
      << "every blue we could still be holding is at or below a stack of 4";

  auto lists = hanabi::reactor0::action_lists(g, static_cast<int>(TestPlayer::ALICE));
  bool found = false;
  for (int o : lists.chuck) {
    if (o == mine) found = true;
  }
  EXPECT_TRUE(found) << "a card we can prove is worthless belongs on the chuck list";
}
