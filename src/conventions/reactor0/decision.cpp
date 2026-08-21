#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"

#include <algorithm>
#include <initializer_list>
#include <functional>
#include <utility>
#include <variant>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/reactor0/facts.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/reversed.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

namespace {
bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}
}  // namespace

const char* shape_name(ClueShape s) {
  switch (s) {
    case ClueShape::REACTIVE_PLAY: return "reactive_play";
    case ClueShape::REACTIVE_DISCARD: return "reactive_discard";
    case ClueShape::DOUBLE_DISCARD: return "double_discard";
    case ClueShape::REACTIVE_LOCK: return "reactive_lock";
    case ClueShape::STABLE_PLAY: return "stable_play";
    case ClueShape::STABLE_DISCARD: return "stable_discard";
    case ClueShape::TRASH_REVEAL: return "trash_reveal";
    case ClueShape::STABLE_LOCK: return "stable_lock";
    case ClueShape::OTHER: break;
  }
  return "other";
}

Outcome outcome_of(const State& s, int order, CardStatus button) {
  auto id = s.deck[order].id();
  // Our own card: the giver never designates one, and cannot judge it anyway.
  if (!id) return Outcome::NONE;
  const bool inverted = variants::is_inverted_id(s, *id);
  switch (button) {
    case CardStatus::CALLED_TO_PLAY:
      // Press Play. On an inverted suit that is a PITCH: the card goes to the
      // discard pile and regains a clue. It can never strike, whatever the id.
      if (inverted) return Outcome::DISCARD;
      return s.is_playable(*id) ? Outcome::PLAY : Outcome::STRIKE;
    case CardStatus::CALLED_TO_DISCARD:
      // Press Discard. On an inverted suit that is a CHUCK: a play attempt onto
      // the stack, which advances it or strikes.
      if (inverted) return s.is_playable(*id) ? Outcome::PLAY : Outcome::STRIKE;
      return Outcome::DISCARD;
    default:
      return Outcome::NONE;
  }
}

namespace {

// The receiver's button, given the clue kind and the button the reacter was
// told to press. Fixed by the resolution parity table in react_play /
// react_discard (reactor0/interpret_reaction.cpp:82, :88-91, :131, :136-139).
// The receiver is not stamped at clue time on the phases that matter here, so
// the reading has to come from the parity rather than from the meta.
//
// Keyed on the PARITY, not the clue kind. The even bucket is normally the rank
// clue, but Odds and Evens swaps the two and `/set` can move an individual
// clue, so reading `ClueKind` here would invert every reactive clue's shape in
// those variants -- and the shape is what rungs 1 and 2 of the General Clue
// Evaluation List select on.
CardStatus receiver_button(bool even_parity, CardStatus reacter_button) {
  const bool reacter_plays = reacter_button == CardStatus::CALLED_TO_PLAY;
  if (even_parity) {
    return reacter_plays ? CardStatus::CALLED_TO_PLAY
                         : CardStatus::CALLED_TO_DISCARD;
  }
  return reacter_plays ? CardStatus::CALLED_TO_DISCARD
                       : CardStatus::CALLED_TO_PLAY;
}

ClueShape shape_of(Outcome reacter, Outcome receiver) {
  const bool a = reacter == Outcome::PLAY;
  const bool b = receiver == Outcome::PLAY;
  if (a && b) return ClueShape::REACTIVE_PLAY;
  if (a != b) return ClueShape::REACTIVE_DISCARD;
  return ClueShape::DOUBLE_DISCARD;
}

// The stable side: which order this clue designated, and how.
ClueReading read_stable(const Game& game, const Game& hypo,
                        const ClueAction& action, ClueInterp interp) {
  ClueReading r;
  const int target = action.target;

  if (interp == ClueInterp::LOCK) {
    r.shape = ClueShape::STABLE_LOCK;
    return r;
  }

  // A newly stamped CTP/CTD in the target's hand is the designation. Only
  // ADDITIONS count: enforce_call_invariants runs straight after interpretation
  // (decide.cpp:65) and erases older calls, so a raw before/after diff would
  // also report cards this clue un-designated.
  for (int o : hypo.state.hands[target]) {
    const CardStatus before = game.meta[o].status;
    const CardStatus after = hypo.meta[o].status;
    if (after == before) continue;
    if (after != CardStatus::CALLED_TO_PLAY &&
        after != CardStatus::CALLED_TO_DISCARD) {
      continue;
    }
    r.stable_subject = o;
    r.stable_button = after;
    const Outcome out = outcome_of(game.state, o, after);
    r.stable_outcome = out;
    r.shape = out == Outcome::PLAY ? ClueShape::STABLE_PLAY
                                   : ClueShape::STABLE_DISCARD;
    return r;
  }

  // A PLAY REVEAL stamps no status either: it narrows a previously-clued card
  // until empathy alone makes it obviously playable, and leaves the physical
  // action to that (interpret_clue.cpp's branch 1). It is still a play clue --
  // it is the whole point of the clue -- so the list has to be able to see one.
  //
  // Replay 1966633 T5: Light-Pink-Fives & Orange, where the 5s take every rank
  // clue but no colour clue. Blue to Bob pinned his previously-clued card to
  // exactly Blue 1, playable on an empty blue stack. Reading no stamp, this
  // classified the clue OTHER, no rung could select it, `choose_clue` declined
  // outright, and a locked Alice bombed her way out of the turn instead.
  if (interp == ClueInterp::REVEAL) {
    auto [_blind, playables] = hanabi::playables_result(game, hypo);
    for (int o : playables) {
      if (!contains(hypo.state.hands[target], o)) continue;
      r.shape = ClueShape::STABLE_PLAY;
      r.stable_subject = o;
      r.stable_outcome = Outcome::PLAY;
      return r;
    }
  }

  // A trash reveal stamps no status at all - it sets meta.trash
  // (reactor0/interpret_clue.cpp:558). That one-field diff isolates it from the
  // other REVEAL branches, none of which flag a newly touched card.
  for (int o : hypo.state.hands[target]) {
    if (!game.meta[o].trash && hypo.meta[o].trash) {
      r.shape = ClueShape::TRASH_REVEAL;
      r.stable_subject = o;
      return r;
    }
  }
  return r;
}

}  // namespace

