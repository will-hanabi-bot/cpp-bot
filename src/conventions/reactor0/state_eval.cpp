#include "hanabi/conventions/reactor0/state_eval.h"
#include "hanabi/conventions/reactor0/reactive_assignment.h"

#include "hanabi/conventions/reactor0/facts.h"

#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
#include "hanabi/conventions/reactor0/interpret_reactive.h"
#include "hanabi/conventions/reactor0/decision.h"
#include "hanabi/conventions/reactor0/interpret_reaction.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/conventions/variants/predicates.h"
#include "hanabi/conventions/variants/reversed.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor0 {

namespace variants = hanabi::reactor::variants;

namespace {

const char* tier_name(ClueTier t) {
  switch (t) {
    case ClueTier::HIGH: return "high";
    case ClueTier::MEDIUM: return "medium";
    default: return "low";
  }
}

}  // namespace

// --- Position facts (declared in facts.h) --------------------------------

// True when Alice can prove she is holding a copy of `id` — either from a
// singleton inference, or from a group ("sudoku") elim.
//
// The pigeonhole: take any subset S of the hand, |S| = k, and let `u` be the
// union of what those cards could be. If none of them were `id`, all k would
// have to be drawn from `u \ {id}` — so if fewer than k copies of `u \ {id}`
// are still unaccounted for, at least one of the k must be `id`.
// `Player::unknown_ids` (src/basics/player.cpp:73-81) supplies that count;
// `linked_orders` (:90-98) applies the identical pigeonhole to links.
//
// Errs safe. `unknown_ids` also counts draw-pile copies, so the bound is an
// over-estimate, which can only make this test fire LESS often — and a false
// "Alice holds it" is the expensive direction: it kills H1/N1, lets the gate
// reject a save clue, and the chop is lost for real. k == 1 reduces to exactly
// the singleton test reactor's `chop_id_is_unique` performs
// (reactor/state_eval.cpp:96-101).
//
// `cross_elim` (src/basics/player_elim.cpp:165-226) does the analogous
// subset-locking during elim, but it runs in the opposite direction — it
// removes locked ids from cards *outside* the group and never concludes that a
// card *inside* it is a given identity — so it cannot answer this. A 3-player
// hand is 5 cards, so all 31 non-empty subsets are enumerated directly.
bool alice_provably_holds(const Game& game, int alice, Identity id) {
  const State& s = game.state;
  // Two different views, each where it belongs. The inference sets come from
  // `common` — the view reactor's `chop_id_is_unique` reads
  // (reactor/state_eval.cpp:99), and the one both the engine and the test
  // harness maintain for a clued own-hand card. The availability counts come
  // from `me()`, which can see the other hands and so knows how many copies
  // are genuinely still in play.
  const Player& infs = game.common;
  const Player& seen = game.me();
  const auto& hand = s.hands[alice];
  const int n = static_cast<int>(hand.size());
  if (n <= 0 || n > 12) return false;  // guard the shift; hands are ≤ 6

  std::vector<IdentitySet> base(n);
  for (int i = 0; i < n; ++i) {
    const Thought& t = infs.thoughts[hand[i]];
    base[i] = t.inferred.non_empty() ? t.inferred : t.possible;
  }

  for (unsigned mask = 1; mask < (1u << n); ++mask) {
    IdentitySet u = IdentitySet::empty();
    int k = 0;
    bool usable = true;
    for (int i = 0; i < n; ++i) {
      if (!((mask >> i) & 1u)) continue;
      if (!base[i].non_empty()) {
        usable = false;
        break;
      }
      u |= base[i];
      ++k;
    }
    if (!usable || k == 0 || !u.contains(id)) continue;
    int cap = 0;  // copies these k cards could be *without* any being `id`
    for (Identity other : u - IdentitySet::single(id)) {
      cap += seen.unknown_ids(s, other);
    }
    if (cap < k) return true;
  }
  return false;
}

