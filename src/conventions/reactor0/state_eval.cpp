#include "hanabi/conventions/reactor0/state_eval.h"

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
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/conventions/reactor0/interpret_clue.h"
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

bool at_risk_chop(const Game& game, int alice, int player) {
  const State& s = game.state;
  auto chop = game.chop(player);
  if (!chop) return false;  // locked hand — no chop to lose
  auto id = s.deck[*chop].id();
  if (!id) return false;  // unknown from our POV — cannot verify
  if (s.is_basic_trash(*id)) return false;

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
  return !has_same_hand_dupe(s, player, *chop, *id);
}

// What the candidate clue achieves, read off the CTP stamps its
// interpretation produced. Same walk as reactor's `is_high_value_clue`
// (reactor/state_eval.cpp:126-142).
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


// --- Clue scoring, reactor0's own ----------------------------------------
// Scheduled for deletion in v7.0.0; see DECISION_MAKING.md and PLAN.md.
//
// Ported from `reactor::get_result` (reactor/state_eval.cpp:152-291) so the
// two conventions can diverge. The structure is deliberately kept
// line-for-line comparable with reactor's; the ONLY intentional differences
// are the two bad-touch constants below.
//
// Why: reactor charges a clue twice for touching a card that turns out to be
// trash — once by shrinking the `good_touch` bonus (it indexes the table on
// `new_touched - bad_touch`) and again through a flat `-0.1 * bad_touch`.
// Under reactor0 that made a clue which *gets a card played* score below
// several clues that achieve nothing, purely because the playing clue also
// swept up some trash. Replay 1942181 T41 is the worked example: Blue to Bob
// plays a Berry 5 off a stack of 4 but touches two dead Aqua cards, and it
// finished behind a referential discard, a play-less reveal, and a
// zero-play double discard — all within 0.14 of each other.
//
// Bad touch still costs, but only through the terms that already model it:
// the touched cards are dead weight in the receiver's hand, and the
// all-bad-touch-with-no-playables reading is still hard-rejected below.
namespace {

// reactor uses 0.1. Charging nothing here is what lets a clue that creates a
// play outrank a clean clue that creates none.
constexpr double kBadTouchPenalty = 0.0;
// When true, `good_touch` indexes on `new_touched - bad_touch` as reactor
// does, so trash never earns the touch bonus; it simply stops being charged
// for a second time. Keeping this true avoids the opposite failure — a bot
// that sprays clues because touching trash is free.
constexpr bool kGoodTouchDiscountsBadTouch = true;

}  // namespace

