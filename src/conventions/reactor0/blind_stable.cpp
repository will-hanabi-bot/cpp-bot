#include "hanabi/conventions/reactor0/blind_stable.h"

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

bool clue_kind_is_blind(const Variant& variant, ClueKind kind) {
  return kind == ClueKind::COLOUR ? variant.colour_clues_touch_nothing
                                  : variant.rank_clues_touch_nothing;
}

BlindCall blind_call(const Variant& variant, ClueKind kind, int value) {
  if (kind == ClueKind::RANK) {
    // 1-4 name their own slot; 5 has no slot to name and locks.
    if (value >= 1 && value <= 4) {
      return BlindCall{/*lock=*/false, CardStatus::CALLED_TO_DISCARD, value};
    }
    return BlindCall{/*lock=*/true};
  }
  const auto& names = variant.clue_colour_names;
  if (value < 0 || value >= static_cast<int>(names.size())) {
    return BlindCall{/*lock=*/true};
  }
  const std::string& name = names[value];
  if (name == "Red") return {false, CardStatus::CALLED_TO_PLAY, 1};
  if (name == "Yellow") return {false, CardStatus::CALLED_TO_PLAY, 2};
  if (name == "Green") return {false, CardStatus::CALLED_TO_PLAY, 3};
  if (name == "Blue") return {false, CardStatus::CALLED_TO_PLAY, 4};
  if (name == "Purple") return {false, CardStatus::CALLED_TO_PLAY, 5};
  // Teal in the 6-suit members, and anything a future Blind variant adds.
  return BlindCall{/*lock=*/true};
}

std::optional<ClueInterp> blind_stable(Game& game, const ClueAction& action) {
  const State& s = game.state;
  const BlindCall call = blind_call(*s.variant, action.clue.kind, action.clue.value);
  const auto& hand = s.hands[action.target];

  if (call.lock) {
    // Locking a hand that already reads locked says nothing, and the giver can
    // see that as well as the receiver -- so it is a MISTAKE rather than a stall,
    // and is never offered. Same guard `reactor::ref_discard`'s lock arm carries.
    if (game.common.thinks_locked(game, action.target)) return std::nullopt;
    const int turn = s.turn_count;
    const int giver = action.giver;
    for (int o : hand) {
      game.with_meta(o, [turn, giver](ConvData& m) {
        m.status = CardStatus::CHOP_MOVED;
        m.by = giver;
        m = m.reason(turn);
      });
    }
    return ClueInterp::LOCK;
  }

  // The slot may simply not be there: a 4-card hand cannot answer Purple, and a
  // hand worn down at the end of the deck cannot answer much. Nothing to stamp,
  // so the clue is a stall -- which is a real thing to spend a token on.
  if (call.slot > static_cast<int>(hand.size())) return ClueInterp::STALL;
  const int order = hand[call.slot - 1];
  const bool pitch = call.button == CardStatus::CALLED_TO_PLAY;

  // Vet on COMMON knowledge, so giver and receiver reach the same verdict about
  // whether a call was made at all. Both predicates are EXISTENTIAL
  // (interpret_reaction.h): the button is acceptable if ANY reading the slot
  // still admits makes it so.
  const IdentitySet cand = effective_possible_for(game, order);
  const bool acceptable =
      pitch ? slot_is_pitchable(s, cand) : slot_is_chuckable(s, cand);
  if (!acceptable) return ClueInterp::STALL;

  // POV-invariant abort, the same shape `synesthesia_stable` uses:
  // `state.deck[o].id()` is nullopt from the holder's own seat, so this fires
  // only for the seats that can SEE the card. If what they see makes the named
  // action bad, the clue is a MISTAKE and the giver never offers it. It must not
  // instead degrade to a stall -- Bob cannot see what they see, so he would act
  // on the call while they had quietly agreed it was cancelled.
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