// Is `player`'s chop a card the team cannot afford to lose?
//
// Judged from Alice's full visibility, and stricter than reactor's pair of
// `chop_is_nontrash` / `chop_id_is_unique` (reactor/state_eval.cpp:44-49,
// :82-103) in one respect: a second copy of the identity in the holder's OWN
// hand makes the chop expendable, so it counts as trash here.
// True when a card other than `chop_order` in `player`'s OWN hand carries
// `id` — the holder can pitch one copy safely, so that card is expendable.
bool has_same_hand_dupe(const State& s, int player, int chop_order,
                        Identity id) {
  for (int o : s.hands[player]) {
    if (o == chop_order) continue;
    auto other = s.deck[o].id();
    if (other && *other == id) return true;
  }
  return false;
}

// A playable card on an INVERTED suit costs its holder nothing to lose, because
// losing it is not what happens: pressing Discard on an orange CHUCKS it onto
// its own stack. It is the one card the holder should be throwing.
//
// So it belongs with basic trash and a same-hand dupe wherever those are asked
// about -- it is not endangered, it is not a play the team must arrange, and it
// is expendable. All three chop predicates below read it.
//
// Replay 1973974 T10: will-bot69's chop was a playable o2, which made both arms
// of `priority_3_applies` fire, and will-bot67 spent a clue LOCKING him over a
// card he was about to chuck for a point.
//
// Judged from the caller's full visibility, like every other term here: nullopt
// for our own card, in which case there is nothing to conclude.
bool chop_is_free_chuck(const State& s, std::optional<int> chop) {
  if (!chop) return false;
  auto id = s.deck[*chop].id();
  if (!id) return false;
  return variants::is_inverted_id(s, *id) && s.is_playable(*id);
}

// Rungs 3.7 / 3.8 / 3.9's "X's chop is critical". See facts.h for why this is
// not simply `is_critical` and not `at_risk_chop` either.
bool chop_is_critical(const Game& game, int player) {
  const State& s = game.state;
  auto chop = game.chop(player);
  if (!chop) return false;  // locked hand — no chop
  auto id = s.deck[*chop].id();
  if (!id) return false;  // unknown from our POV — cannot verify
  if (!s.is_critical(*id)) return false;
  // The chuck plays it rather than losing it, so nothing needs arranging.
  return !chop_is_free_chuck(s, chop);
}

bool at_risk_chop(const Game& game, int alice, int player) {
  const State& s = game.state;
  auto chop = game.chop(player);
  if (!chop) return false;  // locked hand — no chop to lose
  auto id = s.deck[*chop].id();
  if (!id) return false;  // unknown from our POV — cannot verify
  if (s.is_basic_trash(*id)) return false;
  // A playable inverted card is chucked, not lost — nothing to save.
  if (chop_is_free_chuck(s, chop)) return false;

  // Same-hand duplicate: the holder can pitch one copy safely.
  if (has_same_hand_dupe(s, player, *chop, *id)) return false;
  // A copy sitting in a hand we can see (everyone but ourselves and the
  // holder). At 3 players this is exactly "Cathy's hand" when player == Bob.
  for (int p = 0; p < s.num_players; ++p) {
    if (p == alice || p == player) continue;
    for (int o : s.hands[p]) {
      auto other = s.deck[o].id();
      if (other && *other == *id) return false;
    }
  }
  // A copy we can prove we are holding ourselves.
  if (alice_provably_holds(game, alice, *id)) return false;
  return true;
}

