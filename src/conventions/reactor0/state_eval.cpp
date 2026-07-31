#include "hanabi/conventions/reactor0/state_eval.h"

#include <variant>
#include <vector>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
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
bool at_risk_chop(const Game& game, int alice, int player) {
  const State& s = game.state;
  auto chop = game.chop(player);
  if (!chop) return false;  // locked hand — no chop to lose
  auto id = s.deck[*chop].id();
  if (!id) return false;  // unknown from our POV — cannot verify
  if (s.is_basic_trash(*id)) return false;

  // Same-hand duplicate: the holder can pitch one copy safely.
  for (int o : s.hands[player]) {
    if (o == *chop) continue;
    auto other = s.deck[o].id();
    if (other && *other == *id) return false;
  }
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

// What the candidate clue achieves, read off the CTP stamps its
// interpretation produced. Same walk as reactor's `is_high_value_clue`
// (reactor/state_eval.cpp:126-142).
struct NewPlayFacts {
  int count = 0;
  bool critical_low = false;  // a critical 1 or 2 (5 or 4 reversed) plays
  bool regain_rank = false;   // at least one play regains a clue token
};

NewPlayFacts new_play_facts(const Game& game, const Game& hypo) {
  const State& s = game.state;
  NewPlayFacts f;
  for (const auto& hand : s.hands) {
    for (int o : hand) {
      if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
      if (hypo.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      ++f.count;
      auto id = s.deck[o].id();
      if (!id) continue;
      if (s.is_critical(*id) && variants::is_first_or_second_rank(s, *id)) {
        f.critical_low = true;
      }
      if (variants::is_clue_regain_rank(s, *id)) f.regain_rank = true;
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

}  // namespace

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

ClueTier clue_tier(const Game& game, const Game& hypo,
                   const ClueAction& action) {
  const State& s = game.state;
  // Alice is the seat deciding. `alice_provably_holds` reads `game.me()`, so
  // the two must agree; the gate bails out otherwise (see `eval_action`).
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (bob == alice) return ClueTier::HIGH;  // solo — never gate

  const NewPlayFacts f = new_play_facts(game, hypo);

  // H1 / N1 — Bob has nothing safe to do and a chop worth saving.
  if (!game.common.thinks_locked(game, bob)) {
    const bool bob_safe = !game.common.obvious_playables(game, bob).empty() ||
                          !game.common.thinks_trash(game, bob).empty();
    if (!bob_safe && at_risk_chop(game, alice, bob)) return ClueTier::HIGH;
  }
  // H2 — a critical low card gets played.
  if (f.critical_low) return ClueTier::HIGH;
  // H3 — two new plays, at least one regaining a clue token.
  if (f.count >= 2 && f.regain_rank) return ClueTier::HIGH;

  // --- not low ----------------------------------------------------------
  const int cathy = s.next_player_index(bob);
  if (cathy != alice && at_risk_chop(game, alice, cathy)) {
    // N3 — two new plays.
    if (f.count >= 2) return ClueTier::MEDIUM;
    // N2 — a reactive clue, when Bob could not simply push Cathy's play
    // himself. Reactor0's dispatch is positional, so "reactive" is just
    // "not aimed at Bob" (interpret_clue.cpp:318-329). Evaluated last: it
    // is the only expensive term in this function.
    if (action.target != bob && !has_colour_play_clue_for(game, bob, cathy)) {
      return ClueTier::MEDIUM;
    }
  }
  return ClueTier::LOW;
}

double eval_action(const Game& game, const Action& action) {
  // Plays and discards score exactly as they do under reactor.
  if (!std::holds_alternative<ClueAction>(action)) {
    return hanabi::reactor::eval_action(game, action);
  }
  hanabi::instr::ScopedTimer st("reactor0.eval_action");
  hanabi::logging::LogScope ls("reactor0.eval_action");

  const State& state = game.state;
  const auto& ca = std::get<ClueAction>(action);
  Game hypo_game = game.simulate(action);

  auto m = hypo_game.last_move();
  if (m && std::holds_alternative<ClueInterp>(*m) &&
      std::get<ClueInterp>(*m) == ClueInterp::MISTAKE) {
    return -100.0;
  }

  // The pace-clue tier gate. With pace to spare and the clue supply running
  // down, a clue has to earn its token: HIGH only when Alice already holds a
  // call she can fall back on, otherwise anything but LOW.
  // `clue_tier` reasons as the giver about its own hand via `game.me()`, so
  // it is only meaningful when the giver is the POV seat. During `take_action`
  // that always holds; anything else fails open (no gate).
  if (state.pace() >= 3 && state.clue_tokens <= 3 &&
      ca.giver == state.our_player_index) {
    const ClueTier need =
        requires_high_tier(game) ? ClueTier::HIGH : ClueTier::MEDIUM;
    const ClueTier tier = clue_tier(game, hypo_game, ca);
    if (tier < need) {
      hanabi::logging::log_branch("reactor0.pace_clue_gate",
                                  {{"clue_tokens", state.clue_tokens},
                                   {"pace", state.pace()},
                                   {"target", ca.target},
                                   {"required", tier_name(need)},
                                   {"actual", tier_name(tier)},
                                   {"rejected", true}});
      return -1.0;
    }
  }

  auto playables_us = game.me().obvious_playables(game, state.our_player_index);
  double value = hanabi::reactor::clue_branch_value(game, hypo_game, ca,
                                                    !playables_us.empty());
  return value + hanabi::reactor::advance(game, hypo_game, 1);
}

}  // namespace hanabi::reactor0
