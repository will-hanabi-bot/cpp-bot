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
//
// That erasure is ASYMMETRIC in the kind of call doing it (v10.12.0):
//
//   * a RECEIVER call (non-urgent) retires both kinds to its left -- landing to
//     the right of a standing reacter call means leftmost targeting has left
//     that reacter card unactionable;
//   * a REACTER call (urgent) retires only other REACTER calls. It is actioned
//     by the urgent scan on the very next turn and never joins the receiver
//     deque, so it has no standing to retire a receiver call.
//
// Replay 1974512 T8 is what the symmetric version cost: a reacter stamp on an
// older slot erased a standing receiver-CTP on a playable p1, which was then
// never recovered.
#include <gtest/gtest.h>

#include "hanabi/basics/game.h"
#include <algorithm>

#include "hanabi/conventions/reactor0/call_invariants.h"
#include "hanabi/conventions/reactor0/calls.h"
#include "test_reactor0/test_reactor0_helpers.h"

using namespace hanabi;
using namespace hanabi::test;
using namespace hanabi::test::reactor0;

namespace {

// The invariant: walking slots newest -> oldest, CTP stamp turns never
// increase. Equivalently, no CTP card is older-stamped than a CTP card in a
// newer slot.
//
// Asked WITHIN each kind of call, not across them (v10.12.0). Rule 1 is
// asymmetric: a receiver call retires both kinds to its left, but a reacter
// call retires only other reacter calls, so a receiver-CTP may legitimately sit
// in a newer slot than a later-stamped reacter-CTP. That is replay 1974512's
// shape and it is now legal.
//
// What this therefore does NOT cover is the one surviving cross-kind
// constraint -- that no RECEIVER call is stamped later than a reacter call in an
// older slot. The tests that care about it assert the two stamps directly.
void expect_ctp_in_play_order(const Game& g, TestPlayer player,
                              const char* label) {
  for (int kind = 0; kind < 2; ++kind) {
    const bool want_urgent = kind == 0;
    std::optional<int> prev_turn;
    for (int slot = 1; slot <= 5; ++slot) {
      int o = order_at(g, player, slot);
      if (g.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      if (g.meta[o].urgent != want_urgent) continue;
      int turn = g.meta[o].signal_turn ? *g.meta[o].signal_turn : -1;
      if (prev_turn) {
        EXPECT_LE(turn, *prev_turn)
            << label << ": " << (want_urgent ? "reacter" : "receiver")
            << " CTP on slot " << slot << " was stamped at turn " << turn
            << ", later than the " << (want_urgent ? "reacter" : "receiver")
            << " CTP in a newer slot (turn " << *prev_turn
            << ") -- calls of one kind must run newest slot to oldest in play "
               "order";
      }
      prev_turn = turn;
    }
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

// The same fixture, with the standing call made a RECEIVER call. A reacter
// stamp on an older slot must now leave it alone -- replay 1974512 T8, where
// erasing it cost a playable p1 the bot never picked back up.
TEST(Reactor0CallInvariants, AReacterCallDoesNotEraseAReceiverCall) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "b1", "y4"},   // Bob: slot 4 = b1, playable
      {"r4", "y2", "g1", "b4", "p4"},   // Cathy: slot 3 = g1, only playable
  };
  use_reactor0(opts);
  Game g = setup(opts);

  // The standing call on Bob's slot 2 is a RECEIVER call: NOT urgent. That one
  // bit is the whole difference from the test above.
  int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  g.with_meta(bob_s2, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = false;
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
  ASSERT_EQ(g.waiting.front().react_order, order_at(g, TestPlayer::BOB, 4))
      << "fixture must call an older slot than the standing one";

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "the new reacter call stands";
  EXPECT_TRUE(urgent_at(g, TestPlayer::BOB, 4)) << "and it is urgent";
  EXPECT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "the standing RECEIVER call must survive -- a reacter call is "
         "actioned by the urgent scan and never joins the receiver deque, so "
         "it has no standing to retire one";
  EXPECT_FALSE(urgent_at(g, TestPlayer::BOB, 2))
      << "and it is still a receiver call";
  expect_ctp_in_play_order(g, TestPlayer::BOB, "reacter past a receiver call");
}

// The other direction still erases: a RECEIVER call landing to the right of a
// standing reacter call means leftmost targeting has left the reacter card
// unactionable, so the urgent call goes. Asserted against the invariant
// directly rather than through a clue, because a receiver call is stamped when
// the reacter acts, not at clue time.
TEST(Reactor0CallInvariants, AReceiverCallStillErasesAReacterCallToItsLeft) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},
      {"y1", "y3", "b3", "b1", "y4"},
      {"r4", "y2", "g1", "b4", "p4"},
  };
  use_reactor0(opts);
  Game g = setup(opts);

  const int bob_s2 = order_at(g, TestPlayer::BOB, 2);  // reacter, newer slot
  const int bob_s4 = order_at(g, TestPlayer::BOB, 4);  // receiver, older slot
  g.with_meta(bob_s2, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = true;
    m = m.reason(0).signal(0);
  });
  g.with_meta(bob_s4, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = false;
    m = m.reason(3).signal(3);   // stamped LATER, on the older slot
  });

  hanabi::reactor0::enforce_call_invariants(g);

  EXPECT_EQ(status_at(g, TestPlayer::BOB, 4), CardStatus::CALLED_TO_PLAY)
      << "the newer receiver call stands";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "the standing REACTER call on a newer slot is retired -- leftmost "
         "targeting has left it unactionable";
  EXPECT_FALSE(urgent_at(g, TestPlayer::BOB, 2))
      << "and its urgency goes with it";
}

