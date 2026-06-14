// Fundamental reactor-convention invariants.
//
// These pin the most basic guarantees a reactive clue must satisfy
// before the giver issues it. A reactive clue's "reacter slot" is a
// fixed function of the receiver's leftmost-playable target slot via
// `calc_slot(focus_slot, target_slot, hand_size)`. The reacter card
// at that slot must be a card the reacter can actually PLAY this turn
// — otherwise the reacter strikes when they execute their convention-
// assigned play. The giver, who can see the reacter's hand, must
// detect this and not issue the clue.

#include <gtest/gtest.h>

#include <variant>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/player.h"
#include "test_harness.h"

using namespace hanabi;
using namespace hanabi::test;

// Reproducer for replay 1892259 T45. Variant: Funnels & Dark Prism
// (6 Suits). Stacks: r=2, y=4, g=4, b=5, p=0, i=2. Alice = giver
// (will-bot67 in the replay), Bob = reacter (yagami_black), Cathy =
// receiver (will-bot69).
//
// Bob's slot 1 actual = y1 (basic trash, y_stack = 4). Cathy's slot 1
// actual = p1 (currently playable on the empty p stack — the
// "leftmost playable" target).
//
// A rank-2 funnel clue → Cathy decomposes via the reactor as:
//   - leftmost playable target = p1 at Cathy slot 1
//   - focus_slot = clue.value = 2 (Funnels variant routes through the
//     pinkish-rank focus rule, which sets focus = the clued rank)
//   - target_slot = 1
//   - react_slot = calc_slot(2, 1, hand_size=5) = (2+5-1) % 5 = 1
//   - reacter card = Bob slot 1 = y1 (basic trash → strike on play)
//
// The convention's narrowing on Bob's CTP'd slot 1 produces the
// inferred set `{r3, y5, g5, p1, i3}` (= the current playable_set
// since `delayed_plays` from Alice to Cathy yields no connectors).
// y1 is NOT in that set, so Alice — who sees Bob's actual y1 —
// knows committing to this clue means Bob will strike. The
// `find_all_clues` filter must drop the candidate.
TEST(ReactiveBasics, FiltersRankClueWhoseReacterSlotIsBasicTrash) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver, POV). Five distinct rank-5 cards (b5 is already
      // on the b stack so we use other-suit rank-5s as filler).
      {"r5", "y5", "g5", "p5", "i5"},
      // Bob (reacter). Slot 1 = y1 (basic trash since y stack=4).
      // Other slots: pick non-conflicting cards.
      {"y1", "y2", "y3", "y4", "p4"},
      // Cathy (receiver). Slot 1 = p1 (leftmost playable, p stack=0).
      {"p1", "p2", "p3", "g2", "g3"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.play_stacks = std::vector<int>{2, 4, 4, 5, 0, 2};
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));

  // The bad clue must not appear in the candidate list.
  for (const auto& p : clues) {
    if (auto* r = std::get_if<PerformRank>(&p)) {
      bool bad = r->target == static_cast<int>(TestPlayer::CATHY) &&
                 r->value == 2;
      EXPECT_FALSE(bad)
          << "Rank-2 → Cathy must be filtered. The reactive interp's "
             "calc_slot rule lands the reacter slot on Bob's slot 1 = "
             "y1 (basic trash, y_stack=4); Alice can see Bob would "
             "strike when executing the convention's play. Replay "
             "1892259 T45 reproducer.";
    }
  }
}

