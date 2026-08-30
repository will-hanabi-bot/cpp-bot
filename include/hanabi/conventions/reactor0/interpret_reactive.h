// Reactor0 reactive interpretation. The anchor of the slot arithmetic
// (react_slot + target_slot ≡ anchor, mod hand size) is the CLUE VALUE:
// the rank for rank clues, the fixed colour value (colour_value.h) for
// colour clues. There is no reactive focus.
//
// Rank = an even number of plays (2 or 0):
//   A. double play — target = leftmost playable in the receiver's hand,
//      INCLUDING already-CTP'd cards; the reacter blind-plays the react
//      slot. Targets whose react slot is visibly unworkable are skipped
//      leftmost-first.
//   B. finesse — target = leftmost one-away-from-playable; the reacter
//      must hold the connector at the react slot.
//   C. double discard — the reacter DISCARDS the react slot and the
//      receiver discards the dc-target (leftmost trash / same-hand dupe;
//      none → reactive lock when rlocks is on, else sacrifice).
//
// Colour = one play:
//   1. receiver has a playable → reacter discards react slot, receiver
//      plays the target (leftmost playable; react slots holding a known
//      critical are skipped by advancing the target).
//   2. no playable → reacter BLIND-PLAYS the react slot (the giver only
//      gives the clue when the react-slot card is visibly playable) and
//      the receiver discards the dc-target / locks.
#pragma once

#include <optional>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor0 {

// Does a clue to BOB still read as reactive?
//
// Under target parity (Alternating Clues, Synesthesia) every clue is reactive, a
// clue to Bob included -- but only while the score is below **50% of the variant
// maximum**. Past that a clue to Bob is STABLE again (v11.0.0 at 60%, moved to
// 50% in v11.4.0).
//
// Why the switch exists. The reactive reading of a clue to Bob is the ODD
// bucket, which is a reactive DISCARD. Late in a game that is actively harmful:
// it forces a discard nobody needed and it crowds out the two things that
// actually matter near the end -- saving a good card, and getting a play clue
// out in time. Clues to CATHY are unaffected and keep the even bucket.
//
// The cap is the VARIANT maximum, a constant 5 per suit, deliberately not
// `State::max_score()`. A shrinking cap would move the switch point mid-game as
// criticals died, and both seats must agree on which side of it every past clue
// was given. Compared in integers -- `2 * score >= cap` -- so there is no float
// rounding for two seats to disagree about. Half of an odd cap is not a whole
// number, so the switch lands on the first score at or above it: 13 of 25.
//
// FALSE outside a target-parity variant, where a clue to Bob was never reactive
// in the first place.
bool bob_clue_is_reactive(const State& state);

// Is a COLOUR clue in this variant never stable? (v13.0.0)
//
// True in the nine target-parity variants that offer only TWO clue colours --
// every one of them Red + Blue: six Alternating Clues (Rainbow, White, Omni,
// Null, Muddy Rainbow, Light Pink) and three Synesthesia (Rainbow, White, Null).
// There a colour clue to Bob stays reactive whatever `bob_clue_is_reactive`
// says, so neither the 8-clue arm nor the score threshold can stand it down.
//
// Two colours leave only two colour anchors, which is why v11.5.0 made a colour
// clue to Bob an EVEN-bucket clue with its own anchor (Red 2, Blue 5). The
// stable exemption cut across that second bucket; this closes it.
//
// SYNESTHESIA HAS NO RANK CLUES (`clueRanks: []`), so in its three two-colour
// variants this leaves no stable channel at all and `synesthesia_stable` is
// unreachable. Deliberate. A forced turn there can no longer reach for a stable
// clue, but it can always act: `calls.cpp` empties the chuck list at 8 tokens
// and pitches the chop instead.
//
// Deliberately the SAME condition as `bob_colour_joins_even`
// (`reactive_assignment.h`), which it delegates to rather than restating -- one
// source of truth for the family, two names so each call site says which of the
// two rules about it is meant.
bool colour_is_never_stable(const Variant& variant);

// Is this clue REACTIVE?
//
// Positional everywhere except a target-parity variant (Alternating Clues,
// Synesthesia), where a clue to Bob is reactive too -- it simply touches the
// reacter's own hand -- until `bob_clue_is_reactive` says the score has passed
// the threshold above.
//
// This and `reactive_receiver` are the SINGLE definition of reactor0's dispatch
// rule. Every site that needs to know whether a clue is reactive, or who
// receives it, reads them rather than repeating the test. Replay 1973971 T15 is
// what that rule is worth: five separate places had re-derived it as
// `action.target`, and a reactive discard clue to Bob read as a MISTAKE.
bool clue_is_reactive(const State& state, const ClueAction& action, int bob);

// Who the reaction identifies a slot FOR.
//
// Normally the clued seat, `action.target`. In a `variants::uses_target_parity`
// variant (Alternating Clues, Synesthesia) it is ALWAYS Cathy -- the seat after
// the reacter -- because there a clue to Bob touches the reacter's own hand and
// only sets the parity, while the slot it identifies still belongs to Cathy.
int reactive_receiver(const State& state, const ClueAction& action, int reacter);

// Stamp a slot when the call is the PLAY button (a real play, or a *pitch* on an
// inverted suit, where Play discards). Three steps, in order: every reading
// inverted -> pitch; otherwise the ordinary play reading; otherwise, for a clued
// or already-stamped card with an inverted reading it can spare, a pitch. See
// the definition for why the order is load-bearing (replays 1973976 T12 and
// 1974331 T8).
//
// Stamp a slot when the call is the DISCARD button: a *chuck* that stacks an
// inverted card, else the ordinary throw-away reading (replay 1974342 T13).
//
// ONE ladder each, shared by every site that issues the button so they cannot
// drift.
//
// `urgent` is what a REACTIVE call adds: a reaction must be actioned on the
// reacter's very next turn, which is what `decide.cpp`'s urgent scan keys on.
// `stable` is `target_play`'s own flag, and it moves the OTHER way -- it widens
// the delayed-play chain the stamp narrows `inferred` against, which a reactive
// clue cannot rely on when a player has several obvious playables
// (`reactor/interpret_clue.cpp:107`).
//
// They are separate parameters rather than one, even though every caller so far
// sets them opposite: collapsing two questions into one flag is exactly the kind
// of coupling that drifts. Synesthesia's stable table (§1f) passes
// `urgent=false, stable=true` -- it names an action, but pends no reaction.
std::optional<ClueInterp> stamp_react_play_button(Game& game,
                                                  const ClueAction& action,
                                                  int react_order,
                                                  bool urgent = true,
                                                  bool stable = false);
std::optional<ClueInterp> stamp_react_discard_button(Game& game,
                                                     const ClueAction& action,
                                                     int react_order,
                                                     bool urgent = true);

// `receiver` is what `reactive_receiver` returns; callers pass it explicitly so
// the reacter/receiver pair is decided in one place.
//
// Nothing in the reactive branches reads `action.list_`, so a clue that touches
// the REACTER rather than the receiver needs no further special-casing.
std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                             const ClueAction& action,
                                             int reacter, int receiver);

}  // namespace hanabi::reactor0