// N5: does `player` hold a **playable** card on their chop that is not a
// same-hand duplicate? Deliberately weaker than `at_risk_chop` — it does not
// care whether a copy sits in another hand or is provable in Alice's, because
// the point is not that the card is in danger but that it is a play the team
// should be collecting. Cathy is likely already expecting Alice to save it or
// get it played, so any clue this turn carries at least MEDIUM value.
bool has_playable_chop(const Game& game, int player) {
  const State& s = game.state;
  auto chop = game.chop(player);
  if (!chop) return false;  // locked hand — no chop
  auto id = s.deck[*chop].id();
  if (!id) return false;  // unknown from our POV — cannot verify
  if (!s.is_playable(*id)) return false;
  // ...but not an INVERTED playable. N5 exists because the team should be
  // arranging to collect the card; a playable orange collects itself the moment
  // its holder discards, so there is nothing to arrange and no clue is owed.
  if (chop_is_free_chuck(s, chop)) return false;
  return !has_same_hand_dupe(s, player, *chop, *id);
}

// What the candidate clue achieves, read off the CTP stamps its
// interpretation produced. Same walk as reactor's `is_high_value_clue`
// (reactor/state_eval.cpp:126-142).
NewPlayFacts new_play_facts(const Game& game, const Game& hypo) {
  const State& s = game.state;
  NewPlayFacts f;
  auto credit = [&](int o) {
    ++f.count;
    auto id = s.deck[o].id();
    if (!id) return;
    if (s.is_critical(*id) && variants::is_first_or_second_rank(s, *id)) {
      f.critical_low = true;
    }
    if (variants::is_clue_regain_rank(s, *id)) f.regain_rank = true;
  };

  for (const auto& hand : s.hands) {
    for (int o : hand) {
      if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
      if (hypo.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      credit(o);
    }
  }

  // The receiver of a reactive is not stamped at clue time -- the call is made
  // when the reacter acts (§1d) -- so the stamp walk above cannot see it. Left
  // uncounted, H3 ("two new plays") would be unreachable for every reactive and
  // H2 would stop seeing the receiver's card, which quietly deflates the tier of
  // the whole family. Recover it the way `read_clue` does.
  if (!hypo.waiting.empty()) {
    const ReactorWC& wc = hypo.waiting.front();
    auto recv_order = predicted_receiver_order(hypo);
    const CardStatus reacter_status =
        wc.react_order >= 0 ? hypo.meta[wc.react_order].status : CardStatus::NONE;
    const bool reacter_called = reacter_status == CardStatus::CALLED_TO_PLAY ||
                                reacter_status == CardStatus::CALLED_TO_DISCARD;
    // Skip a receiver the stamp walk already credited, so the two sources can
    // never double-count while both exist.
    if (recv_order && reacter_called && !predicts_reactive_lock(hypo) &&
        !(game.meta[*recv_order].status != CardStatus::CALLED_TO_PLAY &&
          hypo.meta[*recv_order].status == CardStatus::CALLED_TO_PLAY)) {
      const CardStatus rb = receiver_button(
          reactive_assignment_for(*s.variant, game.reactive_overrides,
                                  wc.clue.kind, wc.clue.value,
                                  /*target_is_bob=*/wc.clue.target == wc.reacter)
              .even,
          reacter_status);
      // Judged against the stacks the reacter leaves behind, and via
      // `outcome_of` rather than the button name -- so an inverted CTD, which
      // is a CHUCK onto the stack, is correctly counted as a play.
      const State after = state_after_reacter(s, wc.react_order, reacter_status);
      if (outcome_of(after, *recv_order, rb) == Outcome::PLAY) credit(*recv_order);
    }
  }
  return f;
}

// Could `giver` hand `receiver` a colour clue that reactor0 reads as a direct
// play clue naming a card that actually plays? A structural check, not a
// simulation: it replays `stable_colour`'s target choice
// (src/conventions/reactor0/interpret_clue.cpp:130-152) and its three guards,
// then confirms the named card is genuinely playable from our full visibility.
bool has_colour_play_clue_for(const Game& game, int giver, int receiver) {
  const State& s = game.state;
  // This models a STABLE colour play clue, and while target parity binds there
  // are no stable clues at all -- every clue is reactive. Both callers (H1c and
  // N2) are asking "could Bob have handled Cathy himself?", and there the answer
  // is never "yes, with a stable clue".
  //
  // Asked as `bob_clue_is_reactive` rather than `uses_target_parity` since
  // v11.0.0. The question is about a clue from BOB to CATHY, and Cathy is Bob's
  // own "Bob" -- so past the 50% threshold that clue is stable again and this
  // has a real answer to give. Gating on the variant alone would keep both arms
  // vacuous for the rest of the game.
  if (bob_clue_is_reactive(s)) return false;
  for (const Clue& c : s.all_colour_clues(receiver)) {
    auto touched = s.clue_touched(s.hands[receiver], c.kind, c.value);
    if (touched.empty()) continue;
    ClueAction probe{giver, receiver, touched, c.base()};
    auto target = leftmost_could_be_playable(game, probe, probe.list_);
    if (!target) continue;
    if (game.is_blind_playing(*target)) continue;
    auto tid = s.deck[*target].id();
    if (game.meta[*target].status == CardStatus::CALLED_TO_DISCARD &&
        !(tid && s.is_playable(*tid))) {
      continue;
    }
    if (tid && variants::is_inverted_id(s, *tid)) continue;
    if (tid && s.is_playable(*tid)) return true;
  }
  return false;
}



bool requires_high_tier(const Game& game) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  // A CTD is an action only in a variant with an inverted suit, where
  // discarding is how an inverted card is played.
  const bool inverted_variant = variants::includes_inverted(s);
  for (int o : s.hands[alice]) {
    const CardStatus st = game.meta[o].status;
    if (st == CardStatus::CALLED_TO_PLAY) return true;
    if (inverted_variant && st == CardStatus::CALLED_TO_DISCARD) return true;
  }
  return false;
}

