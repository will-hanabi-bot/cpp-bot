#include "hanabi/conventions/reactor0/decision.h"

#include <algorithm>
#include <initializer_list>
#include <functional>
#include <utility>
#include <variant>

#include "hanabi/basics/clue.h"
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
CardStatus receiver_button(ClueKind kind, CardStatus reacter_button) {
  const bool reacter_plays = reacter_button == CardStatus::CALLED_TO_PLAY;
  if (kind == ClueKind::RANK) {
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
  const CardStatus rb = receiver_button(action.clue.kind, reacter_status);
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
    out.push_back(std::move(c));
  }
  return out;
}

bool clue_is_admissible(const Game& game, const ClueCandidate& c) {
  const State& s = game.state;
  if (c.action.giver != s.our_player_index) return true;  // fails open
  if (s.pace() < 3) return true;
  const bool occupied = requires_high_tier(game);
  const bool in_window = occupied ? s.clue_tokens < 8 : s.clue_tokens <= 3;
  if (!in_window) return true;
  return c.tier >= (occupied ? ClueTier::HIGH : ClueTier::MEDIUM);
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
    auto sides = discarded_sides(g, c.reading);
    if (sides.empty()) return false;
    // EVERY discarded card must be affordable. The spec says "the discarded
    // card"; a double discard throws two, and losing either is a real loss, so
    // the conservative reading is the one that cannot cost the team a card.
    for (const auto& side : sides) {
      if (!discard_is_affordable(g, side.first, side.second)) return false;
    }
    return true;
  });
  std::vector<Term> chain = bob_card_chain(g, /*require_bob_plays=*/true);
  // 6. the discarded card is a same-hand-dupe
  chain.push_back([&g](const ClueCandidate& c) {
    for (const auto& side : discarded_sides(g, c.reading)) {
      auto id = id_of(g.state, side.second);
      if (id && has_same_hand_dupe(g.state, side.first, side.second, *id)) return true;
    }
    return false;
  });
  // 7. the discarded card is trash
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

// --- priority 3 ----------------------------------------------------------
// Precondition: Bob has a non-trash card on chop (so is not locked) and no safe
// play or discard. Note this is WEAKER than H1a, which additionally demands the
// chop be endangered -- here it is enough that the card is worth something.
bool priority_3_applies(const Game& g) {
  const int bob = bob_of(g);
  if (g.common.thinks_locked(g, bob)) return false;
  auto chop = g.chop(bob);
  if (!chop) return false;
  auto id = id_of(g.state, *chop);
  if (!id || g.state.is_basic_trash(*id)) return false;
  return has_no_safe_action(g, bob);
}

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
  // 3.3 -- a stable discard or trash reveal to Bob.
  if (auto* c = first_of(g, pool_stable_ditch_trash(g, cs))) return c;
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

// --- priority 4: Alice is at 8 clues and must clue or pitch ---------------
const ClueCandidate* rung_4(const Game& g, const std::vector<ClueCandidate>& cs) {
  if (g.state.clue_tokens != 8) return nullptr;
  if (auto* c = first_of(g, pool_stable_play(g, cs))) return c;         // 4.1
  if (auto* c = first_of(g, pool_stable_ditch_trash(g, cs))) return c;  // 4.2
  if (auto* c = first_of(g, pool_stable_ditch_dupe(g, cs))) return c;   // 4.3
  if (auto* c = first_of(g, pool_lock(g, cs))) return c;                // 4.4
  // 4.5 fill-in and 4.6 safe stall are NOT YET IMPLEMENTED (v7.1.0). The floor
  // below guarantees the branch still returns a legal clue without them.
  //
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
  for (const ClueCandidate& c : cands) {
    if (c.is_h4 && clue_is_admissible(game, c)) h4.push_back(c);
  }
  if (h4.empty()) return std::nullopt;
  // Rank the H4 clues against each other with the ordinary list, minus the
  // section 4 floor -- an H4 clue outranks a pending reaction, an arbitrary one
  // does not.
  if (const ClueCandidate* c = rung_1(game, h4)) return c->perform;
  if (const ClueCandidate* c = rung_2(game, h4)) return c->perform;
  if (const ClueCandidate* c = rung_3(game, h4)) return c->perform;
  Pool all;
  for (const ClueCandidate& c : h4) all.push_back(&c);
  const ClueCandidate* best = first_of(game, std::move(all));
  return best ? std::optional<PerformAction>{best->perform} : std::nullopt;
}

std::optional<PerformAction> choose_clue(
    const Game& game, const std::vector<ClueCandidate>& cands) {
  hanabi::instr::ScopedTimer st("reactor0.choose_clue");
  if (cands.empty()) return std::nullopt;
  std::vector<ClueCandidate> ok;
  for (const ClueCandidate& c : cands) {
    if (clue_is_admissible(game, c)) ok.push_back(c);
  }
  if (ok.empty()) return std::nullopt;

  const ClueCandidate* pick = nullptr;
  const char* rung = "";
  if ((pick = rung_1(game, ok))) {
    rung = "1.reactive_play";
  } else if ((pick = rung_2(game, ok))) {
    rung = "2.reactive_discard";
  } else if ((pick = rung_3(game, ok))) {
    rung = "3.bob_chop";
  } else if ((pick = rung_4(game, ok))) {
    rung = "4.eight_clues";
  }
  if (!pick) return std::nullopt;
  hanabi::logging::log_branch("reactor0.choose_clue",
                              {{"rung", rung},
                               {"shape", shape_name(pick->reading.shape)},
                               {"target", pick->action.target},
                               {"clue_tokens", game.state.clue_tokens}});
  return pick->perform;
}

}  // namespace hanabi::reactor0
