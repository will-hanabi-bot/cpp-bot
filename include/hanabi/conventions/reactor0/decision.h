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
#include "hanabi/conventions/reactor0/state_eval.h"

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
  DOUBLE_DISCARD,    // priority 3.2 / 3.4 / 4.8 — two discards
  REACTIVE_LOCK,     // a reactive whose receiver-side reading is a lock
  STABLE_PLAY,       // priority 3.1 / 4.1
  STABLE_DISCARD,    // priority 3.3 / 3.5 — stamps a CTD on Bob
  TRASH_REVEAL,      // priority 3.3's other arm — flags trash, stamps nothing
  STABLE_LOCK,       // priority 3.6 / 3.7 / 3.9
};

const char* shape_name(ClueShape s);

// How a clue reads, in the vocabulary above.
struct ClueReading {
  ClueShape shape = ClueShape::OTHER;
  Designation reacter_side;   // Bob's designation, for a reactive
  Designation receiver_side;  // Cathy's promised designation, for a reactive
  // The stable side, flattened. `stable_button` distinguishes the two ways a
  // STABLE_DISCARD arises, which several priority rungs name separately: a CTD
  // on an ordinary card, versus a CTP on an inverted card (a pitch). Kept as
  // three fields rather than a `Designation` because `stable_subject` is
  // already asserted on by test_clue_shape.cpp.
  int stable_subject = -1;
  CardStatus stable_button = CardStatus::NONE;
  Outcome stable_outcome = Outcome::NONE;
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

// --- The General Clue Evaluation List ------------------------------------

// Does playing `x` make some card in `player`'s hand playable? That is, is `x`
// the immediate predecessor on its suit of a card they hold? Follows the
// reversed-suit direction, because it asks `variants::connector_of` of the
// candidate rather than stepping `x` forward itself.
//
// Priority 1 and 2's tiebreak chains ask it about Bob's card; Decision phase 2
// rungs 2 and 3 ask it about Alice's own.
bool connects_to_hand(const Game& game, Identity x, int player,
                      int exclude_order = -1);

// Does `player` already hold a card they can safely press DISCARD on, judged
// from COMMON knowledge? The condition on rungs 3.3 and 4.2: telling Bob about
// a second dead card buys nothing if he can already throw one away.
//
// NOT the same as holding known trash. In an inverted variant Discard is a play
// ATTEMPT, so a card known to be a dead Orange 1 is trash and still has no safe
// discard button -- chucking it strikes. Only trash on a PLAIN suit qualifies.
bool has_safe_discard(const Game& game, int player);

// Priority 3's precondition: Bob is not locked, he has no safe play or discard,
// and his chop is worth spending a clue on -- meaning it is either ENDANGERED
// (`at_risk_chop`, the same test H1a uses) or PLAYABLE (`has_playable_chop`,
// N5's test). A chop that is neither earns nothing; "non-trash" alone was too
// weak, and spent a clue on a card duplicated in Cathy's hand (replay 1966745
// T5). Exported so the rung fixtures can pin it directly.
bool priority_3_applies(const Game& game);

// Priority 2's admissibility condition, and the rung-3.3 / 3.5 / 4.7 test: can
// the team afford to have `order` thrown away? True when it is basic trash, a
// same-hand-dupe, or a good card Alice can see a second copy of in some other
// hand (her own included, where "see" means she can pin it).
//
// This is what makes a separate "pointless double discard" filter unnecessary:
// a reactive that loses something real is simply never proposed.
bool discard_is_affordable(const Game& game, int holder, int order);

// The tiebreak quantity for rungs 3.8 and 4.8: how many identities strictly
// between the top of `order`'s stack and `order` itself are visible to Alice in
// NO hand. The more there are, the less likely the team ever plays the card, so
// the higher it sorts as something to throw away.
//
// DECISION_MAKING.md's worked example, empty stacks, Bob `r2 g3 r4 g4 b4` and
// Cathy `r3 r3 b5 g5 p5`: b4 -> 3, g4 -> 2, r4 -> 1.
int missing_connectors(const Game& game, int order);

// One candidate clue, analysed. Building this is the ONLY place a candidate is
// simulated: `Game::simulate` is the expensive call, and both entry points below
// read this struct rather than re-deriving from a hypo.
struct ClueCandidate {
  PerformAction perform;
  ClueAction action;
  ClueReading reading;
  ClueTier tier = ClueTier::LOW;
  bool is_h4 = false;
  // 1.99 * (# new useful cards touched) - (# new trash cards touched), the
  // spec's default tiebreak. "New" is `!prev.state.deck[o].clued`; trash is
  // basic trash from Alice's full visibility. The 1.99 is deliberate: two
  // useful plus one trash (2.98) beats one useful alone (1.99), but one useful
  // plus one trash (0.99) does not.
  double default_score = 0.0;
  // Priority 4.5's subject: orders in the target's hand that were ALREADY
  // clued, are unplayable, and whose identity this clue narrows from the
  // target's own point of view. A "fill-in" tells Bob more about a card he
  // cannot use yet, which at 8 tokens is a way to spend one without lying.
  std::vector<int> fill_ins;
};

// One `Game::simulate` per candidate — the same cost the deleted `eval_action`
// paid. Candidates whose interpretation is a MISTAKE are dropped here, matching
// the old scorer's -100 rejection: no rung may propose a clue the team cannot
// decode.
std::vector<ClueCandidate> analyse_clues(
    const Game& game,
    const std::vector<std::pair<PerformAction, Action>>& all_clues);

// The tier gate of DECISION_MAKING.md "Decision phase 1" items 1 and 2, lifted
// verbatim from the deleted `eval_action` so its boundaries do not move:
//   * Alice occupied  -> need HIGH   while pace() >= 3 && clue_tokens < 8
//   * otherwise       -> need MEDIUM while pace() >= 3 && clue_tokens <= 3
// Outside either window, or when the giver is not the POV seat, there is no
// gate. At 8 tokens neither window applies, which is what lets section 4 rank
// clues the gate would otherwise flatten.
bool clue_is_admissible(const Game& game, const ClueCandidate& c);

// Precedence step 1 — the best H4 clue, or nullopt.
//
// Deliberately does NOT apply section 4's floor. The floor exists so the section
// 4 branch always returns something at 8 tokens; applying it here would make an
// arbitrary clue outrank a pending reaction, which is the opposite of what the
// Precedence rule says.
std::optional<PerformAction> choose_h4_clue(const Game& game,
                                            const std::vector<ClueCandidate>& cands);

// Precedence step 3 — walk the General Clue Evaluation List and return the clue
// reactor0 wants to give, or nullopt to fall through to the play/discard phase.
//
// Returns nullopt on an empty candidate list, so the section 4 floor cannot fire
// when Alice cannot legally clue at all (0 tokens, or she is the pending
// receiver -- decide.cpp's `can_clue_now`). The floor is a guarantee about
// ranking, not a promise to conjure a clue that does not exist.
std::optional<PerformAction> choose_clue(const Game& game,
                                         const std::vector<ClueCandidate>& cands);

}  // namespace hanabi::reactor0
