#include "hanabi/conventions/variants/hole.h"

#include <vector>

#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player,
                                std::optional<int> except_order) {
  State hypo = game.state;
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

}  // namespace hanabi::reactor::variants
