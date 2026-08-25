// Position facts reactor0's decision layer is built from.
//
// These are *rules*, not tuning: each one answers a question a human player
// would ask out loud ("can Bob afford to lose his chop?", "can I prove I hold a
// copy of that?"), and each has a definition in DECISION_MAKING.md. They were
// private to `state_eval.cpp` while the only consumer was the clue-tier gate;
// v7.0.0's General Clue Evaluation List needs them too, so they live here.
//
// Deliberately NOT in `state_eval.h`: most of that header describes the
// tuned-constant scorer being removed, and these facts outlive it.
#pragma once

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/state.h"

namespace hanabi::reactor0 {

// Can Alice *prove* she holds a copy of `id`? Extends the singleton test to
// group ("sudoku") elim: for any subset S of Alice's hand, if fewer than |S|
// copies of (what S could be, minus `id`) are unaccounted for, one of them must
// be `id`. Inference sets come from `common`; availability counts from `me()`.
// Errs safe — a false positive would kill a save clue, so the bound over-counts.
bool alice_provably_holds(const Game& game, int alice, Identity id);

// Is there a second copy of `id` elsewhere in the SAME hand? A chop the holder
// can safely pitch because they hold the other copy is not in danger.
bool has_same_hand_dupe(const State& s, int player, int chop_order, Identity id);

// Is `player`'s chop a card the team cannot afford to lose? Judged from Alice's
// full visibility: the identity is known and not basic trash, there is no second
// copy in the holder's own hand, no copy in the third player's hand, and Alice
// cannot prove she holds one. False for a locked hand, which has no chop.
bool at_risk_chop(const Game& game, int alice, int player);

// Does `player` have a *playable* chop that is not a same-hand dupe? Weaker than
// `at_risk_chop` on purpose — the point is not that the card is in danger but
// that it is a play the team should be collecting.
bool has_playable_chop(const Game& game, int player);

// What a clue's interpretation does to the play count, walked as CTP-status
// transitions between the real game and the clue's hypo.
//
// Known blind spot (TODO entry 4): on an inverted suit a *play* call is stamped
// CALLED_TO_DISCARD, so a genuine two-play reactive reads here as zero plays.
struct NewPlayFacts {
  int count = 0;
  bool critical_low = false;  // a critical 1 or 2 (5 or 4 reversed) plays
  bool regain_rank = false;   // at least one play regains a clue token
};
NewPlayFacts new_play_facts(const Game& game, const Game& hypo);

// Is `player`'s chop a card they can afford to lose — basic trash, or a card
// they hold a second copy of? This is H1c's "expendable chop" clause, and it is
// also the gate on priority 3 rung 2: a double discard that redirects Cathy off
// her chop is only urgent when that chop was worth keeping.
//
// False when the hand is locked (no chop) or the chop's identity is unknown to
// the caller.
bool chop_is_expendable(const Game& game, int player);

// Is this candidate a VH1 clue — a finesse worth the tempo? True when the
// interpretation is reactive rank Phase B (`interpret_reactive.cpp:383-447`) and
// Cathy's chop is not trash or a same-hand-dupe.
//
// The sole member of `ClueTier::VERY_HIGH`, the tier Precedence step 1 admits.
// Through v9.2.0 the rule was called **H4**; v9.3.0 named the tier instead and
// reused "H4" for the unrelated critical-chop rule, so the old name in a commit
// message or an archived log means this predicate, not that one.
//
// `hypo` must be `game.simulate(Action{action})`.
bool clue_is_vh1(const Game& game, const Game& hypo, const ClueAction& action);

// Could `giver` give `receiver` a colour clue that names a genuinely playable
// card? A structural replay of `stable_colour`'s target choice and its three
// guards, not a simulation. The costliest fact here — evaluate it last.
bool has_colour_play_clue_for(const Game& game, int giver, int receiver);

// --- private sight -------------------------------------------------------
//
// `order`'s empathy set, narrowed by what THIS SEAT can see: an identity drops
// out when every copy still in the game is visible in somebody else's hand.
//
// The Player layer cannot express this. Per-player views are copied from
// `common` (basics/game.cpp) and re-elim'd, and `card_elim` accounts for a copy
// only when a CLUE pins it (`certain_map`) -- never when a player merely looks
// at it. Private sight is `state.deck[o].id()`, which is nullopt for one's own
// cards, so this is deliberately a PER-SEAT reading: see CONVENTION.md §1g.
//
// Built on `possible`, not `inferred`: sight REFUTES a promise rather than
// refining it, so it must start from raw empathy.
//
// `Player::unknown_ids` (basics/player.cpp) is the same arithmetic over
// empathy; this is its sight-based twin, kept separate so that function's
// existing callers are untouched.
IdentitySet sight_narrowed(const Game& game, int order);

// True when every identity `sight_narrowed` leaves is basic trash -- the holder
// can PROVE the card is worthless, even though common knowledge cannot.
bool provably_trash(const Game& game, int order);

}  // namespace hanabi::reactor0
