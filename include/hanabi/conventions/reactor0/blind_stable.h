// The BLIND families' stable clue convention. See
// src/conventions/reactor0/CONVENTION.md §1f.
//
// Twelve variants where a clue of one or both kinds is LEGAL but touches no card:
//
//   Color Blind (3-6 Suits)    -- `colorCluesTouchNothing`
//   Number Blind (3-6 Suits)   -- `rankCluesTouchNothing`
//   Totally Blind (3-6 Suits)  -- both
//
// A clue that touches nothing can carry nothing about WHICH cards it reached, so
// its whole meaning is which clue was given. Same shape as the Synesthesia table
// next door, and for the same reason: a fixed (button, slot) lookup, plus a LOCK
// row for the value that has no slot to name.
//
//   colour   Red 1 pitch | Yellow 2 pitch | Green 3 pitch | Blue 4 pitch
//            Purple 5 pitch | any other colour LOCK
//   rank     1 chuck | 2 chuck | 3 chuck | 4 chuck | 5 LOCK
//
// The two tables are INDEPENDENT, keyed only on the clue's own kind, which is
// what makes Totally Blind need no rule of its own -- it is Color Blind and
// Number Blind at once. It is also what makes the dispatch per-KIND rather than
// per-variant: in Color Blind a RANK clue still touches normally and still uses
// the ordinary `stable_rank` ladder.
//
// REACTIVE MEANINGS ARE UNCHANGED from No Variant. The sum rule reads a slot and
// an anchor, neither of which depends on what a clue touched, so none of it
// moves; `uses_target_parity` is false here and the ordinary rank-even /
// colour-odd buckets apply.
//
// REACTOR DOES NOT IMPLEMENT THIS (v15.0.0). Its stable ladder is referential and
// needs newly-touched cards, so every blind clue there reads STALL. Games at 4+
// players force reactor, so they play these variants with no clue channel at all.
#pragma once

#include <optional>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/variant.h"

namespace hanabi::reactor0 {

// What a blind clue names.
struct BlindCall {
  // No slot to name: the clue locks the target's whole hand. `button` and `slot`
  // are meaningless when this is set.
  bool lock = false;
  // CALLED_TO_PLAY is a *pitch* (press Play), CALLED_TO_DISCARD a *chuck* (press
  // Discard) -- the glossary's names for the two buttons, which on an inverted
  // suit swap their ordinary effects.
  CardStatus button = CardStatus::CALLED_TO_PLAY;
  int slot = 1;  // 1-based, in the clued seat's hand
};

// The table. A pure function of the variant and the clue, so `/settings` and the
// table tests can pin it without a Game.
//
// Colour is keyed on the clue colour's NAME, as the Synesthesia table is: Red
// through Purple name their own position, and ANY other colour -- Teal in the
// 6-suit members, and any dark or special suit a future variant adds -- locks.
// Rank is keyed on the value: 1-4 name their slot, 5 locks.
//
// Note the two buttons differ by kind: a colour names a PITCH and a rank a CHUCK.
// That is deliberate and is the only place the kind carries meaning here, which
// is what lets Totally Blind offer both a play call and a discard call for the
// same slot.
BlindCall blind_call(const Variant& variant, ClueKind kind, int value);

// Is a clue of this kind blind in this variant?
bool clue_kind_is_blind(const Variant& variant, ClueKind kind);

// Read a stable blind clue. `action.target` is Bob, since a stable clue is only
// ever one to Bob.
//
// The ladder is `synesthesia_stable`'s, shared deliberately so the two cannot
// drift:
//
//   * the slot does not exist (a 4-card hand cannot answer Purple) -> STALL;
//   * the named button is bad by COMMON knowledge (`slot_is_pitchable` /
//     `slot_is_chuckable` over `effective_possible_for`) -> STALL, so giver and
//     receiver agree that no call was made;
//   * the named button is bad by GIVER-ONLY sight -> nullopt, a MISTAKE the giver
//     never offers, because Bob would otherwise act on a call the seeing seats
//     had quietly cancelled;
//   * otherwise stamp through the shared `stamp_react_*_button` ladders.
//
// The LOCK row stamps CHOP_MOVED across the target's whole hand and returns
// `ClueInterp::LOCK`, and refuses (MISTAKE) when the target already reads locked
// -- the same guard `reactor::ref_discard`'s lock arm carries.
std::optional<ClueInterp> blind_stable(Game& game, const ClueAction& action);

}  // namespace hanabi::reactor0