// Rule 3 has to notice a stack somebody ELSE advanced.
//
// The dead-call rules turn on the stacks, not on the stamps, so a call can die
// without anybody clueing. Until v10.12.0 the play and discard hooks only
// reached `enforce_call_invariants` from inside their `waiting` block, so an
// ordinary play never re-checked standing calls: replay 1971981, where a
// receiver call narrowed to {r1, m1} outlived both being played and the holder
// blind-played it into a strike.
TEST(Reactor0CallInvariants, AnOrdinaryPlayRetiresACallItJustKilled) {
  SetupOptions opts;
  opts.hands = {
      {"xx", "xx", "xx", "xx", "xx"},   // Alice (us)
      {"y1", "y3", "b3", "b4", "y4"},   // Bob: slot 2 carries the call
      {"r1", "y2", "g4", "b2", "p4"},   // Cathy: slot 1 is the r1
  };
  opts.starting = TestPlayer::CATHY;    // she takes the ordinary play below
  use_reactor0(opts);
  Game g = setup(opts);

  const int bob_s2 = order_at(g, TestPlayer::BOB, 2);
  const Identity r1 = g.state.expand_short("r1");
  g.with_meta(bob_s2, [](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.urgent = false;
    m = m.reason(0).signal(0);
  });
  g.with_thought(bob_s2, [r1](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = IdentitySet::single(r1);
    return out;
  });
  ASSERT_EQ(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY);
  ASSERT_TRUE(g.state.is_playable(r1)) << "the call is live to begin with";
  ASSERT_TRUE(g.waiting.empty())
      << "no reaction pending -- that is the whole point";

  // Cathy plays the other r1. Nothing is clued and no reaction resolves, so
  // before v10.12.0 nothing re-examined Bob's call.
  g = take_turn(std::move(g), "Cathy plays r1 (slot 1)", "p1");

  EXPECT_FALSE(g.state.is_playable(r1)) << "red is on 1 now";
  EXPECT_NE(status_at(g, TestPlayer::BOB, 2), CardStatus::CALLED_TO_PLAY)
      << "every identity the call could still be is trash, so rule 3 retires "
         "it -- even though the turn was an ordinary play";
  expect_infs(g, std::nullopt, TestPlayer::BOB, 2, {"r1"});
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

// Rule 0 (v11.1.0): a reacter-CTP whose paired target has left the receiver's
// hand is RELEGATED to a receiver-CTP.
//
// `urgent` is the reacter/receiver discriminator `calls_of` routes on, so
// clearing it IS the relegation: the card leaves `reacter_ctp`, joins the
// `receiver_ctp` deque, and is reached by the pitch list and phase 2's rungs
// 2-8 instead of by the urgent scan.
//
// Before this, the de-urgenting lived in `decide.cpp`'s urgent scan and merely
// SKIPPED the call, leaving the flag set. `calls_of` went on filing it under
// `reacter_ctp` -- where the only thing that actions it is the scan that had
// just skipped it, `choose_action` having no rung 1 -- so the call became
// permanently unactionable. Replay 1975197 T5 discarded its chop holding one.
//
// The three tests below differ only in what happens to the paired card, or in
// which button the call carries.
namespace {

// Alice leads so Cathy acts second; Bob's slot 3 carries the call. Green is on
// 0 and Cathy holds no green, so the call's {g1} reading survives whatever she
// does -- rule 3 must not be what clears it.
SetupOptions relegation_opts() {
  SetupOptions opts;
  opts.variant_name = "No Variant";
  opts.starting = TestPlayer::CATHY;
  opts.play_stacks = {0, 0, 0, 0, 0};
  opts.clue_tokens = 7;  // the control has Cathy discard, illegal at the cap
  opts.hands = {
      {"y4", "y3", "p4", "p3", "y2"},   // Alice
      {"b4", "b3", "g1", "b2", "y5"},   // Bob -- slot 3 is the called card
      {"r1", "r3", "p2", "r4", "r5"},   // Cathy -- slot 1 is the paired target
  };
  use_reactor0(opts);
  return opts;
}

// Seed a reacter call on Bob's slot 3, paired with Cathy's slot 1.
//
// The reading has to suit the BUTTON, or rules 3 and 4 erase the call before
// rule 0 is ever the question: a CTP needs a reading its Play button could
// advance ({g1}, green on 0), a CTD one its Discard button could spare ({y4},
// two copies and not playable). Getting that wrong is what made the first draft
// of the CTD test look like a relegation when it was an erasure.
Game seed_reacter_call(Game g, CardStatus button) {
  const int called = order_at(g, TestPlayer::BOB, 3);
  const int paired = order_at(g, TestPlayer::CATHY, 1);
  const Identity reading = button == CardStatus::CALLED_TO_PLAY
                               ? Identity{2, 1}   // g1 -- playable
                               : Identity{1, 4};  // y4 -- sparable
  g.with_thought(called, [reading](const Thought& t) {
    Thought out = t;
    out.inferred = IdentitySet::from_iter({reading});
    return out;
  });
  g.with_meta(called, [&](ConvData& m) {
    m.status = button;
    m.urgent = true;
    m.react_target_order = paired;
    m = m.reason(0).signal(0);
  });
  return g;
}

}  // namespace

TEST(Reactor0CallInvariants, ASpentReacterPlayCallRelegatesToTheReceiverDeque) {
  Game g = seed_reacter_call(setup(relegation_opts()),
                             CardStatus::CALLED_TO_PLAY);
  const int called = order_at(g, TestPlayer::BOB, 3);
  ASSERT_TRUE(g.meta[called].urgent) << "guard: it starts as a reacter call";

  // Cathy plays the paired card, so it leaves her hand.
  g = take_turn(std::move(g), "Cathy plays r1 (slot 1)", "p5");

  EXPECT_EQ(g.meta[called].status, CardStatus::CALLED_TO_PLAY)
      << "the call stands -- this is a relegation, not an erasure";
  EXPECT_TRUE(g.common.thoughts[called].inferred.contains(Identity{2, 1}))
      << "and so does the inference it installed";
  EXPECT_FALSE(g.meta[called].urgent)
      << "but it is no longer urgent: nobody is decoding against it";

  const auto calls = hanabi::reactor0::calls_of(g, static_cast<int>(TestPlayer::BOB));
  EXPECT_NE(calls.reacter_ctp, called) << "it has left the reacter slot";
  EXPECT_NE(std::find(calls.receiver_ctp.begin(), calls.receiver_ctp.end(), called),
            calls.receiver_ctp.end())
      << "and joined the receiver deque, which is what the pitch list is built "
         "from -- the whole point of relegating rather than skipping";
  EXPECT_FALSE(calls.has_reaction())
      << "so Alice is no longer occupied by a pending reaction";
}

// The control, and the discriminator: the paired card is still held, so the
// receiver is still decoding and the call keeps its urgency.
TEST(Reactor0CallInvariants, AReacterCallKeepsUrgencyWhileItsTargetIsHeld) {
  Game g = seed_reacter_call(setup(relegation_opts()),
                             CardStatus::CALLED_TO_PLAY);
  const int called = order_at(g, TestPlayer::BOB, 3);
  const int paired = g.meta[called].react_target_order;

  // Cathy acts on a DIFFERENT card, so the paired one stays put.
  g = take_turn(std::move(g), "Cathy discards r3 (slot 2)", "p5");

  const auto& cathy = g.state.hands[static_cast<int>(TestPlayer::CATHY)];
  ASSERT_NE(std::find(cathy.begin(), cathy.end(), paired), cathy.end())
      << "guard: the paired card is still in the receiver's hand";
  EXPECT_TRUE(g.meta[called].urgent)
      << "nothing has changed for the receiver, so the reaction is still urgent";
}

// CTP only. A spent reacter-CTD is left alone: the chuck list takes any
// CALLED_TO_DISCARD regardless of urgency, so it still reaches rung 11 and was
// never orphaned. `MiscReplay1972716.SpentReactionStopsBeingUrgent` depends on
// this, and it is the replay that motivated the de-urgenting in the first place.
TEST(Reactor0CallInvariants, ASpentReacterDiscardCallIsNotRelegated) {
  Game g = seed_reacter_call(setup(relegation_opts()),
                             CardStatus::CALLED_TO_DISCARD);
  const int called = order_at(g, TestPlayer::BOB, 3);

  g = take_turn(std::move(g), "Cathy plays r1 (slot 1)", "p5");

  ASSERT_EQ(g.meta[called].status, CardStatus::CALLED_TO_DISCARD)
      << "guard: the {y4} reading keeps rule 4 off it, so what this measures "
         "is rule 0 and nothing else";
  EXPECT_TRUE(g.meta[called].urgent)
      << "the CTD keeps its urgency -- only the pitch list filters on the "
         "urgency-derived classification, so only the CTP was ever orphaned";
}
