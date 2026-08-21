// reactor0: the shape a hand's outstanding calls are allowed to take.
//
// Two rules, both enforced after every reactor0 interpretation rather than at
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

// Enforce both rules across every hand. Idempotent. Call after any reactor0
// interpretation that may have stamped a call.
void enforce_call_invariants(Game& game);

}  // namespace hanabi::reactor0
