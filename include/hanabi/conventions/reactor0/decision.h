// Reactor0's rules-based decision layer (DECISION_MAKING.md).
//
// v7.0.0 implements decision phase 1: which clue to give, chosen by walking an
// ordered priority list rather than by scoring every candidate and taking an
// argmax. This header exports the classification vocabulary the list is written
// in, so each rule can be read against the spec line it implements.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"

namespace hanabi::reactor0 {

// What pressing a button on a card actually DOES, once the inverted-suit rule
// is applied. DECISION_MAKING.md's priorities are written in these terms:
// "a play refers to a pitch of a non-inverted suit and a chuck of an inverted
// suit, while a discard refers to a chuck of a non-inverted suit and a pitch of
// an inverted suit."
enum class Outcome : std::uint8_t {
  NONE,     // no designation
  PLAY,     // the card reaches its stack
  DISCARD,  // the card reaches the discard pile
  STRIKE,   // a misplay
};

// A card this clue designates, and what actioning it will do.
struct Designation {
  int order = -1;
  CardStatus button = CardStatus::NONE;  // which button the holder presses
  Outcome outcome = Outcome::NONE;
};

// The shapes DECISION_MAKING.md's priority list is defined over.
enum class ClueShape : std::uint8_t {
  OTHER,             // nothing the list has a rule for
  REACTIVE_PLAY,     // priority 1  — two designations, both play
  REACTIVE_DISCARD,  // priority 2  — one play and one discard
  DOUBLE_DISCARD,    // priority 2/3.6 — two discards
  REACTIVE_LOCK,     // a reactive whose receiver-side reading is a lock
  STABLE_PLAY,       // priority 3.1 / 4.1
  STABLE_DISCARD,    // priority 3.2 / 3.3 — stamps a CTD on Bob
  TRASH_REVEAL,      // priority 3.2's other arm — flags trash, stamps nothing
  STABLE_LOCK,       // priority 3.4 / 3.5 / 3.7
};

const char* shape_name(ClueShape s);

// How a clue reads, in the vocabulary above.
struct ClueReading {
  ClueShape shape = ClueShape::OTHER;
  Designation reacter_side;   // Bob's designation, for a reactive
  Designation receiver_side;  // Cathy's promised designation, for a reactive
  int stable_subject = -1;    // the designated order, for a stable clue
};

// What pressing `button` on `order` does, judged from the giver's full
// visibility. `s` must be the state the holder will act in.
Outcome outcome_of(const State& s, int order, CardStatus button);

// Read a simulated candidate clue. `hypo` is `game.simulate(action)`.
//
// Reactive designations come from the waiting connection, NOT from a walk over
// `hypo.meta`: rank Phase B and Phase C stamp only the reacter at clue time, so
// a stamp walk sees one card where the spec counts two. The receiver's promised
// order is recovered with `predicted_receiver_order`, which uses the same
// arithmetic the reaction will use a turn later.
ClueReading read_clue(const Game& game, const Game& hypo,
                      const ClueAction& action);

}  // namespace hanabi::reactor0
