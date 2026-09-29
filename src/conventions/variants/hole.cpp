#include "hanabi/conventions/variants/hole.h"

#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/game.h"

namespace hanabi::reactor::variants {

State stacks_after_queued_plays(const Game& game,
                                std::optional<int> only_player,
                                std::optional<int> except_order,
                                const std::vector<int>* base) {
  // Not our own belief. A partner's superposed play advanced ours — we watched
  // the card go in — and advanced nobody else's, so walking from `game.state`
  // would have each seat simulate a different game. The views the caller hands
  // us are the MINIMUM across the worlds their seats' hole cards leave open
  // (§1e, v16.24.0; before that, "assume none of the superposed cards were
  // played").
  //
  // Which view, exactly, is the caller's to say: the SHARED one by default, and
  // the pairwise one when the walk is deciding what a clue between two named
  // seats means (§1.3).
  State hypo = base ? game.state.with_stacks(*base) : game.state.shared_view();
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
      // The hand from `game`, never from `hypo`: `hypo` is reassigned below, and
      // iterating one of its own vectors across that reassignment read freed
      // memory (v18.0.0). A simulated play moves no card, so the hands are the
      // same.
      for (int o : game.state.hands[p]) {
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
  // Only what every seat computes ALIKE (v18.0.0): the card's clue-touch empathy
  // (`possible`, which every seat derives from the same public clues), judged on
  // the shared view. A card is a known play when every identity its touches still
  // allow is playable there.
  //
  // Call statuses, inferences and a seat's own stacks are all left out, because
  // each of them can differ between seats, and the reverse position decides how
  // every seat reads the next clue. Through v17 a standing call counted (v17.4.0),
  // and so did an inference that was all playable on the seat's belief. Over 500
  // self-play games the seats then read about one clue in ten as different kinds,
  // and four in ten of those disagreements involved a seat in the reverse position.
  // Read this way, 25/25 went from 26 to 31 of 300 (6 s endgame), and cards ever
  // read wrongly from 461 to 408 per 100 games.
  const State shared = s.shared_view();
  for (int o : s.hands[player]) {
    const IdentitySet& touched = game.common.thoughts[o].possible;
    if (touched.non_empty() &&
        touched.forall([&shared](Identity i) { return shared.is_playable(i); })) {
      return true;
    }
  }
  return false;
}

bool has_standing_play(const Game& game, int player) {
  // Any standing call counts, not just a card whose touches name it playable
  // (v18.2.0). A stable colour clue stamps a call by the nature of the clue, and
  // every seat stamps it alike. Replay 2013645 T11: yagami's T8 Blue had called
  // will-bot69's o12 as the b3, but its touches still allowed b1-b5, so under the
  // touch-only test will-bot69 held no known play. yagami's 4 to will-bot67 was
  // read as an ordinary reactive, and will-bot69 blind-played an r3 as the
  // reaction into a strike.
  if (has_known_play(game, player)) return true;
  for (int o : game.state.hands[player]) {
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) return true;
  }
  return false;
}

bool inverted_stable(const Game& prev, int giver, int target) {
  const State& s = prev.state;
  const int bob = s.next_player_index(giver);
  const int cathy = s.next_player_index(bob);
  if (cathy == giver || target != cathy) return false;
  if (reverse_reactive_position(prev, giver)) return true;
  return has_standing_play(prev, bob) && !has_standing_play(prev, cathy);
}

bool reverse_reactive_position(const Game& prev, int giver) {
  const State& s = prev.state;
  const int bob = s.next_player_index(giver);
  const int cathy = s.next_player_index(bob);
  // Fewer than three seats leaves nobody to react.
  if (cathy == giver) return false;
  return has_known_play(prev, bob) && !has_known_play(prev, cathy);
}

bool reverse_reactive(const Game& prev, int giver, int target) {
  if (!reverse_reactive_position(prev, giver)) return false;
  return target == prev.state.next_player_index(giver);
}

bool reverse_reactive(const Game& prev, const ClueAction& action) {
  return reverse_reactive(prev, action.giver, action.target);
}

}  // namespace hanabi::reactor::variants