// Reproducer for replay 1892428 T47 — the CTD-critical mirror of the
// 1892259 CTP-strike case above. Variant: Funnels & Dark Prism (6 Suits).
// Stacks: r=4, y=2, g=5, b=5, p=5, i=2 (matches the replay shape: only
// rank-5 colours remain on r/y/i and dark prism i is critical for every
// unplayed rank).
//
// Alice = giver. Bob = reacter, slot 2 = i4 (critical — the only i4 in
// a Dark Prism deck). Cathy = receiver, slot 1 = i3 (leftmost playable
// on i_stack=2) and slot 3 = g2 (basic trash, but touched by a green
// clue via prism rank-3 → color-2).
//
// A green clue → Cathy touches both Cathy's slot 1 (i3 via prism) and
// slot 3 (g2 directly). `reactive_focus` uses the newest-demoted rule:
// the touched newest slot (i3 at slot 1 = `hand[0]`) is demoted to
// key -1, so max_element picks the other touched slot (slot 3) as the
// focus. focus_slot = 3. The reactive play-target is the leftmost
// playable in Cathy = i3 slot 1, so target_slot = 1. Then:
//   react_slot = calc_slot(3, 1, hand_size=5) = (3+5-1) % 5 = 2
//   reacter card = Bob slot 2 = i4 (critical → unrecoverable if discarded)
//
// `target_discard` would stamp Bob's slot 2 CTD-urgent with inferred
// narrowed to non-critical ids — promising the reacter that the slot is
// safe to discard. Bob's solver then discards i4, capping the max score.
// Alice can see Bob's actual slot 2 = i4 and must filter the clue.
TEST(ReactiveBasics, FiltersColourClueWhoseReacterSlotIsCritical) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver, POV). Filler chosen to fit card counts at these
      // stacks (no rank-5 from completed g/b/p suits, etc.).
      {"y3", "y4", "p3", "p4", "b3"},
      // Bob (reacter). slot 2 = i4 (only copy → critical).
      {"r2", "i4", "r3", "r4", "g3"},
      // Cathy (receiver). slot 1 = i3 (leftmost playable), slot 3 = g2
      // (touched by the green clue, used to trigger the newest-demoted
      // focus that maps the reacter slot to Bob's slot 2).
      {"i3", "y2", "g2", "y1", "p1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.play_stacks = std::vector<int>{4, 2, 5, 5, 5, 2};
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));

  for (const auto& p : clues) {
    if (auto* c = std::get_if<PerformColour>(&p)) {
      bool bad = c->target == static_cast<int>(TestPlayer::CATHY) &&
                 c->value == 2;
      EXPECT_FALSE(bad)
          << "Colour-green → Cathy must be filtered. The reactive "
             "convention's newest-demoted focus rule maps the reacter "
             "slot to Bob's slot 2 = i4 (the only i4 in Dark Prism). "
             "`target_discard` would CTD-urgent that slot with a "
             "filtered-non-critical inferred; the reacter's solver then "
             "discards a critical card. Replay 1892428 T47 reproducer.";
    }
  }
}

