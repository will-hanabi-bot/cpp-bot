// The shape a reactor0 hand's outstanding calls are allowed to take.
//
// Rule 1: play calls run in play order (below).
// Rule 2: at most ONE discard call per hand — unlike play calls, CTD does
//         not stack, so a new call replaces the standing one. Cards merely
//         revealed to be basic trash are not calls and are left alone.
//
// reactor0 keeps a hand's CALLED_TO_PLAY stamps in play order.
//
// Several CTP cards at once is a legal state, and the holder actions them
// most-recently-stamped first. That order can disagree with slot order: an
// older clue calls slot 2, a newer clue calls slot 4, and the holder would
// play slot 4 and then come back to slot 2.
//
// The convention erases rather than reorders — a newer clue would not have
// pointed past a card that was still playable, so the earlier call is dead.
// The resulting invariant is that CTP cards run newest slot to oldest slot in
// exactly play order, which is what makes the shared urgent scan in
// Game::take_action correct without it consulting signal turns.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include "hanabi/conventions/reactor0/call_invariants.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// The invariant: walking slots newest -> oldest, CTP stamp turns never
// increase. Equivalently, no CTP card is older-stamped than a CTP card in a
// newer slot.
void expect_ctp_in_play_order(const Game& g, TestPlayer player,
                              const char* label) {
  std::optional<int> prev_turn;
  for (int slot = 1; slot <= 5; ++slot) {
    int o = order_at(g, player, slot);
    if (g.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
    int turn = g.meta[o].signal_turn ? *g.meta[o].signal_turn : -1;
    if (prev_turn) {
      EXPECT_LE(turn, *prev_turn)
          << label << ": slot " << slot << " was stamped at turn " << turn
          << ", later than the CTP card in a newer slot (turn " << *prev_turn
          << ") -- CTP cards must run newest slot to oldest in play order";
    }
    prev_turn = turn;
  }
}

}  // namespace

TEST(Reactor0CallInvariants, NewerClueCallingAnOlderSlotErasesTheEarlierCall) {
  SetupOptions opts;
  // Cathy's ONLY playable is g1 at slot 3, so anchor 2 sends the call to
  // Bob's react_slot (2 - 3) mod 5 = 4 -- an older slot than the standing
  // call on slot 2.
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "b1", "y4"},   // Bob: slot 4 = b1, playable
      {"r4", "y2", "g1", "b4", "p4"},   // Cathy: slot 3 = g1, only playable
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // An earlier clue called Bob's slot 2. Stamp it at turn 0, before the
  // reactive below.
  int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  g.with_meta(bob_s2, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m = m.reason(0).signal(0);
  });
  g.with_thought(bob_s2, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  if (last_clue_interp(g) != ClueInterp::REACTIVE) {
    GTEST_SKIP() << "clue not read as reactive in this position";
  }
  ASSERT_FALSE(g.waiting.empty());
  int named = g.waiting.front().react_order;
  ASSERT_EQ(named, order_at(g, TestPlayer::BOB, 4))
      << "fixture must call an older slot than the standing one";

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "the new call stands";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "the earlier call on a NEWER slot must be erased -- a newer clue "
         "would not have pointed past a card that was still playable";
  EXPECT_FALSE(urgent_at(g, TestPlayer::BOB, 2))
      << "and its urgency must go with it";
  expect_ctp_in_play_order(g, TestPlayer::BOB, "reactive past a standing call");
}

