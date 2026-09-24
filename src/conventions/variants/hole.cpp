#include "hanabi/conventions/variants/hole.h"

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player,
                                std::optional<int> except_order) {
  // The SHARED stacks, not our own belief. A partner's superposed play advanced
  // ours — we watched the card go in — and advanced nobody else's, so walking
  // from `game.state` would have each seat simulate a different game. That is
  // §1e's "assume none of the superposed cards were played", and it is why this
  // reads `shared_view` rather than the state itself.
  State hypo = game.state.shared_view();
  std::vector<int> hands;
  if (only_player) {
    hands.push_back(*only_player);
  } else {
    for (int p = 0; p < hypo.num_players; ++p) hands.push_back(p);
  }

  bool advanced = true;
  while (advanced) {
    advanced = false;
    for (int p : hands) {
      for (int o : hypo.hands[p]) {
        if (except_order && o == *except_order) continue;
        auto id = game.state.deck[o].id();
        if (!id || !hypo.is_playable(*id)) continue;
        // Queued means the team is already committed to it: a standing call, or
        // a card its holder can name from empathy alone.
        const bool called = game.meta[o].status == CardStatus::CALLED_TO_PLAY;
        const IdentitySet live = game.common.thoughts[o].possibilities();
        const bool empathy_playable =
            live.non_empty() &&
            live.forall([&hypo](Identity i) { return hypo.is_playable(i); });
        if (!called && !empathy_playable) continue;
        hypo = hypo.with_play(*id);
        advanced = true;
      }
    }
  }
  return hypo;
}


bool has_known_play(const Game& game, int player) {
  const State& s = game.state;
  for (int o : s.hands[player]) {
    const IdentitySet live = game.common.thoughts[o].possibilities();
    if (!live.non_empty()) continue;
    // A CALLED card qualifies while ONE good playable survives in it: the call
    // is the promise, and the rest of the set is the superposition it was given
    // under. An unstamped card has to be playable on every identity it could
    // still be, which is what `known` means without a call behind it.
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) {
      if (live.exists([&s](Identity i) { return s.is_playable(i); })) return true;
      continue;
    }
    if (live.forall([&s](Identity i) { return s.is_playable(i); })) return true;
  }
  return false;
}

bool reverse_reactive(const Game& prev, int giver, int target) {
  const State& s = prev.state;
  const int bob = s.next_player_index(giver);
  const int cathy = s.next_player_index(bob);
  // Fewer than three seats leaves nobody to react.
  if (cathy == giver) return false;
  if (target != bob) return false;
  return has_known_play(prev, bob) && !has_known_play(prev, cathy);
}

bool reverse_reactive(const Game& prev, const ClueAction& action) {
  return reverse_reactive(prev, action.giver, action.target);
}

}  // namespace hanabi::reactor::variants
