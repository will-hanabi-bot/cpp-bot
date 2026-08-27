#include "hanabi/conventions/reactor0/synesthesia_stable.h"

#include <string>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/variants/inverted.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;
using hanabi::reactor::effective_possible_for;

SynesthesiaCall synesthesia_call(const Variant& variant, int colour_index) {
  const auto& names = variant.clue_colour_names;
  if (colour_index < 0 || colour_index >= static_cast<int>(names.size())) {
    return SynesthesiaCall{CardStatus::CALLED_TO_PLAY, 4};
  }
  const std::string& name = names[colour_index];
  if (name == "Red") return {CardStatus::CALLED_TO_PLAY, 1};
  if (name == "Yellow") return {CardStatus::CALLED_TO_PLAY, 2};
  if (name == "Green") return {CardStatus::CALLED_TO_DISCARD, 3};
  if (name == "Blue") return {CardStatus::CALLED_TO_DISCARD, 2};
  if (name == "Purple") return {CardStatus::CALLED_TO_PLAY, 5};
  if (name == "Orange") return {CardStatus::CALLED_TO_DISCARD, 1};
  return {CardStatus::CALLED_TO_PLAY, 4};
}

std::optional<ClueInterp> synesthesia_stable(Game& game,
                                             const ClueAction& action) {
  const State& s = game.state;
  const SynesthesiaCall call = synesthesia_call(*s.variant, action.clue.value);
  const bool pitch = call.button == CardStatus::CALLED_TO_PLAY;

  // The slot may simply not be there: a 4-card hand cannot answer Purple, and a
  // hand worn down at the end of the deck cannot answer much. Nothing to stamp,
  // so the clue is a stall -- which is a real thing to spend a token on.
  const auto& hand = s.hands[action.target];
  if (call.slot > static_cast<int>(hand.size())) return ClueInterp::STALL;
  const int order = hand[call.slot - 1];

  // Vet on COMMON knowledge, so giver and receiver reach the same verdict about
  // whether a call was made at all. `effective_possible_for` is the empathy set
  // every seat reconstructs identically, which is what makes that true.
  //
  // Both predicates are EXISTENTIAL (interpret_reaction.h): the button is
  // acceptable if ANY reading the slot still admits makes it so.
  const IdentitySet cand = effective_possible_for(game, order);
  const bool acceptable =
      pitch ? slot_is_pitchable(s, cand) : slot_is_chuckable(s, cand);
  if (!acceptable) return ClueInterp::STALL;

  // POV-invariant abort, the same shape the reactive branches use:
  // `state.deck[o].id()` is nullopt from the holder's own seat, so this fires
  // only for the seats that can SEE the card. If what they see makes the named
  // action bad, the clue is a MISTAKE and the giver never offers it. It must not
  // instead degrade to a stall -- Bob cannot see what they see, so he would act
  // on the call while they had quietly agreed it was cancelled.
  //
  // The four cases are the two buttons crossed with the two suit kinds, and the
  // inverted column is the mirror of the plain one:
  //
  //             | plain suit              | inverted suit
  //   pitch     | Play plays it: must be  | Play throws it: must be
  //   (Play)    | playable                | a copy we can spare
  //   chuck     | Discard throws it: must | Discard stacks it: must be
  //   (Discard) | not be the last copy    | playable
  if (auto id = s.deck[order].id()) {
    const bool inverted = variants::is_inverted_id(s, *id);
    const bool bad = pitch ? (inverted ? s.is_critical(*id)
                                       : !s.is_playable(*id))
                           : (inverted ? !s.is_playable(*id)
                                       : s.is_critical(*id));
    if (bad) return std::nullopt;
  }

  // One ladder each, shared with the reactive sites so the two cannot drift --
  // `urgent=false` because a stable clue names an action without pending a
  // reaction that must be actioned on the very next turn.
  auto interp = pitch ? stamp_react_play_button(game, action, order,
                                               /*urgent=*/false,
                                               /*stable=*/true)
                      : stamp_react_discard_button(game, action, order,
                                                   /*urgent=*/false);
  // The ladder can still refuse where the existential vet passed: it narrows
  // `inferred` rather than merely testing it, and a narrowing can come back
  // empty. Nothing was stamped, so this is the stall again, not a mistake.
  if (!interp) return ClueInterp::STALL;
  return interp;
}

}  // namespace hanabi::reactor0