// --- Cathy-chop facts, and the finesse detector (DECISION_MAKING.md H1b/H1c/VH1)

// `player`'s chop identity from ALICE's full visibility. Nullopt when the hand
// is locked (no chop) or the card is Alice's own.
std::optional<Identity> chop_id_of(const Game& game, int player) {
  auto chop = game.chop(player);
  if (!chop) return std::nullopt;
  return game.state.deck[*chop].id();
}

// H1b, negated. "Cathy's chop is not playable or critical" — judged from Alice's
// full visibility, the same viewpoint as `at_risk_chop`, so that every term in
// `clue_tier` reads from one view. Vacuously true when Cathy has no chop.
bool chop_is_playable_or_critical(const Game& game, int player) {
  auto id = chop_id_of(game, player);
  if (!id) return false;
  const State& s = game.state;
  return s.is_playable(*id) || s.is_critical(*id);
}

// "Cathy's chop is either a trash or a same-hand-dupe" — the expendable-chop
// half of H1c, and the negated guard of VH1.
bool chop_is_expendable(const Game& game, int player) {
  auto chop = game.chop(player);
  if (!chop) return false;
  auto id = game.state.deck[*chop].id();
  if (!id) return false;
  return game.state.is_basic_trash(*id) ||
         chop_is_free_chuck(game.state, chop) ||
         has_same_hand_dupe(game.state, player, *chop, *id);
}