double get_result(const Game& game, const Game& hypo, const ClueAction& action) {
  const State& state = game.state;
  const Player& common = game.common;
  const auto& meta = game.meta;

  auto [new_touched, fill, elim] = hanabi::elim_result(
      game, hypo, hypo.state.hands[action.target], action.list_);
  auto [bad_touch, trash, _] = hanabi::bad_touch_result(game, hypo, action);
  auto [_blind_plays, playables] = hanabi::playables_result(game, hypo);

  int revealed_trash = 0;
  for (int o : hypo.common.thinks_trash(hypo, action.target)) {
    if (!hypo.state.deck[o].clued) continue;
    auto game_trash = common.thinks_trash(game, action.target);
    if (std::find(game_trash.begin(), game_trash.end(), o) == game_trash.end()) {
      ++revealed_trash;
    }
  }

  std::vector<int> new_playables;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (meta[o].status != CardStatus::CALLED_TO_PLAY &&
          hypo.meta[o].status == CardStatus::CALLED_TO_PLAY) {
        new_playables.push_back(o);
      }
    }
  }
  for (int o : new_playables) {
    bool ok = hypo.me().hypo_plays.count(o) > 0;
    if (!ok && game.in_endgame()) {
      auto id = state.deck[o].id();
      ok = id && state.is_playable(*id);
    }
    if (!ok) return -100.0;
  }

  auto move = hypo.last_move();
  auto move_is = [&](ClueInterp ci) {
    return move && std::holds_alternative<ClueInterp>(*move) &&
           std::get<ClueInterp>(*move) == ci;
  };

  if (move_is(ClueInterp::PLAY) && playables.empty() && !game.in_endgame()) {
    return -100.0;
  }
  if (move_is(ClueInterp::REVEAL) && playables.empty() && !trash.empty()) {
    bool all_clued = true;
    for (int o : trash) {
      if (!state.deck[o].clued) {
        all_clued = false;
        break;
      }
    }
    if (all_clued) return -100.0;
  }
  // Retained in full: dropping the bad-touch *penalty* must not make a clue
  // that only buries trash and creates nothing look acceptable.
  if (!move_is(ClueInterp::REACTIVE) && !bad_touch.empty()) {
    bool all_in_bad = true;
    for (int o : new_touched) {
      if (std::find(bad_touch.begin(), bad_touch.end(), o) == bad_touch.end()) {
        all_in_bad = false;
        break;
      }
    }
    if (all_in_bad && playables.empty()) return -100.0;
  }

  int duped_playables = 0;
  for (int p : hypo.me().hypo_plays) {
    if (state.deck[p].clued) continue;
    bool dup = false;
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if (o == p) continue;
        if (game.is_touched(o) && state.deck[o].matches(state.deck[p])) {
          dup = true;
          break;
        }
      }
      if (dup) break;
    }
    if (dup) ++duped_playables;
  }

  const int new_touched_count = static_cast<int>(new_touched.size());
  const int bad_count = static_cast<int>(bad_touch.size());
  double good_touch;
  if (kGoodTouchDiscountsBadTouch && bad_count > new_touched_count) {
    good_touch = -static_cast<double>(bad_count);
  } else {
    static constexpr double table[] = {0.0, 0.125, 0.25, 0.35, 0.45, 0.55};
    const int touched =
        kGoodTouchDiscountsBadTouch ? new_touched_count - bad_count
                                    : new_touched_count;
    good_touch = table[std::min(touched, 5)];
  }

  int untouched_plays = 0;
  for (int o : playables) {
    if (!hypo.state.deck[o].clued) ++untouched_plays;
  }

  // As reactor: a reading that flips a queued CALLED_TO_PLAY into a
  // CALLED_TO_DISCARD destroys that play and whatever chains behind it.
  int destroyed_plays = 0;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      if (hypo.meta[o].status != CardStatus::CALLED_TO_DISCARD) continue;
      auto id = state.deck[o].id();
      if (id && state.is_basic_trash(*id)) continue;
      ++destroyed_plays;
    }
  }

  double value =
      good_touch +
      (static_cast<double>(playables.size()) - 2.0 * duped_playables) +
      0.2 * untouched_plays +
      (game.in_endgame() ? 0.01 : 0.05) * revealed_trash +
      (game.in_endgame() ? 0.1 : 0.05) * static_cast<double>(fill.size()) +
      (game.in_endgame() ? 0.05 : 0.02) * static_cast<double>(elim.size()) +
      -kBadTouchPenalty * bad_count - 10.0 * destroyed_plays;

  if (move_is(ClueInterp::MISTAKE)) return value - 10.0;
  if (move_is(ClueInterp::FIX)) return value + 1.0;
  // Reactor0's rank double play is the only 2-play reactive shape; colour
  // reactives are 1-play by construction, so this fires exactly where
  // reactor's does.
  if (move_is(ClueInterp::REACTIVE) && playables.size() >= 2) value += 10.0;
  return value;
}