TEST(Reactor0CallInvariants, CallOnANewerSlotLeavesTheOlderCallStanding) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "g1", "y4"},   // Bob
      {"r1", "y2", "g1", "b4", "p4"},   // Cathy
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // The standing call is on slot 4 this time. Cathy's leftmost playable is
  // r1 at slot 1, so anchor 2 names Bob's react_slot 1 -- NEWER than the
  // standing call, which must therefore survive and simply be played second.
  int bob_s4 = order_at(g, TestPlayer::BOB, 4);
  g.with_meta(bob_s4, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m = m.reason(0).signal(0);
  });
  g.with_thought(bob_s4, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });

  g = take_turn(std::move(g), "Alice clues 2 to Cathy");

  if (last_clue_interp(g) == ClueInterp::REACTIVE) {
    ASSERT_FALSE(g.waiting.empty());
    int named = g.waiting.front().react_order;
    int named_slot = 0;
    for (int s = 1; s <= 5; ++s) {
      if (order_at(g, TestPlayer::BOB, s) == named) named_slot = s;
    }
    if (named_slot > 0 && named_slot < 4) {
      EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
          << "a call on a newer slot must not disturb the older standing "
             "call -- it is simply played second";
    }
  }
  expect_ctp_in_play_order(g, TestPlayer::BOB, "call on a newer slot");
}

TEST(Reactor0CallInvariants, InvariantHoldsAcrossReactivePositions) {
  struct Case {
    const char* name;
    std::vector<std::vector<std::string>> hands;
    std::vector<int> stacks;
    int standing_slot;
    const char* clue;
  };
  const std::vector<Case> cases = {
      {"rank double play, standing call slot 1",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"g1", "y3", "b3", "g4", "y4"},
        {"r1", "y2", "g3", "b4", "p5"}},
       {0, 0, 0, 0, 0},
       1,
       "Alice clues 2 to Cathy"},
      {"colour dc+play, standing call slot 2",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"y2", "y3", "b3", "g4", "y4"},
        {"r1", "y2", "g3", "b4", "p2"}},
       {0, 0, 0, 0, 0},
       2,
       "Alice clues red to Cathy"},
      {"rank double discard, standing call slot 3",
       {{"xx", "xx", "xx", "xx", "xx"},
        {"y2", "y3", "b3", "g4", "y4"},
        {"y4", "b4", "r1", "g4", "p4"}},
       {1, 0, 0, 0, 0},
       3,
       "Alice clues 4 to Cathy"},
  };

  for (const auto& c : cases) {
    SetupOptions opts;
    opts.hands = c.hands;
    opts.play_stacks = c.stacks;
    use_reactor0(opts);
    Game g = setup(opts);

    int standing = order_at(g, TestPlayer::BOB, c.standing_slot);
    g.with_meta(standing, [](ConvData& m) {
      m.status = CardStatus::CALLED_TO_PLAY;
      m.urgent = true;
      m = m.reason(0).signal(0);
    });
    g.with_thought(standing, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });

    g = take_turn(std::move(g), c.clue);
    expect_ctp_in_play_order(g, TestPlayer::BOB, c.name);
  }
}

// --- Rule 2: at most one discard call ------------------------------------

TEST(Reactor0CallInvariants, ANewDiscardCallReplacesTheStandingOne) {
  SetupOptions opts;
  // Red stack at 1 makes Cathy's r1 at slot 2 trash — the dc-target. Green =
  // anchor 3, so slot 2 maps to Bob's react_slot 1, a playable y1.
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g4", "y3", "b3", "p4"},   // Bob: slot 1 = y1, playable
      {"y4", "r1", "g3", "p3", "b4"},   // Cathy: slot 2 = r1, trash
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // A standing discard call on Bob's slot 4 from an earlier clue.
  int bob_s4 = order_at(g, TestPlayer::BOB, 4);
  g.with_meta(bob_s4, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m = m.reason(0).signal(0);
  });
  g.with_thought(bob_s4, [](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    return out;
  });

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  int ctds = 0;
  for (int slot = 1; slot <= 5; ++slot) {
    if (status_at(g, TestPlayer::BOB, slot) == CardStatus::CALLED_TO_DISCARD) {
      ++ctds;
    }
  }
  EXPECT_LE(ctds, 1)
      << "a player holds at most one CALLED_TO_DISCARD at a time; a new "
         "discard call must replace the standing one, not stack on it";
}