ClueReading read_clue(const Game& game, const Game& hypo,
                      const ClueAction& action) {
  ClueReading r;
  auto move = hypo.last_move();
  if (!move || !std::holds_alternative<ClueInterp>(*move)) return r;
  const ClueInterp interp = std::get<ClueInterp>(*move);
  // A MISTAKE has no shape. Drop it before anything reads a stamp: the post-elim
  // demotion (decide.cpp:228-230) leaves real-looking CTP stamps on a hypo whose
  // interp is MISTAKE, so classifying stamps first would call it a play clue.
  if (interp == ClueInterp::MISTAKE) return r;

  const State& s = game.state;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (action.target == bob) return read_stable(game, hypo, action, interp);

  // --- reactive ---------------------------------------------------------
  if (!wc_is_fresh(game, hypo, alice, action.target, bob)) return r;
  const ReactorWC& wc = hypo.waiting.front();
  const CardStatus reacter_status = hypo.meta[wc.react_order].status;

  if (predicts_reactive_lock(hypo)) {
    r.shape = ClueShape::REACTIVE_LOCK;
    r.reacter_side = {wc.react_order, reacter_status,
                      outcome_of(s, wc.react_order, reacter_status)};
    return r;
  }

  auto receive_order = predicted_receiver_order(hypo);
  if (!receive_order) return r;
  if (reacter_status != CardStatus::CALLED_TO_PLAY &&
      reacter_status != CardStatus::CALLED_TO_DISCARD) {
    return r;
  }
  r.reacter_side = {wc.react_order, reacter_status,
                    outcome_of(s, wc.react_order, reacter_status)};

  // Bob acts FIRST, so the receiver's card is judged against the stacks Bob
  // leaves behind. Without this a finesse reads as a strike - its whole point is
  // that the receiver's card is one away until the blind play lands - and a
  // chained double play (r1 then r2) reads as play-then-strike.
  State after_bob = s;
  if (r.reacter_side.outcome == Outcome::PLAY) {
    if (auto id = s.deck[wc.react_order].id()) after_bob = s.with_play(*id);
  }
  const CardStatus rb = receiver_button(
      reactive_assignment(*s.variant, game.reactive_overrides, action.clue.kind,
                          action.clue.value)
          .even,
      reacter_status);
  r.receiver_side = {*receive_order, rb,
                     outcome_of(after_bob, *receive_order, rb)};
  r.shape = shape_of(r.reacter_side.outcome, r.receiver_side.outcome);
  return r;
}



// =========================================================================
// The General Clue Evaluation List (DECISION_MAKING.md "Decision phase 1").
//
// The list is an ordered set of RUNGS. Alice does the first thing that applies,
// and every rung carries its own tiebreak chain; the chain is walked left to
// right, and the first term that separates the survivors wins. The default
// tiebreak sits at the foot of every chain, and "first available" beneath that.
//
// The point of the shape is that the reason a clue was given is a rule you can
// point at, which the argmax this replaces could not offer.
// =========================================================================

// "Card X connects to card Y": Y is X's immediate successor on X's suit, so
// playing X makes Y playable. Asked as "is X the connector of Y", which reuses
// `variants::connector_of` and so follows the reversed-suit direction for free.
bool connects_to_hand_orders(const State& s, Identity x,
                             const std::vector<int>& hand, int exclude_order) {
  for (int o : hand) {
    if (o == exclude_order) continue;
    auto y = s.deck[o].id();
    if (!y) continue;
    auto conn = variants::connector_of(s, *y);
    if (conn && *conn == x) return true;
  }
  return false;
}

bool connects_to_hand(const Game& game, Identity x, int player,
                      int exclude_order) {
  return connects_to_hand_orders(game.state, x, game.state.hands[player],
                                 exclude_order);
}