double clue_branch_value(const Game& game, const Game& hypo,
                         const ClueAction& ca, bool we_hold_a_play) {
  const double mult = !we_hold_a_play ? 0.5 : (game.in_endgame() ? 0.1 : 0.25);
  const double result = get_result(game, hypo, ca);
  return result * (result > 0 ? mult : 1.0) - 0.5;
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

// --- Cathy-chop facts, and the finesse detector (DECISION_MAKING.md H1b/H1c/H4)

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
// half of H1c, and the negated guard of H4.
bool chop_is_expendable(const Game& game, int player) {
  auto chop = game.chop(player);
  if (!chop) return false;
  auto id = game.state.deck[*chop].id();
  if (!id) return false;
  return game.state.is_basic_trash(*id) ||
         has_same_hand_dupe(game.state, player, *chop, *id);
}

// H4's "the clue gets a finesse": the interpretation is reactive rank Phase B
// (`interpret_reactive.cpp:383-447`), the blind-play phase that calls the
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
bool clue_gets_finesse(const Game& game, const Game& hypo,
                       const ClueAction& action) {
  const State& s = game.state;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  if (action.target == bob) return false;             // stable: no finesse
  if (action.clue.kind != ClueKind::RANK) return false;  // Phase B is rank-only
  if (hypo.waiting.empty()) return false;
  const ReactorWC& wc = hypo.waiting.front();
  if (wc.turn != s.turn_count || wc.giver != alice ||
      wc.receiver != action.target || wc.reacter != bob || wc.react_order < 0) {
    return false;                                     // stale or aborted WC
  }
  auto receive_order = predicted_receiver_order(hypo);
  if (!receive_order) return false;
  if (hypo.meta[*receive_order].status != game.meta[*receive_order].status) {
    return false;                                     // stamped: Phase A, not B
  }
  auto id = s.deck[*receive_order].id();
  return id && s.playable_away(*id) == 1;
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

  // H1a — Bob has nothing safe to do and a chop worth saving. Also feeds the
  // NOT-LOW test below, where it stands alone without H1b/H1c.
  bool h1a = false;
  if (!game.common.thinks_locked(game, bob)) {
    const bool bob_safe = !game.common.obvious_playables(game, bob).empty() ||
                          !game.common.thinks_trash(game, bob).empty();
    h1a = !bob_safe && at_risk_chop(game, alice, bob);
  }
  // H1 — H1a AND H1b AND H1c. H1b and H1c are about CATHY: rescuing Bob's chop
  // is only HIGH when Cathy is not herself about to lose something, and when
  // Bob could not have handled Cathy himself. Both are vacuous at two seats.
  if (h1a) {
    // H1b — Cathy's chop is not playable or critical.
    const bool h1b = !has_cathy || !chop_is_playable_or_critical(game, cathy_seat);
    // H1c — Cathy's chop is expendable, or Bob has no colour stable play clue
    // for her. Evaluated last: `has_colour_play_clue_for` is the costly term.
    const bool h1c = !has_cathy || chop_is_expendable(game, cathy_seat) ||
                     !has_colour_play_clue_for(game, bob, cathy_seat);
    if (h1b && h1c) return ClueTier::HIGH;
  }
  // H2 — a critical low card gets played.
  if (f.critical_low) return ClueTier::HIGH;
  // H3 — two new plays, at least one regaining a clue token.
  if (f.count >= 2 && f.regain_rank) return ClueTier::HIGH;
  // H4 — a finesse, when Cathy's chop is worth keeping. A blind play is worth
  // the tempo only if it is not being bought at the cost of Cathy's chop.
  if ((!has_cathy || !chop_is_expendable(game, cathy_seat)) &&
      clue_gets_finesse(game, hypo, action)) {
    return ClueTier::HIGH;
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
    // himself. Reactor0's dispatch is positional, so "reactive" is just
    // "not aimed at Bob" (interpret_clue.cpp:318-329). Evaluated last: it
    // is the only expensive term in this function.
    if (action.target != bob &&
        !has_colour_play_clue_for(game, bob, cathy_seat)) {
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

  // The pace-clue tier gate. With pace to spare a clue has to earn its token,
  // and how hard it has to work depends on whether Alice already holds a call
  // she can fall back on.
  //
  // The two requirements carry **different windows**, so they are decided
  // together:
  //   * Alice holds a call (`requires_high_tier`) → HIGH, at every clue count
  //     short of the forced-clue case. Holding something to do is exactly when
  //     the team can afford to wait for a clue worth the token, so the old
  //     `<= 3` cap was leaving value on the table between 4 and 7.
  //   * otherwise → the original `<= 3` window, requiring anything but LOW.
  //
  // The `< 8` bound is the forced-clue exemption: at 8 tokens discarding is
  // illegal, so gating there would flatten every non-HIGH clue to the same
  // `-1.0` and let the argmax break the tie arbitrarily, losing the ranking
  // precisely when the bot has no alternative to cluing.
  //
  // `clue_tier` reasons as the giver about its own hand via `game.me()`, so
  // it is only meaningful when the giver is the POV seat. During `take_action`
  // that always holds; anything else fails open (no gate).
  const bool high_tier_required = requires_high_tier(game);
  const bool in_gate_window =
      state.pace() >= 3 && ca.giver == state.our_player_index &&
      (high_tier_required ? state.clue_tokens < 8 : state.clue_tokens <= 3);
  if (in_gate_window) {
    const ClueTier need =
        high_tier_required ? ClueTier::HIGH : ClueTier::MEDIUM;
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
  double value = clue_branch_value(game, hypo_game, ca, !playables_us.empty());
  return value + hanabi::reactor::advance(game, hypo_game, 1);
}

// --- The pointless-double-discard filter ---------------------------------
// Scheduled for deletion in v7.0.0; see DECISION_MAKING.md and PLAN.md.

bool receiver_is_safe(const Game& game, int alice, int receiver) {
  // An expendable chop — trash, same-hand dupe, covered elsewhere, or no
  // chop at all because the hand is locked.
  if (!at_risk_chop(game, alice, receiver)) return true;
  // Known trash, which `order_trash` (player_game.cpp:115-132) already folds
  // together with CALLED_TO_DISCARD and PERMISSION_TO_DISCARD.
  if (!game.common.thinks_trash(game, receiver).empty()) return true;
  // A known play.
  if (!game.common.obvious_playables(game, receiver).empty()) return true;
  // An outstanding play call. Usually implied by the line above, but
  // `order_kp`'s CALLED_TO_PLAY branch can fail when the card's `possible`
  // no longer intersects the playable set — a card the convention told the
  // receiver to play is still an action they can take.
  for (int o : game.state.hands[receiver]) {
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) return true;
  }
  return false;
}

bool is_pointless_double_discard(const Game& game, const Game& hypo,
                                 const ClueAction& action) {
  const State& s = game.state;
  if (action.giver != s.our_player_index) return false;
  const int alice = s.our_player_index;
  const int bob = s.next_player_index(alice);
  // Positional dispatch: a clue to anyone but Bob is reactive (§1a).
  if (action.target == bob) return false;
  // Only a rank reactive can call zero plays. Colour mode 2 always stamps
  // the reacter's blind play, and colour mode 1 the receiver's
  // (interpret_reactive.cpp:450, :489-490).
  if (action.clue.kind != ClueKind::RANK) return false;
  // On an inverted (Orange) suit a *play* call is stamped CALLED_TO_DISCARD
  // (interpret_reactive.cpp:198-200), so `new_play_facts` — which counts
  // CALLED_TO_PLAY transitions only — would read a genuine two-play Phase A
  // as zero plays. Rather than mis-fire, sit the rule out in those variants.
  if (variants::includes_inverted(s)) return false;

  auto m = hypo.last_move();
  if (!m || !std::holds_alternative<ClueInterp>(*m) ||
      std::get<ClueInterp>(*m) != ClueInterp::REACTIVE) {
    return false;
  }
  if (new_play_facts(game, hypo).count != 0) return false;
  if (predicts_reactive_lock(hypo)) return false;
  return receiver_is_safe(game, alice, action.target);
}

bool is_stable_play_clue_for_bob(const Game& game, const Game& hypo,
                                 const ClueAction& action) {
  const State& s = game.state;
  if (action.giver != s.our_player_index) return false;
  if (action.target != s.next_player_index(s.our_player_index)) return false;
  auto m = hypo.last_move();
  if (!m || !std::holds_alternative<ClueInterp>(*m)) return false;
  const ClueInterp interp = std::get<ClueInterp>(*m);
  // PLAY covers both direct play readings (stable colour, stable rank
  // priority 1); REVEAL covers the play reveal, which stamps nothing and so
  // is only visible as a new playable. The interp test is what excludes a
  // referential discard, a LOCK or a FIX that happens to free up a playable
  // through elimination on an untouched card.
  if (interp != ClueInterp::PLAY && interp != ClueInterp::REVEAL) return false;
  return !hanabi::playables_result(game, hypo).second.empty();
}

void drop_pointless_double_discards(
    const Game& game, std::vector<std::pair<PerformAction, Action>>& clues) {
  hanabi::instr::ScopedTimer st("reactor0.dd_filter");
  if (clues.size() < 2) return;

  bool have_bob_play_clue = false;
  std::vector<size_t> pointless;
  for (size_t i = 0; i < clues.size(); ++i) {
    const Action& act = clues[i].second;
    if (!std::holds_alternative<ClueAction>(act)) continue;
    const auto& ca = std::get<ClueAction>(act);
    Game hypo = game.simulate(act);
    if (is_stable_play_clue_for_bob(game, hypo, ca)) {
      // Only an alternative we would actually be willing to give. A stable
      // play clue that the pace-clue tier gate rejects (-1.0) or that
      // `get_result` hard-rejects is no reason to drop anything — without
      // this check the bot could suppress the double discard and then fall
      // through to chucking its own chop.
      if (!have_bob_play_clue && eval_action(game, act) > 0.0) {
        have_bob_play_clue = true;
      }
      continue;
    }
    if (is_pointless_double_discard(game, hypo, ca)) pointless.push_back(i);
  }
  if (!have_bob_play_clue || pointless.empty()) return;

  hanabi::logging::log_branch("reactor0.dd_filter",
                              {{"dropped", pointless.size()},
                               {"candidates", clues.size()}});
  // Back to front, so the indices stay valid. The qualifying Bob clue is
  // itself still in `clues`, so this can never empty the pool.
  for (auto it = pointless.rbegin(); it != pointless.rend(); ++it) {
    clues.erase(clues.begin() + static_cast<std::ptrdiff_t>(*it));
  }
}

}  // namespace hanabi::reactor0
