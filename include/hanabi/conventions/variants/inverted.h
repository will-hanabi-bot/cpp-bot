// Convention helpers for inverted suits (Orange / Dark Orange), whose
// game rule swaps the physical play and discard actions: PerformPlay sends
// the card to the discard pile, PerformDiscard is a play attempt onto the
// stack. The generic reactor conventions call into these helpers wherever
// an interpretation must account for that swap.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"

namespace hanabi::reactor::variants {

// True when `id` is on an inverted (Orange / Dark Orange) suit.
bool is_inverted_id(const State& state, Identity id);

// Reactor + inverted (Orange / Dark Orange) suits: when the receiver's
// reactive target is on an inverted suit, the reacter must invert their
// physical action (play↔discard) so that the receiver's standard reading
// of (clue kind + reacter action) ends up calling them to perform the
// physical action that *advances* the orange stack (or sends the orange
// card to the discard pile, depending on play_target vs dc_target). This
// helper reads the giver-visible identity of the target order.
bool target_is_inverted(const State& state, int target_order);

// For an inverted-suit (Orange / Dark Orange) reacter card, the convention's
// reacter call has these outcomes via the game-rule inversion:
//   * target_play(orange) ⇒ on the reacter's turn the bot would issue
//     PerformPlay, which the inversion turns into "discard the orange" —
//     the orange card goes to the discard pile with no stack progress.
//     A losing path whenever the orange is worth keeping, which is what this
//     helper assumes. It does NOT hold for an orange the holder knows is
//     basic trash: that is a free PITCH, and reactor0's rank Phase A skips
//     this guard entirely for one (`can_pitch_for_free` below,
//     reactor0/interpret_reactive.cpp:342-350). Kept blanket here because the
//     helper reads `state.deck` and so is POV-asymmetric — it may only ever
//     reject, and the exemption has to be decided on common knowledge.
//   * target_discard(orange) ⇒ PerformDiscard, which the inversion turns
//     into "play attempt" — if the orange is currently playable this
//     advances the orange stack (the intended outcome); otherwise it is
//     a misplay strike.
// The receiver-orange swap may toggle play↔discard for some
// clue/target combinations; this helper answers the post-swap question of
// whether we are about to take a losing path on an inverted reacter card.
bool would_lose_inverted_reacter(const State& state, int react_order,
                                 bool receiver_target_inverted,
                                 bool standard_is_target_play);

// CTP/CTD are PHYSICAL action labels. For an orange (inverted) focus the
// convention wants the receiver to advance the orange stack — the orange
// game-rule inversion requires PerformDiscard to do that — so mark CTD
// instead of CTP. The receiver's urgent_action then dispatches
// PerformDiscard naturally.
CardStatus called_focus_status(const State& state,
                               const IdentitySet& new_inferred);

// Build a DiscardAction for a known order in advance()'s simulation. For
// inverted (Orange / Dark Orange) suits the engine's `on_discard` runs the
// orange game-rule: failed=false advances the stack (via `with_play`),
// failed=true strikes. If we hard-code `failed=false` for non-playable
// inverted cards, `with_play` jumps the play stack to the (non-playable)
// rank — corrupting the simulated state. So inverted + !playable must
// use failed=true.
DiscardAction make_discard_for_simulation(const State& state, int player_index,
                                          int order);

// True when PerformDiscard of `id` advances an inverted stack — i.e. the
// identity is known, on an inverted suit, and currently playable.
bool discard_advances_stack(const State& state,
                            const std::optional<Identity>& id);

// True when any identity in `possible` is on an inverted suit. Used by the
// chuck-safety filter in `take_action`'s discard-candidate construction: a card
// that could be orange has no safe Discard button.
bool possible_has_inverted(const State& state, const IdentitySet& possible);

// True when some identity in `possible` is on an inverted suit **and** is
// currently playable — i.e. a chuck of this card could actually advance the
// stack. The unpinned counterpart to `discard_advances_stack`, and a strictly
// stronger test than `possible_has_inverted`: "might be orange" is not the same
// claim as "might advance the orange stack".
bool possible_chuck_advances_stack(const State& state,
                                   const IdentitySet& possible);

// Can the HOLDER of `order` pitch it for free? True when every identity the
// card could still be is on an inverted suit **and** is basic trash. Both
// halves are load-bearing:
//   * all-inverted, because pressing Play is only a *pitch* on an inverted
//     suit — on a normal suit it is a play attempt, and on a trash card that
//     is a misplay strike;
//   * all-trash, because a pitch sends the card to the discard pile, so a
//     useful orange loses a copy for nothing. A useful-but-not-critical
//     orange still counts as a loss and is deliberately excluded.
// Reads `common.thoughts[order].possible`, so giver, holder and every
// observer compute the same answer — the reactive walk may branch on it
// without desyncing (reactor0 §1g).
bool can_pitch_for_free(const Game& game, int order);

// Chop-save fallback for the rank-reactive path. If no play_target /
// finesse worked and the receiver's chop is an inverted-suit (orange)
// card, encode the rank reactive as "receiver PerformPlay's chop (= orange
// game-rule discard pile, a clean voluntary loss that avoids the misplay
// strike that would come from BOB PerformDiscard'ing a non-playable orange
// chop). Reacter does PerformPlay → CTP on reacter, mirroring the
// convention "rank + reacter plays = receiver plays."
std::optional<ClueInterp> orange_chop_save(
    const Game& prev, Game& game, const ClueAction& action, int focus_slot,
    int reacter, const std::vector<std::pair<int, Identity>>& possible_conns);

}  // namespace hanabi::reactor::variants
