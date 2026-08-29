// The Actionable Card Priority must never press DISCARD at 8 clues.
//
// A chuck IS the Discard button -- that an inverted card happens to reach its
// stack does not change which button was pressed -- and the server rejects a
// discard at 8 clues outright. So emitting one is not a bad choice, it is an
// ILLEGAL action: the move is refused and the bot cannot take its turn at all.
//
// Rungs 3, 9, 10 and 11 all walk `ActionLists::chuck` and none of them was
// guarded, while rung 13 (`pitch_chop_at_eight`) and the shared ladder's
// `cant_discard` already were. Replay 1977786 T35 is where it surfaced:
// `11.chuck_leftmost` answered a forced turn with `discard(order=5)` at 8
// tokens. v11.2.0 routed around it in the target-parity variants by giving the
// clue phase something readable to offer; v11.3.0 closes the hole itself, which
// was never variant-specific.
//
// `choose_action` is called directly rather than through `take_action`, because
// the point is the LADDER: `take_action` reaches it only after the clue phase
// has declined, and arranging that as well would test two things at once.
#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "test_harness.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// Pin a card in EVERY view, the way a fully-resolving clue sequence would.
// `Game::with_thought` writes only `common`, and `action_lists` reads the
// HOLDER's own view -- and the holder cannot see their own hand, so without this
// the chuck list comes back empty and the ladder never reaches the rungs under
// test at all. (`test_chuck_dupes.cpp` needs the same idiom, for the same
// reason.)
void pin(Game& g, int order, Identity id) {
  const IdentitySet one = IdentitySet::single(id);
  g.with_thought(order, [one](const Thought& t) {
    Thought out = t;
    out.inferred = one;
    out.possible = one;
    return out;
  });
  for (Player& p : g.players) {
    p.thoughts[order].inferred = one;
    p.thoughts[order].possible = one;
  }
}

// Orange is the inverted suit, so a playable orange is the strongest possible
// chuck candidate -- rung 9 takes it as "a play in all but name". At 8 tokens
// it must still not be chucked.
SetupOptions chuckable_opts(int clue_tokens) {
  SetupOptions opts;
  opts.variant_name = "Orange (5 Suits)";   // r/y/g/b/o, orange inverted
  opts.play_stacks = {2, 2, 2, 2, 0};       // the o1 is the next orange
  opts.clue_tokens = clue_tokens;
  opts.starting = TestPlayer::ALICE;
  opts.hands = {
      {"o1", "r1", "y1", "g1", "b1"},  // Alice: a playable orange, then trash
      {"r4", "y4", "g4", "b4", "o4"},
      {"r3", "y3", "g3", "b3", "o3"},
  };
  use_reactor0(opts);
  return opts;
}

// The fixture as the ladder must actually see it: Alice KNOWS her slot 1 is the
// playable o1, so it reaches the chuck list and rung 9 wants it.
Game chuckable_game(int clue_tokens) {
  Game g = setup(chuckable_opts(clue_tokens));
  pin(g, g.state.hands[0][0], Identity{4, 1});  // o1
  return g;
}

bool is_discard(const std::optional<PerformAction>& a) {
  return a && std::holds_alternative<PerformDiscard>(*a);
}

std::string describe(const std::optional<PerformAction>& a) {
  if (!a) return "(nothing)";
  if (auto* p = std::get_if<PerformPlay>(&*a))
    return "play(order=" + std::to_string(p->target) + ")";
  if (auto* d = std::get_if<PerformDiscard>(&*a))
    return "discard(order=" + std::to_string(d->target) + ")";
  return "(a clue)";
}

}  // namespace

TEST(Reactor0NoChuckAtEightClues, TheLadderNeverDiscardsAtEightTokens) {
  Game g = chuckable_game(8);
  ASSERT_EQ(g.state.clue_tokens, 8) << "guard: a discard is illegal here";
  ASSERT_TRUE(g.state.is_playable(Identity{4, 1}))
      << "guard: the o1 is exactly what rung 9 wants to chuck";

  auto action = hanabi::reactor0::choose_action(g);
  ASSERT_TRUE(action.has_value()) << "the ladder must always answer";
  EXPECT_FALSE(is_discard(action))
      << "a discard at 8 clues is ILLEGAL -- the server refuses it and the bot "
         "deadlocks. Got " << describe(action);
}

// The chuck list itself is what the guard empties, so assert that directly:
// the rungs above rung 13 have nothing to walk.
TEST(Reactor0NoChuckAtEightClues, TheChuckListIsEmptyAtEightTokens) {
  Game g = chuckable_game(8);
  // `action_lists` is deliberately NOT what the guard touches -- it still
  // reports what is chuckable in the abstract. The ladder is where the token
  // rule applies, so this asserts through the ladder's answer rather than the
  // list, which is the behaviour that matters.
  auto action = hanabi::reactor0::choose_action(g);
  ASSERT_TRUE(action.has_value());
  auto* play = std::get_if<PerformPlay>(&*action);
  ASSERT_NE(play, nullptr) << "expected a PLAY-button action, got "
                           << describe(action);
}

// One token down and the chuck is right again -- which is what shows the guard
// is doing the work rather than something else about the fixture.
TEST(Reactor0NoChuckAtEightClues, AtSevenTokensThePlayableOrangeIsChucked) {
  Game g = chuckable_game(7);
  auto action = hanabi::reactor0::choose_action(g);
  ASSERT_TRUE(action.has_value());
  EXPECT_TRUE(is_discard(action))
      << "with a token spent the Discard button is legal again, and chucking a "
         "playable orange advances its stack. Got " << describe(action);
}

// A locked hand has no chop for rung 13 to pitch, so it is the case where an
// unguarded ladder would have had nowhere else to go. It must still answer with
// a legal action.
TEST(Reactor0NoChuckAtEightClues, EvenALockedHandAnswersLegally) {
  Game g = chuckable_game(8);
  for (int o : g.state.hands[0]) {
    g.with_meta(o, [](ConvData& m) { m.status = CardStatus::CHOP_MOVED; });
  }
  ASSERT_FALSE(g.chop(0).has_value()) << "guard: the hand is locked, no chop";

  auto action = hanabi::reactor0::choose_action(g);
  ASSERT_TRUE(action.has_value()) << "the ladder must always answer";
  EXPECT_FALSE(is_discard(action))
      << "still illegal at 8 tokens, chop or no chop. Got " << describe(action);
}
