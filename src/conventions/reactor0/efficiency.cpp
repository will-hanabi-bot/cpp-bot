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
  // Four families default OFF whatever the formula says (v13.5.0). All four
  // score comfortably under the threshold and so used to default ON: a plain
  // 3-suit variant is 15/14 ~ 1.07, Alternating Clues (5 Suits) 25/26 ~ 0.96,
  // Synesthesia & Black (6 Suits) 30/27 ~ 1.11.
  //
  // Only the DEFAULT moves. `/rlocks on` still turns them back on per process,
  // and an in-flight reaction keeps whatever was bound into `ReactorWC::rlocks`
  // at clue time.

  // Three suits: a lock commits five cards out of a fifteen-card score. There is
  // not enough hand to spend on one.
  if (variant.suits.size() == 3) return false;

  // Odds and Evens: two rank clues, so the even bucket carries the whole rank
  // channel and a lock spends a scarcer clue than the count suggests.
  if (variant.odds_and_evens) return false;

  // Alternating Clues and Synesthesia: no stable clues at all, so every clue is
  // reactive and a lock costs the only channel there is.
  //
  // This pair is today exactly `variants::uses_target_parity`
  // (`conventions/variants/predicates.cpp`), and the coincidence is not the
  // reason -- that predicate answers "where does a clue's parity come from",
  // which is a different question from "is a lock worth its cost here". Named as
  // the two flags so the two rules can move apart without surprising anyone.
  if (variant.alternating_clues || variant.synesthesia) return false;

  return starting_required_efficiency(variant, num_players) <= 1.42;
}

}  // namespace hanabi::reactor0