// VH1's "the clue gets a finesse": the interpretation is reactive rank Phase B
// (`interpret_reactive.cpp:485-575`), the blind-play phase that calls the
// reacter onto a prerequisite for a one-away card in the receiver's hand.
//
// Phase B is invisible to a walk over `hypo.meta`, because it stamps ONLY the
// reacter — the receiver's target is stamped a turn later when the reaction
// resolves. So the detector recovers the receiver's promised order from the
// waiting connection (`predicted_receiver_order`) and asks two questions Phase B
// answers uniquely: the receiver's card is still unstamped (Phase A and colour
// mode 1 stamp it via `stamp_receiver_play`), and it is exactly one away from
// playable (`:390`).
//
// The WC freshness guard matters: `Game::interpret_clue` clears `waiting` only
// when the new clue's giver was the pending reacter (`decide.cpp:51-53`), so a
// stale connection from an earlier turn can survive into a candidate's hypo.
// It is `wc_is_fresh` (reactor0/interpret_reaction.h), shared with the decision
// layer's classifier — this detector originally inlined the same check with an
// EXACT turn compare, which never matches and left the rule dead from the commit that
// introduced it. See that helper's comment for why the compare is `>=`.
bool clue_gets_finesse(const Game& game, const Game& hypo,
                       const ClueAction& action) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  // Stable clues carry no finesse. Still correct under target parity, where a
  // clue to Bob is not stable at all -- but is ODD parity, and a finesse is
  // Phase B of the EVEN ruleset, so it cannot be one either way.
  if (action.target == bob) return false;
  // Phase B is not RANK-only -- it belongs to a RULESET, not a clue kind. It
  // lives in `reactive_rank`, which is the EVEN-parity family; Odds and Evens
  // makes that the colour clue, and `/set` can move an individual clue. Reading
  // the kind here made VH1 unreachable in those variants, so a finesse was
  // invisible to the pre-check that outranks everything else (replay 1967416
  // T1: yellow to Cathy was the finesse, and the bot clued yellow to Bob).
  if (!reactive_assignment_for(*s.variant, game.reactive_overrides,
                               action.clue.kind, action.clue.value,
                               /*target_is_bob=*/action.target == bob)
           .even) {
    return false;
  }
  // The RECEIVER, not the clued seat -- see `read_clue`.
  if (!wc_is_fresh(game, hypo, alice, reactive_receiver(s, action, bob), bob)) {
    return false;
  }
  // A reactive LOCK is not a finesse. It stamps CHOP_MOVED only a turn later,
  // in `reactive_lock`, so at clue time the receiver's predicted slot carries
  // no status at all -- which sails straight past the "stamped: Phase A, not B"
  // test below. If that slot happens to hold a one-away card, a lock then reads
  // as a finesse and, because VERY HIGH outranks a pending reaction, it lets Alice
  // abandon a reaction to give it. That is replay 1966091 T10.
  if (predicts_reactive_lock(hypo)) return false;
  auto receive_order = predicted_receiver_order(hypo);
  if (!receive_order) return false;
  // Phase B's own two gates, from the phase that produces it
  // (interpret_reactive.cpp:492, :519-520), rather than "is the receiver
  // unstamped".
  //
  // Not quite verbatim any more: since v10.10.0 Phase B asks the connector of
  // BOTH `effective_possible_for` and `possibilities()`, so it is the stricter
  // of the two. That direction is the safe one here -- this runs on the hypo,
  // where a Phase B that refused left no reacter CTP for the status test below
  // to find.
  //
  // That test used to mean "Phase A and colour mode 1 stamp the receiver at
  // clue time, Phase B does not". Since v8.0.0 no reactive stamps the receiver
  // at clue time (§1d), so it discriminates nothing. It was also never quite
  // right: a Phase C dc-target that happens to sit one away from playable
  // sailed past it, and read as a finesse.
  //
  // The reacter's button is the sharp edge -- Phase B calls a blind PLAY, Phase
  // C a discard -- and the connector test is what makes it a finesse at all.
  auto id = s.deck[*receive_order].id();
  if (!id || s.playable_away(*id) != 1) return false;
  const ReactorWC& wc = hypo.waiting.front();
  if (wc.react_order < 0) return false;
  if (hypo.meta[wc.react_order].status != CardStatus::CALLED_TO_PLAY) return false;
  // Direction-aware: on a reversed suit the connector is rank+1, not rank-1.
  auto prev_id = variants::connector_of(s, *id);
  if (!prev_id) return false;
  return hanabi::reactor::effective_possible_for(hypo, wc.react_order)
      .contains(*prev_id);
}