// Sanity sibling: same shape but Bob's slot 2 swapped to a non-critical
// card (y4 has two copies, both still alive at y_stack=2). The green
// clue should survive find_all_clues — the filter must only drop clues
// where the actual id at the reacter slot is critical from the giver's
// POV.
TEST(ReactiveBasics, AllowsColourClueWhenReacterSlotIsNonCritical) {
  SetupOptions opts;
  opts.hands = {
      {"y3", "p3", "p4", "b3", "b4"},
      // Bob slot 2 = y4 (non-critical: y4 has 2 copies, 0 discarded, so
      // discarding one is recoverable).
      {"r2", "y4", "r3", "r4", "g3"},
      {"i3", "y2", "g2", "y1", "p1"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.play_stacks = std::vector<int>{4, 2, 5, 5, 5, 2};
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));

  bool found = false;
  for (const auto& p : clues) {
    if (auto* c = std::get_if<PerformColour>(&p)) {
      if (c->target == static_cast<int>(TestPlayer::CATHY) && c->value == 2) {
        found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found)
      << "Colour-green → Cathy should survive find_all_clues when the "
         "reacter-slot card (Bob slot 2 = y4) is non-critical. The "
         "filter must only drop clues where the CTD'd card is critical.";
}

// Symmetric sanity: when the reacter's slot 1 is currently playable,
// the same clue is acceptable and remains in the candidate list. This
// guards against the filter being overly broad.
TEST(ReactiveBasics, AllowsRankClueWhenReacterSlotIsPlayable) {
  SetupOptions opts;
  opts.hands = {
      {"r5", "y5", "g5", "p5", "i5"},
      // Bob slot 1 = r3 (currently playable, r stack=2).
      {"r3", "y2", "y3", "y4", "p4"},
      // Cathy slot 1 = p1 (leftmost playable).
      {"p1", "p2", "p3", "g2", "g3"},
  };
  opts.variant_name = "Funnels & Dark Prism (6 Suits)";
  opts.play_stacks = std::vector<int>{2, 4, 4, 5, 0, 2};
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));

  bool found = false;
  for (const auto& p : clues) {
    if (auto* r = std::get_if<PerformRank>(&p)) {
      if (r->target == static_cast<int>(TestPlayer::CATHY) && r->value == 2) {
        found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found)
      << "Rank-2 → Cathy should survive find_all_clues when the "
         "convention's calculated reacter slot (Bob slot 1 = r3) is a "
         "currently playable card. The filter must only drop clues "
         "where the reacter would actually strike.";
}

// Reproducer for replay 1892505 T32 — the pre-clued-trash focus
// convention. Variant: Funnels & Prism (4 Suits) — suits are Red,
// Green, Blue, Prism. Stacks all set to 3 so every rank-1-to-3
// identity is basic-trash from common knowledge. Alice = giver. Bob =
// reacter, slot 4 = i4 (currently playable, prism stack = 3). Cathy =
// receiver, slot 4 = b2 pre-clued with rank ≤ 3 so its post-elim
// `possible` reduces to a non-empty subset of basic-trash ids —
// `common.order_kt(Cathy slot 4) == true` and the slot lands in
// `prev_kt`.
//
// Alice clues colour-red to Cathy. Red touches red cards + prism
// rank 1 + prism rank 4. In Cathy that's slot 1 (r1) and slot 3 (r5)
// — both newly clued. Newest-demoted focus rule: hand[0] (slot 1)
// demoted, focus lands on slot 3 → focus_slot = 3.
//
// Pre-fix behaviour: `play_targets` in Cathy = ∅, `dc_targets`
// cascade picks `unknown_trash` (the unclued basic-trash slots) as
// the first pool — none of those react-slot maps to Bob's slot 4 =
// i4, so any candidate that target_plays through hits a Bob slot
// whose actual id isn't in the narrowed inferred. `get_result`'s
// `hypo_plays.count` check filters those, and the red clue is
// dropped from find_all_clues entirely.
//
// Post-fix behaviour: the new `pre_clued_trash` pool (cards in
// `prev_kt` AND pre-clued before this turn) is consulted first in
// the dc_targets cascade. Cathy slot 4 is the lone candidate.
// target_slot = 4, focus_slot = 3, react_slot = calc_slot(3, 4, 5) =
// 4 → Bob's slot 4 = i4. `target_play(i4)` narrows inferred to a
// subset that contains Bob's actual i4, so the convention's promise
// holds. The clue survives find_all_clues.
TEST(ReactiveBasics, AllowsColourClueWithPreCluedTrashFocus) {
  SetupOptions opts;
  opts.hands = {
      // Alice (giver, POV). Non-red filler so the red clue doesn't
      // accidentally touch Alice's hand.
      {"g4", "b4", "g5", "b5", "i5"},
      // Bob (reacter). Slot 4 = i4 (currently playable on prism
      // stack = 3). Other slots filler; none currently playable
      // (otherwise the reacter loop's vacuous any_kept calculation
      // wouldn't fire).
      {"g3", "b3", "g2", "i4", "g1"},
      // Cathy (receiver). Slot 1 = r1, slot 3 = r5 (both newly
      // touched by red, drives focus_slot = 3 via newest-demoted).
      // Slot 4 = b2 (will be pre-clued via rank-3 below to seed
      // pre-clued basic-trash).
      {"r1", "b1", "r5", "b2", "g1"},
  };
  opts.variant_name = "Funnels & Prism (4 Suits)";
  opts.play_stacks = std::vector<int>{3, 3, 3, 3};
  opts.starting = TestPlayer::ALICE;
  Game g = setup(std::move(opts));

  // Pre-clue Cathy's slot 4 with rank-3 (Funnels rank ≤ 3 touches
  // ranks 1-3). With stacks all at 3, every still-possible identity
  // for slot 4 is basic-trash, so common-knowledge order_kt holds.
  g = pre_clue(std::move(g), TestPlayer::CATHY, /*slot=*/4, {"3"});
  g.elim();

  int cathy_s4 = g.state.hands[static_cast<int>(TestPlayer::CATHY)][3];
  ASSERT_TRUE(g.common.order_kt(g, cathy_s4))
      << "test setup must produce a pre-clued basic-trash slot for "
         "the convention to focus on";

  auto clues = g.find_all_clues(static_cast<int>(TestPlayer::ALICE));
  bool found_red_to_cathy = false;
  for (const auto& p : clues) {
    if (auto* c = std::get_if<PerformColour>(&p)) {
      if (c->target == static_cast<int>(TestPlayer::CATHY) &&
          c->value == 0) {
        found_red_to_cathy = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found_red_to_cathy)
      << "Colour-red → Cathy must survive find_all_clues. With the "
         "pre_clued_trash pool the convention now produces a valid "
         "REACTIVE that CTPs Bob's slot 4 (currently-playable i4); "
         "without the pool, the convention falls back to an "
         "unknown_trash candidate whose react_slot CTPs a non-"
         "playable card on Bob → get_result returns −100 and the "
         "clue is dropped. Replay 1892505 T32 reproducer.";

  g = take_turn(std::move(g), "Alice clues Red to Cathy");
  int bob_s4 = g.state.hands[static_cast<int>(TestPlayer::BOB)][3];
  EXPECT_EQ(g.meta[bob_s4].status, CardStatus::CALLED_TO_PLAY)
      << "After Alice's red clue Bob's slot 4 (the convention's "
         "reacter mapping target) must be CTP'd via the new "
         "pre_clued_trash → target_play path.";
  EXPECT_TRUE(g.meta[bob_s4].urgent);
}