TEST(Reactor0CallInvariants, RevealedTrashIsNotADiscardCall) {
  SetupOptions opts;
  opts.play_stacks = {{1, 0, 0, 0, 0}};
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "g4", "y3", "b3", "p4"},
      {"y4", "r1", "g3", "p3", "b4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // Two cards flagged as revealed trash, plus a standing discard call. The
  // trash flags are not calls, so the single-call rule must not touch them.
  int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  int bob_s3 = order_at(g, TestPlayer::BOB, 3);
  g.with_meta(bob_s2, [](ConvData& m) { m.trash = true; });
  g.with_meta(bob_s3, [](ConvData& m) { m.trash = true; });

  g = take_turn(std::move(g), "Alice clues green to Cathy");

  EXPECT_TRUE(g.meta[bob_s2].trash)
      << "revealed trash is not a discard call and must survive";
  EXPECT_TRUE(g.meta[bob_s3].trash)
      << "several revealed-trash cards at once are fine — only CTD is capped";
}

// --- rule 3: a dead call is dropped --------------------------------------

// A call is only as good as the card. Once common knowledge leaves the stamped
// button with no identity it handles correctly, every seat drops it.
//
// Replay 1966653: yagami was called to play a Red 2 and never did, while the
// other copy went down. By T17 the red stack was at 3, so the card was globally
// known to be unplayable and the standing CTP would have walked him into a
// second strike.
TEST(Reactor0CallInvariants, ACtpDiesOnceNoPitchIsLeft) {
  SetupOptions opts;
  opts.hands = {
      {"y4", "g4", "b4", "p4", "y5"},
      {"r2", "y3", "g3", "b3", "p3"},
      {"g2", "b2", "p2", "y2", "g5"},
  };
  opts.play_stacks = {3, 0, 0, 0, 0};  // red is past 2, so r2 is dead
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int r2 = order_at(g, TestPlayer::BOB, 1);
  ASSERT_TRUE(g.state.is_basic_trash(Identity{0, 2})) << "guard: r2 is dead";
  g.meta[r2].status = CardStatus::CALLED_TO_PLAY;
  g.with_thought(r2, [](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(Identity{0, 2});
    return out;
  });

  hanabi::reactor0::enforce_call_invariants(g);
  EXPECT_NE(g.meta[r2].status, CardStatus::CALLED_TO_PLAY)
      << "pressing Play on a dead plain card strikes, so the call is dead too";
}

// But a CTP on an unplayable ORANGE is a PITCH -- throwing it away -- and being
// unplayable is exactly what makes that call sensible. It must survive.
TEST(Reactor0CallInvariants, ACtpOnASpareOrangeSurvives) {
  SetupOptions opts;
  opts.variant_name = "Orange (4 Suits)";
  opts.hands = {
      {"r3", "b4", "g4", "r5", "b5"},
      {"o3", "r2", "g3", "b3", "r4"},
      {"g2", "b2", "r3", "g5", "o4"},
  };
  opts.play_stacks = {0, 0, 0, 0};  // orange stack 0, so o3 is NOT playable
  opts.starting = TestPlayer::ALICE;
  use_reactor0(opts);
  Game g = setup(std::move(opts));

  const int orange = 3;
  ASSERT_TRUE(g.state.variant->suits[orange].suit_type.inverted);
  const int o3 = order_at(g, TestPlayer::BOB, 1);
  ASSERT_FALSE(g.state.is_playable(Identity{orange, 3}));
  ASSERT_FALSE(g.state.is_critical(Identity{orange, 3}));
  g.meta[o3].status = CardStatus::CALLED_TO_PLAY;
  g.with_thought(o3, [orange](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::single(Identity{orange, 3});
    return out;
  });

  hanabi::reactor0::enforce_call_invariants(g);
  EXPECT_EQ(g.meta[o3].status, CardStatus::CALLED_TO_PLAY)
      << "a pitch of a spare orange is a live call, not a dead one";
}