namespace {

// --- small facts the rungs are written in --------------------------------

int alice_of(const Game& g) { return g.state.our_player_index; }
int bob_of(const Game& g) { return g.state.next_player_index(alice_of(g)); }
int cathy_of(const Game& g) { return g.state.next_player_index(bob_of(g)); }
bool has_cathy(const Game& g) { return cathy_of(g) != alice_of(g); }

// `variants::is_first_or_second_rank` answers the pair; the priority chains
// separate "critical 1" from "critical 2", so they compare the direction rank
// itself.
bool is_rank_in_direction(const State& s, Identity id, int n) {
  return variants::direction_rank(s, id) == n;
}

bool is_critical_rank(const State& s, std::optional<Identity> id, int n) {
  return id && s.is_critical(*id) && is_rank_in_direction(s, *id, n);
}

std::optional<Identity> id_of(const State& s, int order) {
  if (order < 0) return std::nullopt;
  return s.deck[order].id();
}

bool connects_to_a_card_in(const State& s, Identity x, const std::vector<int>& hand,
                           int exclude_order) {
  return connects_to_hand_orders(s, x, hand, exclude_order);
}

// The same question against Alice's OWN hand, judged from her inference rather
// than from the deck: "connects to another card in Alice's hand (that Alice
// knows, not necessarily globally known)".
bool connects_to_alices_own(const Game& g, Identity x) {
  const State& s = g.state;
  for (int o : s.hands[alice_of(g)]) {
    auto y = g.me().thoughts[o].id(/*infer=*/true);
    if (!y) continue;
    auto conn = variants::connector_of(s, *y);
    if (conn && *conn == x) return true;
  }
  return false;
}

// Does Alice see a copy of `id` in any hand other than `holder`'s — including
// her own, where "see" means she can pin the card to that identity?
bool dupe_visible_elsewhere(const Game& g, int holder, int order, Identity id) {
  const State& s = g.state;
  for (int p = 0; p < s.num_players; ++p) {
    if (p == holder) continue;
    for (int o : s.hands[p]) {
      if (o == order) continue;
      if (p == alice_of(g)) {
        auto inferred = g.me().thoughts[o].id(/*infer=*/true);
        if (inferred && *inferred == id) return true;
      } else if (auto seen = s.deck[o].id(); seen && *seen == id) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

// The admissibility test priority 2 applies to a card it is about to have
// thrown away: trash, a same-hand-dupe, or a good card Alice can see a second
// copy of somewhere else. This is what makes a separate "pointless double
// discard" filter unnecessary — a discard that loses something real is simply
// never proposed.
bool discard_is_affordable(const Game& g, int holder, int order) {
  auto id = id_of(g.state, order);
  if (!id) return false;
  if (g.state.is_basic_trash(*id)) return true;
  if (has_same_hand_dupe(g.state, holder, order, *id)) return true;
  return dupe_visible_elsewhere(g, holder, order, *id);
}

// Missing connectors of X: identities strictly between the top of X's stack and
// X that Alice can see in NO hand. The spec's worked example, no stacks, Bob
// `r2 g3 r4 g4 b4` and Cathy `r3 r3 b5 g5 p5`: b4 -> 3, g4 -> 2, r4 -> 1.
int missing_connectors(const Game& g, int order) {
  const State& s = g.state;
  auto id = id_of(s, order);
  if (!id) return 0;
  int missing = 0;
  for (Identity step = *id;;) {
    auto prev = variants::connector_of(s, step);
    if (!prev) break;
    // Stop only once the walk drops BELOW the stack top — an identity that is
    // already played is not "between" anything. The currently playable card is
    // itself strictly between the stack top and X, so it counts.
    if (s.playable_away(*prev) < 0) break;
    bool seen = false;
    for (int p = 0; p < s.num_players && !seen; ++p) {
      for (int o : s.hands[p]) {
        if (o == order) continue;
        if (auto oid = s.deck[o].id(); oid && *oid == *prev) { seen = true; break; }
      }
    }
    if (!seen) ++missing;
    step = *prev;
  }
  return missing;
}

namespace {



// Does `player` hold nothing they can safely act on? The three safe actions are
// an obvious play, known trash and a standing CTD, and `thinks_trash` covers the
// last two (player_game.cpp:115-132) — the same reading H1a uses.
bool has_no_safe_action(const Game& g, int player) {
  return g.common.obvious_playables(g, player).empty() &&
         g.common.thinks_trash(g, player).empty();
}


// Cathy's "relief", which is what relaxes a `>= N clues**` threshold by one:
// she has a safe discard, or her chop is something she can afford to lose.
bool cathy_has_relief(const Game& g) {
  if (!has_cathy(g)) return true;
  const int cathy = cathy_of(g);
  if (!g.common.thinks_trash(g, cathy).empty()) return true;
  auto chop = g.chop(cathy);
  if (!chop) return false;
  return discard_is_affordable(g, cathy, *chop);
}

// `>= N clues**` — N tokens, or N-1 when Cathy has relief.
bool clues_at_least(const Game& g, int n) {
  const int need = cathy_has_relief(g) ? n - 1 : n;
  return g.state.clue_tokens >= need;
}

// A reading that predicts a misplay is never proposed, with one exception the
// spec makes explicit: rung 4.7 allows a strike, because at 8 tokens with
// nothing else left a strike beats burning the turn blind.
bool predicts_a_strike(const ClueReading& r) {
  return r.reacter_side.outcome == Outcome::STRIKE ||
         r.receiver_side.outcome == Outcome::STRIKE ||
         r.stable_outcome == Outcome::STRIKE;
}

bool is_stable_to_bob(const Game& g, const ClueCandidate& c) {
  return c.action.target == bob_of(g);
}

// The card a reactive throws away, on whichever side is doing the throwing.
// A DOUBLE_DISCARD has two; both are returned so a rung can require that the
// team can afford EITHER loss, which is the conservative reading of the spec's
// singular "the discarded card".
std::vector<std::pair<int, int>> discarded_sides(const Game& g,
                                                 const ClueReading& r) {
  std::vector<std::pair<int, int>> out;  // (holder, order)
  const int bob = bob_of(g);
  if (r.reacter_side.outcome == Outcome::DISCARD) {
    out.emplace_back(bob, r.reacter_side.order);
  }
  if (r.receiver_side.outcome == Outcome::DISCARD) {
    out.emplace_back(cathy_of(g), r.receiver_side.order);
  }
  return out;
}

}  // namespace

// A reactive must not call TWO COPIES of the same card to play.
//
// The reacter acts first, so their copy stacks the identity and the receiver's
// is trash by the time they act -- but the receiver still reads their card as
// "the playable one", which by then is the NEXT rank, and bombs. Replay 1967363
// T1: both halves of a double play were an Orange 1. will-bot67 chucked theirs
// onto the stack and yagami_black chucked theirs for a strike, "thinking it is
// orange 2".
//
// Giver-side only, and a candidate FILTER rather than an interpretation rule.
// It reads the giver's sight of two hands, which no other seat has; making it a
// reading would have observers decode the clue differently from the giver and
// break POV invariance (CONVENTION.md 1g). Dropping the candidate simply means
// the clue is never offered.
//
// Variant-independent: the hazard is the double-play SHAPE, not the inverted
// suit. A vanilla rank reactive fails the same way.
//
// PLAYS only. Two cards called to be DISCARDED that happen to match is not a
// bomb, and throwing a spare copy is often right.
bool calls_two_copies_to_play(const Game& game, const Game& hypo) {
  if (hypo.waiting.empty()) return false;  // stable clues call one card
  const ReactorWC& wc = hypo.waiting.front();
  const State& s = game.state;

  // The card this clue newly called, and whether the BUTTON it was handed
  // would play it: CTP plays a plain card and throws an inverted one, CTD the
  // reverse. Own cards have no deck id, so they are skipped -- the giver can
  // only veto on what they can see.
  auto called_to_play = [&](int player) -> std::optional<std::pair<Identity, int>> {
    if (player < 0 || player >= static_cast<int>(s.hands.size())) return std::nullopt;
    for (int o : s.hands[player]) {
      const CardStatus now = hypo.meta[o].status;
      if (now == game.meta[o].status) continue;  // not newly stamped
      auto id = s.deck[o].id();
      if (!id) continue;
      const bool plays =
          now == CardStatus::CALLED_TO_DISCARD
              ? variants::discard_advances_stack(s, id)
              : (now == CardStatus::CALLED_TO_PLAY &&
                 !variants::is_inverted_id(s, *id) && s.is_playable(*id));
      if (plays) return std::make_pair(*id, o);
    }
    return std::nullopt;
  };

  auto react_call = called_to_play(wc.reacter);
  auto recv_call = called_to_play(wc.receiver);
  if (!react_call || !recv_call) return false;
  if (react_call->first != recv_call->first) return false;
  // Unless the receiver already knows exactly what they hold -- then they
  // cannot be fooled into pressing the button on a copy that has just died.
  return game.common.thoughts[recv_call->second].possibilities().length() != 1;
}

std::vector<ClueCandidate> analyse_clues(
    const Game& game,
    const std::vector<std::pair<PerformAction, Action>>& all_clues) {
  hanabi::instr::ScopedTimer st("reactor0.analyse_clues");
  const State& s = game.state;
  std::vector<ClueCandidate> out;
  out.reserve(all_clues.size());
  for (const auto& [perform, action] : all_clues) {
    if (!std::holds_alternative<ClueAction>(action)) continue;
    const auto& ca = std::get<ClueAction>(action);
    const Game hypo = game.simulate(action);
    auto move = hypo.last_move();
    if (move && std::holds_alternative<ClueInterp>(*move) &&
        std::get<ClueInterp>(*move) == ClueInterp::MISTAKE) {
      continue;  // undecodable: no rung may propose it
    }
    if (calls_two_copies_to_play(game, hypo)) continue;
    ClueCandidate c{perform, ca, read_clue(game, hypo, ca),
                    clue_tier(game, hypo, ca), clue_is_h4(game, hypo, ca), 0.0};
    int useful = 0, trash = 0;
    for (int o : ca.list_) {
      if (s.deck[o].clued) continue;  // not a NEW touch
      auto id = s.deck[o].id();
      if (id && s.is_basic_trash(*id)) {
        ++trash;
      } else {
        ++useful;
      }
    }
    c.default_score = 1.99 * useful - trash;

    // Fill-ins, for rung 4.5. "Narrows" is judged from the TARGET's own view --
    // he is the one who learns something -- and counts both positive and
    // negative information, since a clue that misses a card tells him about it
    // too. Playable cards are excluded: filling in something he can already use
    // is not a stall, and the rungs above have first refusal on it anyway.
    auto set_size = [](const Thought& t) {
      const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
      int n = 0;
      for (Identity i : set) {
        (void)i;
        ++n;
      }
      return n;
    };
    for (int o : s.hands[ca.target]) {
      if (!s.deck[o].clued) continue;  // must ALREADY be clued
      auto id = s.deck[o].id();
      if (!id || s.is_playable(*id)) continue;
      const int before = set_size(game.players[ca.target].thoughts[o]);
      const int after = set_size(hypo.players[ca.target].thoughts[o]);
      if (after < before) c.fill_ins.push_back(o);
    }
    out.push_back(std::move(c));
  }
  return out;
}

bool clue_is_admissible(const Game& game, const ClueCandidate& c) {
  const State& s = game.state;
  if (c.action.giver != s.our_player_index) return true;  // fails open
  // The gate runs at any pace above zero. It was `pace() >= 3` through v7.3.0,
  // inherited from reactor's low-clue-count gate, and that left a hole: at
  // replay 1966119 T5 will-bot69 was occupied by a receiver-CTP, pace was 2, so
  // the gate stood down and a LOW reactive discard was admissible. Low pace is
  // if anything a worse time to spend a token on a LOW clue, not a better one.
  if (s.pace() < 1) return true;
  const bool occupied = requires_high_tier(game);
  // 2a carries an extra condition 1a does not: an UNOCCUPIED Alice is exempt
  // while LOCKED. She has no chop to discard, so cluing is the only thing she
  // can do that is not burning a card -- the same reasoning that exempts 8 clue
  // tokens, where a discard is illegal. Without it the gate can empty the
  // candidate set before section 4 is reached, and section 4 is precisely the
  // branch that promises to return a clue when she is out of alternatives
  // (replay 1966653 T17: pace 1, 2 tokens, seven LOW candidates, all rejected,
  // and a locked Alice bombed a chop-moved card).
  //
  // An OCCUPIED Alice keeps 1a as written: she holds a call she can action, so
  // she is not out of alternatives.
  if (!occupied && game.common.thinks_locked(game, s.our_player_index)) {
    return true;
  }
  const bool in_window = occupied ? s.clue_tokens < 8 : s.clue_tokens <= 3;
  if (!in_window) return true;
  return c.tier >= (occupied ? ClueTier::HIGH : ClueTier::MEDIUM);
}

// Does `player` already hold a card they can safely press DISCARD on, judged
// from COMMON knowledge -- the shared view Alice and Bob both have?
//
// NOT the same as holding known trash, and that difference is the whole reason
// this exists rather than reusing `thinks_trash`. In an inverted variant
// Discard is a play ATTEMPT, so a card the holder knows is a dead Orange 1 is
// known trash and yet has no safe discard button at all: chucking it strikes.
// Only a card every one of whose readings is trash on a PLAIN suit can simply
// be thrown away.
bool has_safe_discard(const Game& g, int player) {
  const State& s = g.state;
  for (int o : s.hands[player]) {
    const Thought& t = g.common.thoughts[o];
    const IdentitySet& set = t.inferred.non_empty() ? t.inferred : t.possible;
    if (set.is_empty()) continue;
    bool safe = true;
    for (Identity i : set) {
      if (!s.is_basic_trash(i) || variants::is_inverted_id(s, i)) {
        safe = false;
        break;
      }
    }
    if (safe) return true;
  }
  return false;
}

namespace {

// A tiebreak chain: each term partitions the survivors, and the first term that
// actually separates them decides. A term that is true of everything (or of
// nothing) is skipped, which is what lets the chains be written as a plain list
// of questions rather than as nested conditionals.
using Term = std::function<bool(const ClueCandidate&)>;

const ClueCandidate* settle(const Game& g, std::vector<const ClueCandidate*> pool,
                            const std::vector<Term>& chain) {
  if (pool.empty()) return nullptr;
  for (const Term& t : chain) {
    std::vector<const ClueCandidate*> kept;
    for (const ClueCandidate* c : pool) {
      if (t(*c)) kept.push_back(c);
    }
    if (!kept.empty() && kept.size() < pool.size()) pool = std::move(kept);
    if (pool.size() == 1) return pool.front();
  }
  // Default tiebreak, then first available.
  const ClueCandidate* best = pool.front();
  for (const ClueCandidate* c : pool) {
    if (c->default_score > best->default_score) best = c;
  }
  (void)g;
  return best;
}

// Bob's card, for the priority-1 and priority-2 chains. Both are written about
// "Bob's card" meaning the reacter side of a reactive.
std::optional<Identity> bobs_card(const Game& g, const ClueCandidate& c) {
  return id_of(g.state, c.reading.reacter_side.order);
}

// Tiebreaks 1-5 of priority 1, and — guarded by "if Bob plays" — 1-5 of
// priority 2. Shared because the spec writes them out identically.
std::vector<Term> bob_card_chain(const Game& g, bool require_bob_plays) {
  auto gate = [&g, require_bob_plays](const ClueCandidate& c) {
    return !require_bob_plays || c.reading.reacter_side.outcome == Outcome::PLAY;
  };
  return {
      // 1. connects to another card in Bob's or Cathy's hand
      [&g, gate](const ClueCandidate& c) {
        if (!gate(c)) return false;
        auto x = bobs_card(g, c);
        if (!x) return false;
        const int self = c.reading.reacter_side.order;
        return connects_to_a_card_in(g.state, *x, g.state.hands[bob_of(g)], self) ||
               (has_cathy(g) && connects_to_a_card_in(g.state, *x,
                                                     g.state.hands[cathy_of(g)], self));
      },
      // 2. connects to another card in Alice's hand that Alice knows
      [&g, gate](const ClueCandidate& c) {
        if (!gate(c)) return false;
        auto x = bobs_card(g, c);
        return x && connects_to_alices_own(g, *x);
      },
      // 3. a critical 1 (5 reversed)
      [&g, gate](const ClueCandidate& c) {
        return gate(c) && is_critical_rank(g.state, bobs_card(g, c), 1);
      },
      // 4. a critical 2 (4 reversed)
      [&g, gate](const ClueCandidate& c) {
        return gate(c) && is_critical_rank(g.state, bobs_card(g, c), 2);
      },
      // 5. a clue-regain card (5 normally, 1 reversed)
      [&g, gate](const ClueCandidate& c) {
        if (!gate(c)) return false;
        auto x = bobs_card(g, c);
        return x && variants::is_clue_regain_rank(g.state, *x);
      },
  };
}

}  // namespace

namespace {

using Pool = std::vector<const ClueCandidate*>;

Pool select(const std::vector<ClueCandidate>& cands,
            const std::function<bool(const ClueCandidate&)>& pred) {
  Pool out;
  for (const ClueCandidate& c : cands) {
    if (predicts_a_strike(c.reading)) continue;  // rung 4.7 is the exception
    if (pred(c)) out.push_back(&c);
  }
  return out;
}

// --- priority 1: a reactive play clue ------------------------------------
const ClueCandidate* rung_1(const Game& g, const std::vector<ClueCandidate>& cs) {
  Pool p = select(cs, [](const ClueCandidate& c) {
    return c.reading.shape == ClueShape::REACTIVE_PLAY;
  });
  return settle(g, std::move(p), bob_card_chain(g, /*require_bob_plays=*/false));
}

// --- priority 2: a reactive discard clue Alice can afford ----------------
const ClueCandidate* rung_2(const Game& g, const std::vector<ClueCandidate>& cs) {
  Pool p = select(cs, [&g](const ClueCandidate& c) {
    if (c.reading.shape != ClueShape::REACTIVE_DISCARD) return false;
    // BOB is the one who plays, and CATHY is the one who discards. A reactive
    // discard the other way round -- Cathy playing, Bob throwing something away
    // -- is not priority 2 and falls to the rungs below. The tiebreaks are
    // written entirely about Bob's played card, which is what makes the
    // direction load-bearing rather than cosmetic.
    if (c.reading.reacter_side.outcome != Outcome::PLAY) return false;
    if (c.reading.receiver_side.outcome != Outcome::DISCARD) return false;
    // Cathy's discarded card is the one that has to be affordable.
    return discard_is_affordable(g, cathy_of(g), c.reading.receiver_side.order);
  });
  // `require_bob_plays` is now vacuous here -- the rung already demands it --
  // but it is kept so the chain still reads as the spec writes it, and so
  // priority 1 and priority 2 keep sharing one definition.
  std::vector<Term> chain = bob_card_chain(g, /*require_bob_plays=*/true);
  // 6. Cathy's discarded card is a same-hand-dupe
  chain.push_back([&g](const ClueCandidate& c) {
    for (const auto& side : discarded_sides(g, c.reading)) {
      auto id = id_of(g.state, side.second);
      if (id && has_same_hand_dupe(g.state, side.first, side.second, *id)) return true;
    }
    return false;
  });
  // 7. Cathy's discarded card is trash
  chain.push_back([&g](const ClueCandidate& c) {
    for (const auto& side : discarded_sides(g, c.reading)) {
      auto id = id_of(g.state, side.second);
      if (id && g.state.is_basic_trash(*id)) return true;
    }
    return false;
  });
  return settle(g, std::move(p), chain);
}

// --- priority 3's rungs, shared with priority 4 --------------------------

// 3.1 / 4.1 -- a stable play clue to Bob.
Pool pool_stable_play(const Game& g, const std::vector<ClueCandidate>& cs) {
  return select(cs, [&g](const ClueCandidate& c) {
    return is_stable_to_bob(g, c) && c.reading.shape == ClueShape::STABLE_PLAY;
  });
}

// 3.2 / 4.2 -- a stable discard or trash reveal to Bob aimed at a card the team
// does not want: a CTD on trash or a same-hand-dupe, or a CTP on an inverted
// trash card (which is a pitch, not a play).
Pool pool_stable_ditch_trash(const Game& g, const std::vector<ClueCandidate>& cs) {
  return select(cs, [&g](const ClueCandidate& c) {
    if (!is_stable_to_bob(g, c)) return false;
    if (c.reading.shape == ClueShape::TRASH_REVEAL) return true;
    if (c.reading.shape != ClueShape::STABLE_DISCARD) return false;
    const int o = c.reading.stable_subject;
    auto id = id_of(g.state, o);
    if (!id) return false;
    return g.state.is_basic_trash(*id) ||
           has_same_hand_dupe(g.state, bob_of(g), o, *id);
  });
}

// 3.3 / 4.3 -- the same, but the card is merely DUPLICATED where Alice can see
// it (Cathy's hand, or her own if she knows), rather than outright trash.
Pool pool_stable_ditch_dupe(const Game& g, const std::vector<ClueCandidate>& cs) {
  return select(cs, [&g](const ClueCandidate& c) {
    if (!is_stable_to_bob(g, c)) return false;
    if (c.reading.shape != ClueShape::STABLE_DISCARD) return false;
    const int o = c.reading.stable_subject;
    auto id = id_of(g.state, o);
    return id && dupe_visible_elsewhere(g, bob_of(g), o, *id);
  });
}

// 3.4 / 3.5 / 3.7 / 4.4 -- a lock clue to Bob.
Pool pool_lock(const Game& g, const std::vector<ClueCandidate>& cs) {
  return select(cs, [&g](const ClueCandidate& c) {
    return is_stable_to_bob(g, c) && c.reading.shape == ClueShape::STABLE_LOCK;
  });
}

// Is `order` something the team can simply lose — basic trash, or a card its
// holder has a second copy of? Tighter than `discard_is_affordable`, which also
// accepts a copy visible in ANOTHER hand. The double-discard rungs use this one:
// "two trash cards or same-hand-dupes" is about cards nobody needs, not about
// cards someone else happens to be holding too.
bool is_trash_or_same_hand_dupe(const Game& g, int holder, int order) {
  auto id = id_of(g.state, order);
  if (!id) return false;
  return g.state.is_basic_trash(*id) ||
         has_same_hand_dupe(g.state, holder, order, *id);
}

// Priority 3 rungs 2 and 4 — a double discard that throws away two cards nobody
// needs. Both designated cards must be trash or a same-hand-dupe.
//
// The spec writes the shape as "stamps CTD on two trash cards or same-hand-dupes,
// or CTP to a trash or same-hand-dupe in an inverted suit". Those are the same
// two cases `Outcome::DISCARD` already distinguishes — CTD on a plain suit, CTP
// on an inverted one — and DOUBLE_DISCARD means both sides discard by
// construction, so what is left to check is only that neither card is wanted.
Pool pool_double_discard(const Game& g, const std::vector<ClueCandidate>& cs) {
  return select(cs, [&g](const ClueCandidate& c) {
    if (c.reading.shape != ClueShape::DOUBLE_DISCARD) return false;
    auto sides = discarded_sides(g, c.reading);
    if (sides.size() != 2) return false;
    for (const auto& side : sides) {
      if (!is_trash_or_same_hand_dupe(g, side.first, side.second)) return false;
    }
    return true;
  });
}

// 3.8 / 4.8 -- throw away a non-critical card of Bob's through a REACTIVE,
// preferring the card with the most connectors Alice cannot see (so the one the
// team is least likely to ever play).
//
// The spec pairs a shape set with a BUTTON on each of its two arms, and the
// pairing differs between the two rungs, so both sets are passed in rather than
// inferred:
//
//   3.8  CTD arm: reactive discard.                  CTP arm: reactive play.
//   4.8  CTD arm: reactive discard or double discard. CTP arm: reactive play or
//                                                     reactive discard.
//
// The CTP arm additionally requires the card to be on an inverted suit, because
// that is the only way pressing Play throws a card away.
const ClueCandidate* rung_reactive_ditch(const Game& g,
                                         const std::vector<ClueCandidate>& cs,
                                         std::initializer_list<ClueShape> ctd_shapes,
                                         std::initializer_list<ClueShape> ctp_shapes) {
  auto allows = [](std::initializer_list<ClueShape> set, ClueShape sh) {
    for (ClueShape s : set) {
      if (s == sh) return true;
    }
    return false;
  };
  Pool p = select(cs, [&](const ClueCandidate& c) {
    // Bob is the reacter on a reactive; the rung is about HIS card.
    const Designation& side = c.reading.reacter_side;
    auto id = id_of(g.state, side.order);
    if (!id || g.state.is_critical(*id)) return false;
    if (side.button == CardStatus::CALLED_TO_DISCARD) {
      return allows(ctd_shapes, c.reading.shape);
    }
    if (side.button == CardStatus::CALLED_TO_PLAY) {
      return allows(ctp_shapes, c.reading.shape) &&
             variants::is_inverted_id(g.state, *id);
    }
    return false;
  });
  if (p.empty()) return nullptr;
  const ClueCandidate* best = p.front();
  int best_missing = missing_connectors(g, best->reading.reacter_side.order);
  for (const ClueCandidate* c : p) {
    const int m = missing_connectors(g, c->reading.reacter_side.order);
    if (m > best_missing ||
        (m == best_missing && c->default_score > best->default_score)) {
      best = c;
      best_missing = m;
    }
  }
  return best;
}

const ClueCandidate* first_of(const Game& g, Pool p) {
  return settle(g, std::move(p), {});
}

}  // namespace

// --- priority 3 ----------------------------------------------------------
// Precondition: Bob is not locked, he has no safe play or discard, and his
// chop is worth a clue -- endangered or playable. See the body; this used to
// be the weaker "non-trash", which spent clues on chops nothing was at stake for.
bool priority_3_applies(const Game& g) {
  const int bob = bob_of(g);
  if (g.common.thinks_locked(g, bob)) return false;
  // Two reasons to spend a clue on Bob's chop, and "non-trash" was too weak to
  // separate them from the case with neither.
  //
  //   * `at_risk_chop` -- the same test H1a uses. It clears a same-hand dupe, a
  //     copy sitting in a hand Alice can see, and a copy Alice can prove she
  //     holds; in each case Bob loses nothing by throwing his.
  //   * `has_playable_chop` -- N5's test. The card is not in DANGER, but it is
  //     a play the team should be collecting, which is a reason of its own.
  //
  // Both replays turn on the difference. 1966745 T5: Bob's chop was an r2 with
  // red on 0 and Cathy held the other r2 -- not endangered AND not playable, so
  // §3 fired and spent a clue for nothing. 1942330 T33: Bob's chop was a
  // PLAYABLE Navy 2, also duplicated in Cathy's hand -- not endangered either,
  // but worth the Blue play clue that gets it onto the stacks.
  //
  // `at_risk_chop` also returns false for a locked hand and for a chop Alice
  // cannot see, so the explicit guards this replaces are subsumed, not dropped.
  if (!at_risk_chop(g, alice_of(g), bob) && !has_playable_chop(g, bob)) {
    return false;
  }
  return has_no_safe_action(g, bob);
}

namespace {

const ClueCandidate* rung_3(const Game& g, const std::vector<ClueCandidate>& cs) {
  if (!priority_3_applies(g)) return nullptr;
  // 3.1 -- a stable play clue to Bob.
  if (clues_at_least(g, 2)) {
    if (auto* c = first_of(g, pool_stable_play(g, cs))) return c;
  }
  // 3.2 -- a double discard, when Cathy's chop is NOT expendable. It outranks
  // the stable discard below because it does two jobs at once: it clears two
  // unwanted cards AND it redirects Cathy off a chop she could not afford to
  // lose. Without the chop condition that second job does not exist, which is
  // why the same clue sits again at 3.4, lower.
  if (has_cathy(g) && !chop_is_expendable(g, cathy_of(g))) {
    if (auto* c = first_of(g, pool_double_discard(g, cs))) return c;
  }
  // 3.3 -- a stable discard or trash reveal to Bob, and only when Bob does not
  // already have a safe discard. Telling him about a second dead card buys
  // nothing if he can already throw one away.
  if (!has_safe_discard(g, bob_of(g))) {
    if (auto* c = first_of(g, pool_stable_ditch_trash(g, cs))) return c;
  }
  // 3.4 -- the same double discard as 3.2, now unconditional on Cathy's chop.
  if (auto* c = first_of(g, pool_double_discard(g, cs))) return c;
  // 3.5 -- a stable discard to Bob on a card Alice can see a copy of elsewhere.
  if (auto* c = first_of(g, pool_stable_ditch_dupe(g, cs))) return c;
  // 3.6 -- all of Bob's cards are critical
  if (clues_at_least(g, 2)) {
    bool all_critical = true;
    for (int o : g.state.hands[bob_of(g)]) {
      auto id = id_of(g.state, o);
      if (!id || !g.state.is_critical(*id)) {
        all_critical = false;
        break;
      }
    }
    if (all_critical) {
      if (auto* c = first_of(g, pool_lock(g, cs))) return c;
    }
  }
  // 3.7 -- L = (# 1-away) + 2 * (# trash) in Bob's hand
  if (clues_at_least(g, 3)) {
    int L = 0;
    for (int o : g.state.hands[bob_of(g)]) {
      auto id = id_of(g.state, o);
      if (!id) continue;
      if (g.state.is_basic_trash(*id)) {
        L += 2;
      } else if (g.state.playable_away(*id) == 1) {
        L += 1;
      }
    }
    if (L >= 3) {
      if (auto* c = first_of(g, pool_lock(g, cs))) return c;
    }
  }
  // 3.8 -- unconditional: it carries no clue-count condition, and the `**`
  // relaxation does not reach it (the ruling on open item 2).
  if (auto* c = rung_reactive_ditch(g, cs, {ClueShape::REACTIVE_DISCARD},
                                    {ClueShape::REACTIVE_PLAY})) {
    return c;
  }
  // 3.9
  if (clues_at_least(g, 2)) {
    if (auto* c = first_of(g, pool_lock(g, cs))) return c;
  }
  return nullptr;
}

// 4.4 -- a FILL-IN clue: a stable clue to Bob that narrows an already-clued
// card he cannot play yet. At 8 tokens this spends the token without asking him
// to do anything, which is the point of a stall.
//
// Candidates are ranked by their BEST filled-in card, on the spec's three
// criteria in order: a card duplicated where Alice can see it (Cathy's hand or
// her own) is worth resolving first, then the fewest missing connectors, then
// the lowest stack rank. The last two both prefer a card the team is CLOSE to
// playing, which is the opposite of rungs 3.8 / 4.8, where the tiebreak wants
// the card least likely ever to matter.
const ClueCandidate* rung_fill_in(const Game& g,
                                  const std::vector<ClueCandidate>& cs) {
  const int bob = bob_of(g);
  // The key for one filled-in card. Lower sorts better on every component.
  struct Key {
    int not_duped = 1;
    int connectors = 0;
    int rank = 0;
    bool operator<(const Key& o) const {
      if (not_duped != o.not_duped) return not_duped < o.not_duped;
      if (connectors != o.connectors) return connectors < o.connectors;
      return rank < o.rank;
    }
  };
  auto key_of = [&](int order) {
    Key k;
    auto id = id_of(g.state, order);
    if (!id) return k;
    k.not_duped = dupe_visible_elsewhere(g, bob, order, *id) ? 0 : 1;
    k.connectors = missing_connectors(g, order);
    k.rank = variants::direction_rank(g.state, *id);
    return k;
  };

  const ClueCandidate* best = nullptr;
  Key best_key;
  for (const ClueCandidate& c : cs) {
    if (predicts_a_strike(c.reading)) continue;
    if (!is_stable_to_bob(g, c) || c.fill_ins.empty()) continue;
    // A clue that locks is not a fill-in, however much it also narrows. The
    // whole reason 4.4 sits above the lock is to prefer spending the forced
    // token WITHOUT committing Bob's hand; letting a lock in here would defeat
    // that, and if the only fill-in available also locks then 4.6 takes it
    // anyway.
    if (c.reading.shape == ClueShape::STABLE_LOCK) continue;
    for (int o : c.fill_ins) {
      const Key k = key_of(o);
      if (!best || k < best_key) {
        best = &c;
        best_key = k;
      }
    }
  }
  return best;
}

// 4.5 -- a safe STALL: a clue that designates nothing at all, so Bob cannot
// read it as an instruction to play or discard anything.
//
// "Cannot be misinterpreted by Bob" is answered by the interpretation itself
// rather than by a separate model of him: `read_clue` reports the reading every
// seat decodes, and `analyse_clues` has already dropped anything reading
// MISTAKE. A clue that stamps no call therefore cannot make him strike or throw
// a critical card away.
//
// Locks are excluded even though they designate no single card: a lock is a
// real instruction, and it has its own rung just below at 4.6.
const ClueCandidate* rung_safe_stall(const Game& g,
                                     const std::vector<ClueCandidate>& cs) {
  Pool p = select(cs, [](const ClueCandidate& c) {
    if (c.reading.shape != ClueShape::OTHER) return false;
    return c.reading.reacter_side.order < 0 &&
           c.reading.receiver_side.order < 0 && c.reading.stable_subject < 0;
  });
  return settle(g, std::move(p), {});
}

// --- priority 4: Alice is at 8 clues and must clue or pitch ---------------
const ClueCandidate* rung_4(const Game& g, const std::vector<ClueCandidate>& cs) {
  // "Alice is LOCKED or at 8 clues and is forced to clue or pitch." Both are
  // positions where the ordinary list has run out and she still has to do
  // something: at 8 tokens a discard is illegal, and locked she has no chop to
  // discard, so her only alternative to cluing is burning a card.
  //
  // Replay 1966633 T5 is the locked half. Bob held a standing CTD, so the whole
  // of priority 3 declined on "no safe play or discard", section 4 needed 8
  // tokens and there were 7, and a locked Alice bombed her way out of the turn
  // with a perfectly good play reveal on the table.
  const bool locked = g.common.thinks_locked(g, alice_of(g));
  if (g.state.clue_tokens != 8 && !locked) return nullptr;
  // 4.1 is "same as 3.1", which carries 3.1's own clue-count condition.
  if (clues_at_least(g, 2)) {
    if (auto* c = first_of(g, pool_stable_play(g, cs))) return c;       // 4.1
  }
  // 4.2 is "same as 3.3", and carries 3.3's safe-discard condition with it.
  if (!has_safe_discard(g, bob_of(g))) {
    if (auto* c = first_of(g, pool_stable_ditch_trash(g, cs))) return c;  // 4.2
  }
  if (auto* c = first_of(g, pool_stable_ditch_dupe(g, cs))) return c;   // 4.3
  // 4.4 and 4.5 sit ABOVE the lock deliberately. A lock commits Bob's whole
  // hand, so at a forced clue it is worth less than information or than a
  // harmless stall. It is also the only order in which either can ever run: at
  // 8 tokens a re-clue of already-clued cards reads as a LOCK, so a lock
  // candidate is available in essentially every position, and a lock above
  // these two would swallow them both.
  if (auto* c = rung_fill_in(g, cs)) return c;                          // 4.4
  if (auto* c = rung_safe_stall(g, cs)) return c;                       // 4.5
  if (auto* c = first_of(g, pool_lock(g, cs))) return c;                // 4.6
  // 4.7 -- below 2 strikes, a stable clue that makes Bob throw away a trash or
  // duplicated card, explicitly allowing a strike. This is the ONE rung that
  // tolerates a predicted misplay, so it does not go through `select`.
  if (g.state.strikes < 2) {
    Pool p;
    for (const ClueCandidate& c : cs) {
      if (!is_stable_to_bob(g, c)) continue;
      if (c.reading.shape != ClueShape::STABLE_DISCARD &&
          c.reading.shape != ClueShape::STABLE_PLAY) {
        continue;
      }
      const int o = c.reading.stable_subject;
      auto id = id_of(g.state, o);
      if (!id) continue;
      const bool unwanted = g.state.is_basic_trash(*id) ||
                            has_same_hand_dupe(g.state, bob_of(g), o, *id) ||
                            dupe_visible_elsewhere(g, bob_of(g), o, *id);
      if (unwanted) p.push_back(&c);
    }
    if (auto* c = first_of(g, std::move(p))) return c;
  }
  // 4.8 -- wider than 3.8 on both arms: the CTD arm also takes a double
  // discard, and the CTP arm also takes a reactive discard.
  if (auto* c = rung_reactive_ditch(
          g, cs, {ClueShape::REACTIVE_DISCARD, ClueShape::DOUBLE_DISCARD},
          {ClueShape::REACTIVE_PLAY, ClueShape::REACTIVE_DISCARD})) {
    return c;
  }
  // The floor. At 8 tokens a discard is illegal, so section 4 must return
  // something: the default tiebreak, IGNORING tier. Without this an empty clue
  // set walks into take_action's last-resort branch and blind-plays slot 1,
  // which is strictly worse than any decodable clue.
  Pool all;
  for (const ClueCandidate& c : cs) all.push_back(&c);
  return first_of(g, std::move(all));
}

}  // namespace

std::optional<PerformAction> choose_h4_clue(
    const Game& game, const std::vector<ClueCandidate>& cands) {
  std::vector<ClueCandidate> h4;
  int h4_seen = 0;
  for (const ClueCandidate& c : cands) {
    if (!c.is_h4) continue;
    ++h4_seen;
    if (clue_is_admissible(game, c)) h4.push_back(c);
  }
  if (h4.empty()) {
    // Logged even when it declines: this is Precedence step 1, so "no H4 clue"
    // is exactly why a pending reaction was allowed to proceed, and that is
    // worth being able to read off a trace.
    hanabi::logging::log_branch("reactor0.choose_h4_clue",
                                {{"fired", false},
                                 {"h4_candidates", h4_seen},
                                 {"candidates", cands.size()}});
    return std::nullopt;
  }
  // Rank the H4 clues against each other with the ordinary list, minus the
  // section 4 floor -- an H4 clue outranks a pending reaction, an arbitrary one
  // does not.
  const ClueCandidate* pick = nullptr;
  const char* rung = "";
  if ((pick = rung_1(game, h4))) {
    rung = "1.reactive_play";
  } else if ((pick = rung_2(game, h4))) {
    rung = "2.reactive_discard";
  } else if ((pick = rung_3(game, h4))) {
    rung = "3.bob_chop";
  } else {
    Pool all;
    for (const ClueCandidate& c : h4) all.push_back(&c);
    pick = first_of(game, std::move(all));
    rung = "default_tiebreak";
  }
  if (!pick) return std::nullopt;
  // An H4 clue OUTRANKS a pending reaction (DECISION_MAKING.md Precedence step
  // 1), so when this fires it is the reason the urgent path never ran. Record
  // enough to tell that from a trace without a debugger.
  hanabi::logging::log_branch(
      "reactor0.choose_h4_clue",
      {{"fired", true},
       {"rung", rung},
       {"shape", shape_name(pick->reading.shape)},
       {"target", pick->action.target},
       {"kind", pick->action.clue.kind == ClueKind::COLOUR ? "colour" : "rank"},
       {"value", pick->action.clue.value},
       {"h4_candidates", h4_seen},
       {"outranked_a_reaction", !game.waiting.empty()}});
  return pick->perform;
}

std::optional<PerformAction> choose_clue(
    const Game& game, const std::vector<ClueCandidate>& cands) {
  hanabi::instr::ScopedTimer st("reactor0.choose_clue");
  if (cands.empty()) return std::nullopt;
  std::vector<ClueCandidate> ok;
  for (const ClueCandidate& c : cands) {
    if (clue_is_admissible(game, c)) ok.push_back(c);
  }
  // Logged on decline as well as on a pick. "Why did it not clue?" is the same
  // question as "which rung fired?", asked from the other side, and until now
  // only one of the two left a trace.
  if (ok.empty()) {
    hanabi::logging::log_branch("reactor0.choose_clue",
                                {{"picked", false},
                                 {"reason", "tier_gate_rejected_all"},
                                 {"candidates", cands.size()},
                                 {"pace", game.state.pace()},
                                 {"clue_tokens", game.state.clue_tokens},
                                 {"occupied", requires_high_tier(game)}});
    return std::nullopt;
  }

  const ClueCandidate* pick = nullptr;
  const char* rung = "";
  if ((pick = rung_1(game, ok))) {
    rung = "1.reactive_play";
  } else if ((pick = rung_2(game, ok))) {
    rung = "2.reactive_discard";
  } else if ((pick = rung_3(game, ok))) {
    rung = "3.bob_chop";
  } else if ((pick = rung_4(game, ok))) {
    rung = "4.locked_or_eight_clues";
  }
  if (!pick) {
    hanabi::logging::log_branch("reactor0.choose_clue",
                                {{"picked", false},
                                 {"reason", "no_rung_matched"},
                                 {"admissible", ok.size()},
                                 {"candidates", cands.size()},
                                 {"clue_tokens", game.state.clue_tokens}});
    return std::nullopt;
  }
  hanabi::logging::log_branch("reactor0.choose_clue",
                              {{"rung", rung},
                               {"shape", shape_name(pick->reading.shape)},
                               {"target", pick->action.target},
                               {"kind", pick->action.clue.kind == ClueKind::COLOUR
                                            ? "colour"
                                            : "rank"},
                               {"value", pick->action.clue.value},
                               {"tier", static_cast<int>(pick->tier)},
                               {"candidates", ok.size()},
                               {"clue_tokens", game.state.clue_tokens}});
  return pick->perform;
}

}  // namespace hanabi::reactor0
