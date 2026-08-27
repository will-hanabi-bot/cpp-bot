// reactor0: the shape a hand's outstanding calls are allowed to take.
//
// Four rules, all enforced after every reactor0 interpretation rather than at
// each stamping site, so no path can forget them.
//
// 1. **Play calls run in play order.** A hand may carry several
//    CALLED_TO_PLAY cards at once, and the holder actions them
//    most-recently-stamped first. Left alone that order can disagree with
//    slot order: an older clue calls slot 2, a newer clue calls slot 4, and
//    the holder plays slot 4 then comes back to slot 2. The convention
//    resolves this by erasing, not reordering — a newer clue would not have
//    pointed past a card that was still playable, so the earlier call is
//    dead. The resulting invariant is that CTP cards run newest slot to
//    oldest slot in exactly play order, which is what makes the shared
//    urgent scan in `Game::take_action` correct without consulting signal
//    turns: the first urgent card in slot order *is* the most recent call.
//
//    **The erasure is ASYMMETRIC in the kind of call doing it.** `urgent`
//    separates the two, the same discriminator `calls_of` uses:
//      * a RECEIVER call (non-urgent) retires both kinds to its left —
//        landing to the right of a standing reacter call means leftmost
//        targeting has left that reacter card unactionable;
//      * a REACTER call (urgent) retires only other REACTER calls. It is
//        actioned by the urgent scan on the holder's very next turn and
//        never joins the receiver deque, so it has no standing to retire a
//        receiver call. Replay 1974512 is what the symmetric version cost:
//        a reacter stamp on an older slot erased a standing receiver call on
//        a playable p1, which the holder then never picked back up.
//
// 3. **A dead call is dropped.** A call is only as good as the card. Once
//    common knowledge leaves the stamped button with no identity it handles
//    correctly, every seat drops it -- which also removes the card from the
//    reacter-CTP and receiver-CTP structures, since those are derived from
//    these stamps. Judged against `pitch_candidates`, not against playability:
//    a CTP on an inverted card is a PITCH, and being unplayable is what makes
//    that call sensible in the first place.
//
// 2. **At most one discard call.** Unlike play calls, CALLED_TO_DISCARD does
//    not stack: a player holds at most one at a time, and a new call
//    replaces the standing one. Cards merely *revealed* to be basic trash
//    (`meta.trash`) are not discard calls and are left alone — they outrank
//    the chop for discard purposes without occupying the CTD slot.
#pragma once

namespace hanabi {
class Game;
}

namespace hanabi::reactor0 {

// Rule 4 -- a CTD whose chuck could now only strike is erased, the mirror of
// rule 3 for the other button. A chuck presses Discard, which on an INVERTED
// suit is a play attempt, so the call dies when every remaining reading is an
// inverted card that is not next for its stack (tested against
// `chuck_candidates`). Leaving it in place is not harmless: `Game::chop`'s
// first pass returns a CTD, so a stale one silently becomes the hand's chop,
// and `requires_high_tier` counts it, so the holder stays "occupied". Replay
// 1967287.
//
// Enforce every rule across all hands. Idempotent.
//
// Call after any reactor0 interpretation that may have stamped a call -- and
// after EVERY play and discard, whether or not a reaction was being resolved.
// Rules 3 and 4 turn on the STACKS, not on the stamps, so a call can die
// because somebody else advanced a pile. Until v10.12.0 the play and discard
// hooks only reached this from inside their `if (!waiting.empty())` block, so
// an ordinary play never re-checked standing calls and a dead one could survive
// indefinitely -- replay 1971981, where a receiver call narrowed to {r1, m1}
// outlived both being played and the holder blind-played it into a strike.
void enforce_call_invariants(Game& game);

}  // namespace hanabi::reactor0