// VH1, as a named predicate: the sole member of `ClueTier::VERY_HIGH`, the tier
// that outranks a pending reaction.
//
// NOTE ON THE NAME. Through v9.2.0 this rule was called **H4** and sat inside
// HIGH, with Precedence step 1 singling it out by name. v9.3.0 gave the tier it
// actually earns a name instead, and freed "H4" to denote a DIFFERENT rule --
// the critical-chop rule in `clue_tier`. An "H4" in older commits, logs or
// comments means the finesse; an "H4" today does not.
//
// Kept separate from `clue_tier` so a caller can ask about the finesse itself
// rather than about the tier.
IdentitySet sight_narrowed(const Game& game, int order) {
  const State& s = game.state;
  if (order < 0 || order >= static_cast<int>(game.common.thoughts.size())) {
    return IdentitySet::empty();
  }
  const IdentitySet base = game.common.thoughts[order].possible;
  return base.filter([&s, order](Identity i) {
    const int ord = i.to_ord();
    // Copies still unaccounted for: total, minus those already played or
    // discarded.
    const int remaining = s.card_count[ord] - s.base_count[ord];
    if (remaining <= 0) return false;  // every copy is already gone
    int seen = 0;
    for (int p = 0; p < s.num_players; ++p) {
      // Never our OWN hand: we cannot see it. In production `deck[o].id()` is
      // already nullopt there, but saying so explicitly keeps the rule true
      // when a caller hands us a fully-populated deck.
      if (p == s.our_player_index) continue;
      for (int o : s.hands[p]) {
        if (o == order) continue;
        if (s.deck[o].id() == i) ++seen;
      }
    }
    return seen < remaining;  // at least one copy this seat cannot place
  });
}

bool provably_trash(const Game& game, int order) {
  const IdentitySet live = sight_narrowed(game, order);
  if (live.is_empty()) return false;  // no information, not a proof
  const State& s = game.state;
  return live.forall([&s](Identity i) { return s.is_basic_trash(i); });
}

bool clue_is_vh1(const Game& game, const Game& hypo, const ClueAction& action) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (bob == alice) return false;  // solo
  const int cathy_seat = s.next_player_index(bob);
  const bool has_cathy = cathy_seat != alice;
  return (!has_cathy || !chop_is_expendable(game, cathy_seat)) &&
         clue_gets_finesse(game, hypo, action);
}

