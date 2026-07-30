#include "hanabi/conventions/reactor0/efficiency.h"

#include "hanabi/basics/state.h"

namespace hanabi::reactor0 {

double starting_required_efficiency(const Variant& variant, int num_players) {
  const int max_score = 5 * static_cast<int>(variant.suits.size());
  const int dealt = num_players * kHandSize[num_players];
  // Starting pace, matching State::pace() at turn 0:
  // score(0) + cards_left(total - dealt) + num_players - max_score.
  const int starting_pace =
      variant.total_cards() - dealt + num_players - max_score;
  // Every point of pace is a discard (one clue back), and every suit's
  // final rank returns a clue when played — half a clue in Clue Starved.
  const double regains =
      static_cast<double>(starting_pace) + static_cast<double>(variant.suits.size());
  const double clues = 8.0 + (variant.clue_starved ? 0.5 * regains : regains);
  if (clues <= 0.0) return 99.0;  // degenerate; treat as maximally hard.
  return static_cast<double>(max_score) / clues;
}

bool default_allow_reactive_locks(const Variant& variant, int num_players) {
  return starting_required_efficiency(variant, num_players) <= 1.42;
}

}  // namespace hanabi::reactor0