ClueTier clue_tier(const Game& game, const Game& hypo,
                   const ClueAction& action) {
  const State& s = game.state;
  // Alice is the seat deciding. `alice_provably_holds` reads `game.me()`, so
  // the two must agree; the gate bails out otherwise (see `eval_action`).
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (bob == alice) return ClueTier::HIGH;  // solo — never gate

  const NewPlayFacts f = new_play_facts(game, hypo);
  const int cathy_seat = s.next_player_index(bob);
  const bool has_cathy = cathy_seat != alice;

  // Bob has nothing safe to do: no obvious play, and no known trash (which
  // covers the CTD case -- see `thinks_trash`, player_game.cpp:115-132). Shared
  // by H1a and H4a, which differ only in how bad the chop they are protecting
  // is: H1a wants it ENDANGERED, H4a wants it outright CRITICAL.
  const bool bob_stuck = !game.common.thinks_locked(game, bob) &&
                         game.common.obvious_playables(game, bob).empty() &&
                         game.common.thinks_trash(game, bob).empty();
  // H1b, and H4b -- the same clause under two names. Rescuing Bob is only
  // worth a token when Cathy is not herself about to lose something. Vacuous at
  // two seats.
  const bool cathy_can_wait =
      !has_cathy || !chop_is_playable_or_critical(game, cathy_seat);

  // --- very high --------------------------------------------------------
  //
  // The one clue that out-ranks a PENDING REACTION. Tested before HIGH because
  // the ladder returns on the first match, so a VERY HIGH clue that also
  // satisfies H1 must not be reported as merely HIGH.

  // VH1 — a finesse, when Cathy's chop is worth keeping. A blind play is worth
  // the tempo only if it is not being bought at the cost of Cathy's chop.
  if (clue_is_vh1(game, hypo, action)) return ClueTier::VERY_HIGH;

  // --- high -------------------------------------------------------------

  // H1a — Bob is stuck on an ENDANGERED chop.
  //
  // DECISION_MAKING.md lists H1a as a NOT-LOW condition in its own right, so
  // this position should read at least MEDIUM even when H1b/H1c fail. There is
  // no such arm below and there never has been: `h1a` is read once, on the next
  // line. TODO.md 30 -- left as-is here because widening the gate is a change
  // that wants its own measurement.
  const bool h1a = bob_stuck && at_risk_chop(game, alice, bob);
  // H1 — H1a AND H1b AND H1c. H1c is about CATHY too: rescuing Bob's chop is
  // only HIGH when Bob could not have handled Cathy himself. Vacuous at two
  // seats.
  if (h1a && cathy_can_wait) {
    // H1c — Cathy's chop is expendable, or Bob has no colour stable play clue
    // for her. Evaluated last: `has_colour_play_clue_for` is the costly term.
    const bool h1c = !has_cathy || chop_is_expendable(game, cathy_seat) ||
                     !has_colour_play_clue_for(game, bob, cathy_seat);
    if (h1c) return ClueTier::HIGH;
  }
  // H2 — a critical low card gets played.
  if (f.critical_low) return ClueTier::HIGH;
  // H3 — two new plays, at least one regaining a clue token.
  if (f.count >= 2 && f.regain_rank) return ClueTier::HIGH;
  // H4 — Bob is stuck on a CRITICAL chop and Cathy can wait. A critical card
  // discarded is gone for the rest of the game, so every clue this turn is worth
  // at least a token.
  //
  // HIGH and not VERY HIGH, deliberately. Like H1 and N5 this is a property of
  // the POSITION rather than of the candidate, so it lifts every legal clue that
  // turn; at VERY HIGH it would therefore fire Precedence step 1 unconditionally
  // and out-rank the pending reaction, phase 1 and phase 2 together. Measured
  // over the corpus turns that action a reaction, that moved 171 of 3332 -- 137
  // of them giving up a known play, including replay 1970589 T42's p3. At HIGH
  // it only widens what `clue_is_admissible` will pass, which is the intent.
  if (bob_stuck && cathy_can_wait) {
    auto bob_chop = chop_id_of(game, bob);
    if (bob_chop && s.is_critical(*bob_chop)) return ClueTier::HIGH;
  }

  // --- not low ----------------------------------------------------------
  // N5 — Bob has a playable chop he cannot just pitch a duplicate of. Any
  // clue is then worth at least MEDIUM: the team is expecting that card to
  // be saved or played, and suppressing every clue here is how replay
  // 1942330 T33 ended up giving a LOCK by arbitrary tie-break.
  if (has_playable_chop(game, bob)) return ClueTier::MEDIUM;

  if (has_cathy && at_risk_chop(game, alice, cathy_seat)) {
    // N3 — two new plays.
    if (f.count >= 2) return ClueTier::MEDIUM;
    // N2 — a reactive clue, when Bob could not simply push Cathy's play
    // himself. Asked through the dispatch predicate rather than by hand:
    // "reactive" is "not aimed at Bob" only while dispatch is positional, and
    // under target parity EVERY clue is reactive, a clue to Bob included.
    // Evaluated last: it is the only expensive term in this function.
    if (clue_is_reactive(s, action, bob) &&
        !has_colour_play_clue_for(game, bob, cathy_seat)) {
      return ClueTier::MEDIUM;
    }
  }
  return ClueTier::LOW;
}

}  // namespace hanabi::reactor0
