#include "hanabi/conventions/reactor/state_eval.h"

#include <algorithm>
#include <variant>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/eval.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor {
namespace {

// Forward declarations.
double force_clue_inner(const Game& orig, const Game& game, int offset);

bool identity_called_to_play_elsewhere(const Game& game, Identity id,
                                       int exclude_order) {
  const State& s = game.state;
  for (const auto& hand : s.hands) {
    for (int o : hand) {
      if (o == exclude_order) continue;
      if (game.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      auto cid = game.me().thoughts[o].id(/*infer=*/true);
      if (cid && *cid == id) return true;
      auto did = s.deck[o].id();
      if (did && *did == id) return true;
    }
  }
  return false;
}

// True iff `chop_order` (the candidate discard) has a non-trash identity
// from the common-knowledge perspective. Treats unknown chop as non-trash
// (we don't know it's safe).
bool chop_is_nontrash(const Game& game, std::optional<int> chop_order) {
  if (!chop_order) return false;
  auto id = game.state.deck[*chop_order].id();
  if (!id) return true;
  return !game.state.is_basic_trash(*id);
}

// Low-clue-count gate. When `state.clue_tokens < 3` AND
// `state.pace() >= 3`, a clue is only "worth it" if it's high-value
// (the bot always has a pending play in this branch — has_real_play
// is the gate's conjoined guard, so the strict v0.34 predicate is the
// only one consulted). The caller penalises a non-high-value clue so
// any known play wins.
//
// v0.34 spec: a clue is HIGH VALUE iff ANY of:
//   (1) Bob is not locked AND Bob has no safe discard (no obvious
//       play, no known trash, no CTD) AND Bob's chop is a non-trash
//       card whose identity has NO other visible copy — i.e., no
//       same-id card in Cathy's hand AND no same-id card in the
//       giver's own hand whose common.thoughts inferred is the
//       singleton matching identity. "Unique good chop in danger".
//   (2) The clue gets a critical "low" card (suit-direction
//       sensitive) played. For normal suits that's rank 1 or 2; for
//       reversed suits (play direction 5→1) that's rank 5 or 4.
//   (3) The clue gets ≥ 2 new plays AND at least one of them is the
//       suit's "final" rank — rank 5 for normal suits, rank 1 for
//       reversed suits — i.e., a card that regains a clue token when
//       played.
//
// Notes:
// - "Bob safe (discard)" = Bob has an obvious play OR `thinks_trash`
//   anywhere (which includes CTD and known basic-trash).
// - Identity-direction for ranks uses `suit_type.reversed`. Orange's
//   `inverted` flag (play↔discard action swap) does NOT change the
//   stack direction or the regain-clue rank — only `reversed` does.
// - "New plays" = orders that transitioned from non-CTP to CTP
//   through this clue's interpretation in `hypo`.

bool chop_id_is_unique(const Game& game, int bob_chop_order) {
  const State& s = game.state;
  auto chop_id = s.deck[bob_chop_order].id();
  if (!chop_id) return false;  // unknown chop id from POV — can't verify
  int giver = s.current_player_index;
  int bob = s.next_player_index(giver);
  // Cathy's hand: any visible copy of the same identity disqualifies.
  for (int p = 0; p < s.num_players; ++p) {
    if (p == giver || p == bob) continue;
    for (int o : s.hands[p]) {
      auto id = s.deck[o].id();
      if (id && *id == *chop_id) return false;
    }
  }
  // Giver's own hand: a common-knowledge singleton-inferred copy of
  // the same identity disqualifies (giver knows they hold it).
  for (int o : s.hands[giver]) {
    const auto& inf = game.common.thoughts[o].inferred;
    if (inf.length() == 1 && inf.head() == *chop_id) return false;
  }
  return true;
}

bool is_clue_regain_rank(const State& s, Identity id) {
  bool reversed = s.variant->suits[id.suit_index].suit_type.reversed;
  return reversed ? (id.rank == 1) : (id.rank == 5);
}

bool is_first_or_second_rank(const State& s, Identity id) {
  bool reversed = s.variant->suits[id.suit_index].suit_type.reversed;
  if (reversed) return id.rank == 4 || id.rank == 5;
  return id.rank == 1 || id.rank == 2;
}

bool is_high_value_clue(const Game& game, const Game& hypo,
                        const ClueAction& /*ca*/) {
  const State& s = game.state;
  const Player& common = game.common;
  int giver = s.current_player_index;
  int bob = s.next_player_index(giver);
  if (bob == giver) return true;  // Solo — let it through.

  // Condition (1): Bob has a unique good chop in danger.
  if (!common.thinks_locked(game, bob)) {
    bool bob_safe_discard = !common.obvious_playables(game, bob).empty() ||
                             !common.thinks_trash(game, bob).empty();
    if (!bob_safe_discard) {
      auto bob_chop = game.chop(bob);
      if (bob_chop && chop_is_nontrash(game, bob_chop) &&
          chop_id_is_unique(game, *bob_chop)) {
        return true;
      }
    }
  }

  // Conditions (2) and (3) walk newly-CTP'd cards from this clue.
  int new_plays = 0;
  bool any_clue_regain_rank = false;
  for (const auto& hand : s.hands) {
    for (int o : hand) {
      if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
      if (hypo.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      ++new_plays;
      auto id = s.deck[o].id();
      if (!id) continue;
      // (2) Critical first/second-rank in play direction.
      if (s.is_critical(*id) && is_first_or_second_rank(s, *id)) return true;
      if (is_clue_regain_rank(s, *id)) any_clue_regain_rank = true;
    }
  }
  // (3) ≥ 2 plays AND at least one regains a clue token.
  if (new_plays >= 2 && any_clue_regain_rank) return true;
  return false;
}

}  // namespace

// --- get_result ----------------------------------------------------------

double get_result(const Game& game, const Game& hypo, const ClueAction& action) {
  const State& state = game.state;
  const Player& common = game.common;
  const auto& meta = game.meta;

  auto [new_touched, fill, elim] =
      elim_result(game, hypo, hypo.state.hands[action.target], action.list_);
  auto [bad_touch, trash, _] = bad_touch_result(game, hypo, action);
  auto [_blind_plays, playables] = playables_result(game, hypo);

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

  if (move_is(ClueInterp::PLAY) && playables.empty() && !game.in_endgame()) return -100.0;
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

  int new_touched_count = static_cast<int>(new_touched.size());
  int bad_count = static_cast<int>(bad_touch.size());
  double good_touch;
  if (bad_count > new_touched_count) {
    good_touch = -static_cast<double>(bad_count);
  } else {
    static constexpr double table[] = {0.0, 0.125, 0.25, 0.35, 0.45, 0.55};
    int delta = std::min(new_touched_count - bad_count, 5);
    good_touch = table[delta];
  }

  int untouched_plays = 0;
  for (int o : playables) {
    if (!hypo.state.deck[o].clued) ++untouched_plays;
  }

  double value =
      good_touch + (static_cast<double>(playables.size()) - 2.0 * duped_playables) +
      0.2 * untouched_plays +
      (game.in_endgame() ? 0.01 : 0.05) * revealed_trash +
      (game.in_endgame() ? 0.1 : 0.05) * static_cast<double>(fill.size()) +
      (game.in_endgame() ? 0.05 : 0.02) * static_cast<double>(elim.size()) +
      -0.1 * bad_count;

  if (move_is(ClueInterp::MISTAKE)) return value - 10.0;
  if (move_is(ClueInterp::FIX)) return value + 1.0;
  if (move_is(ClueInterp::REACTIVE) && playables.size() >= 2) {
    // Strongly prefer a reactive that gets 2+ plays over a reactive
    // (or any other clue) that gets only 1. The +1.0 from the extra
    // `playables.size()` term and +0.125 from `good_touch` are too
    // small to survive the `result * mult - 0.5` damping in
    // eval_action's clue branch — competing 1-play clues can win on
    // the advance() lookahead. This flat bonus is sized to dominate
    // realistic alternatives even after damping (mult=0.25 mid-game
    // → +2.5; mult=0.1 endgame → +1.0).
    value += 10.0;
  }
  return value;
}

namespace {

// Build a DiscardAction for a known order in advance()'s simulation. For
// inverted (Orange / Dark Orange) suits the engine's `on_discard` runs the
// orange game-rule: failed=false advances the stack (via `with_play`),
// failed=true strikes. If we hard-code `failed=false` for non-playable
// inverted cards, `with_play` jumps the play stack to the (non-playable)
// rank — corrupting the simulated state. So inverted + !playable must
// use failed=true.
DiscardAction make_discard_for_simulation(const State& state, int player_index,
                                            int order) {
  auto id = state.deck[order].id();
  if (!id) return DiscardAction{player_index, order, -1, -1, /*failed=*/false};
  bool inverted = state.variant->suits[id->suit_index].suit_type.inverted;
  bool playable = state.is_playable(*id);
  bool failed = inverted && !playable;
  return DiscardAction{player_index, order, id->suit_index, id->rank, failed};
}

double force_clue_inner(const Game& orig, const Game& game, int offset) {
  const State& state = game.state;
  int giver = (state.our_player_index + offset) % state.num_players;
  int bob = state.next_player_index(giver);

  if (bob == state.our_player_index) {
    Game next = game;
    --next.state.clue_tokens;
    return advance(orig, next, offset + 1) + 1.0;
  }

  auto advance_fn = [&](const Game& g) { return advance(orig, g, offset + 1); };
  return force_clue(game, giver, advance_fn, /*only=*/bob) + 0.5;
}

}  // namespace

// --- advance -------------------------------------------------------------

double advance(const Game& orig, const Game& game, int offset) {
  hanabi::instr::ScopedTimer st("reactor.advance");
  const State& state = game.state;
  const Player& common = game.common;
  const auto& meta = game.meta;
  int player_index = (state.our_player_index + offset) % state.num_players;
  const Player& player = game.players[player_index];

  int bob = state.next_player_index(player_index);
  auto bob_chop = (state.num_players != 2) ? game.chop(bob) : std::nullopt;
  (void)bob_chop;

  std::vector<int> trash = player.thinks_trash(game, player_index);
  std::optional<int> urgent_dc;
  for (int o : trash) {
    if (meta[o].urgent) {
      urgent_dc = o;
      break;
    }
  }
  std::vector<int> all_playables = player.obvious_playables(game, player_index);

  if (player_index == state.our_player_index ||
      (state.endgame_turns && *state.endgame_turns == 0)) {
    return eval_game(orig, game);
  }

  if (!urgent_dc && !all_playables.empty()) {
    std::optional<int> urgent_play;
    for (int o : all_playables) {
      if (meta[o].urgent) {
        urgent_play = o;
        break;
      }
    }
    std::vector<int> playables;
    if (urgent_play) {
      playables.push_back(*urgent_play);
    } else {
      for (int o : all_playables) {
        bool dominated = false;
        for (int p : all_playables) {
          if (p > o && common.thoughts[p].possible == common.thoughts[o].possible) {
            dominated = true;
            break;
          }
        }
        if (!dominated) playables.push_back(o);
      }
    }

    bool strike = false;
    std::vector<double> play_values;
    for (int order : playables) {
      auto id = state.deck[order].id();
      Action act;
      if (!id) {
        act = PlayAction{player_index, order, -1, -1};
      } else if (state.is_playable(*id)) {
        act = PlayAction{player_index, order, id->suit_index, id->rank};
      } else {
        act = DiscardAction{player_index, order, id->suit_index, id->rank, true};
      }
      Game advanced = game.simulate(act);
      if (advanced.state.strikes > game.state.strikes) strike = true;
      play_values.push_back(advance(orig, advanced, offset + 1));
    }

    if (strike) {
      return *std::min_element(play_values.begin(), play_values.end());
    }
    double best_play = *std::max_element(play_values.begin(), play_values.end());
    return std::max(best_play, force_clue_inner(orig, game, offset));
  }

  if (player.obvious_locked(game, player_index)) {
    if (!state.can_clue()) {
      int locked_dc = player.locked_discard(state, player_index);
      Action act{make_discard_for_simulation(state, player_index, locked_dc)};
      return advance(orig, game.simulate(act), offset + 1);
    }
    return force_clue_inner(orig, game, offset);
  }

  if (state.clue_tokens == 8) return force_clue_inner(orig, game, offset);

  if (urgent_dc) {
    Action act{make_discard_for_simulation(state, player_index, *urgent_dc)};
    return advance(orig, game.simulate(act), offset + 1);
  }

  auto try_discard = [&](int order) {
    Action act{make_discard_for_simulation(state, player_index, order)};
    double dc_value = advance(orig, game.simulate(act), offset + 1);
    if (state.clue_tokens < 2) return dc_value;
    double clue_value = force_clue_inner(orig, game, offset);
    double clue_prob;
    if (offset == 1) {
      if (common.obvious_loaded(game, bob)) {
        clue_prob = 0.2;
      } else if (bob_chop) {
        auto bob_chop_id = state.deck[*bob_chop].id();
        clue_prob = (bob_chop_id && state.is_basic_trash(*bob_chop_id)) ? 0.2 : 0.7;
      } else {
        clue_prob = 0.5;
      }
    } else {
      clue_prob = 0.8;
    }
    if (clue_value < dc_value) return dc_value;
    return clue_prob * clue_value + (1.0 - clue_prob) * dc_value;
  };

  int order;
  if (urgent_dc) {
    order = *urgent_dc;
  } else if (!trash.empty()) {
    order = trash.front();
  } else {
    Game check_game = game;
    check_game.state.current_player_index = player_index;
    if (offset == 1 && !check_game.has_ptd()) {
      return force_clue_inner(orig, game, offset);
    }
    auto chop = game.chop(player_index);
    if (chop) {
      order = *chop;
    } else {
      // Defensive fallback.
      order = player.locked_discard(state, player_index);
    }
  }
  return try_discard(order);
}

// --- eval_action ---------------------------------------------------------

double eval_action(const Game& game, const Action& action) {
  hanabi::instr::ScopedTimer st("reactor.eval_action");
  hanabi::logging::LogScope ls("reactor.eval_action");
  const State& state = game.state;
  Game hypo_game = game.simulate(action);

  bool mistake = false;
  if (std::holds_alternative<ClueAction>(action)) {
    auto m = hypo_game.last_move();
    if (m && std::holds_alternative<ClueInterp>(*m) &&
        std::get<ClueInterp>(*m) == ClueInterp::MISTAKE) {
      mistake = true;
    }
  } else if (std::holds_alternative<DiscardAction>(action)) {
    auto m = hypo_game.last_move();
    if (m && std::holds_alternative<DiscardInterp>(*m) &&
        std::get<DiscardInterp>(*m) == DiscardInterp::MISTAKE) {
      mistake = true;
    }
  }
  if (mistake) return -100.0;

  double value = 0.0;
  if (std::holds_alternative<ClueAction>(action)) {
    const auto& ca = std::get<ClueAction>(action);
    auto playables_us = game.me().obvious_playables(game, state.our_player_index);
    // Low-clue-count gate: at clue_tokens < 3 AND pace >= 3, only
    // high-value clues clear; otherwise penalise so a known play wins.
    // Skip the gate when every known play is a dupe (CTP'd elsewhere) —
    // suppressing the clue would force a wasted dupe-play or a worse
    // discard, and the v0.21 policy is about preferring real plays over
    // low-value clues, not about forcing dupe-plays.
    if (state.clue_tokens < 3 && state.pace() >= 3) {
      bool has_real_play = false;
      for (int o : playables_us) {
        auto pid = game.me().thoughts[o].id(/*infer=*/true);
        if (pid && identity_called_to_play_elsewhere(game, *pid, o)) continue;
        has_real_play = true;
        break;
      }
      if (has_real_play && !is_high_value_clue(game, hypo_game, ca)) {
        return -1.0;
      }
    }
    double mult = playables_us.empty() ? 0.5 : (game.in_endgame() ? 0.1 : 0.25);
    double result = get_result(game, hypo_game, ca);
    value = result * (result > 0 ? mult : 1.0) - 0.5;
  } else if (std::holds_alternative<PlayAction>(action)) {
    const auto& pa = std::get<PlayAction>(action);
    std::optional<Identity> id;
    if (pa.suit_index != -1) id = Identity(pa.suit_index, pa.rank);
    bool unknown_dupe = false;
    if (id && !game.in_endgame()) {
      auto matches = visible_find(state, game.me(), *id, /*exclude_order=*/pa.order);
      for (int o : matches) {
        if (game.is_touched(o) && !hypo_game.common.order_trash(hypo_game, o)) {
          unknown_dupe = true;
          break;
        }
      }
    }
    if (unknown_dupe) value = -0.25;
    else if (!id) value = 1.5;
    else value = 0.02 * (5 - id->rank);
  } else if (std::holds_alternative<DiscardAction>(action)) {
    const auto& da = std::get<DiscardAction>(action);
    std::optional<Identity> id;
    if (da.suit_index != -1) id = Identity(da.suit_index, da.rank);
    bool is_trash = game.me().order_kt(game, da.order) ||
                     game.meta[da.order].status == CardStatus::CALLED_TO_DISCARD;
    auto chop = game.chop(state.holder_of(da.order));
    if (game.in_endgame()) value = -1.0;
    else if (is_trash) value = 0.0;
    else if (chop && *chop == da.order) value = -0.25;
    else if (!id) value = -1.5;
    else value = -0.5;

    // Orange-variant tiering. In a variant with an inverted (Orange /
    // Dark Orange) suit the on_discard rule turns a PerformDiscard of
    // an orange card into a play attempt — so discarding an orange-
    // playable card *is* a play, and discarding a card that could be
    // orange has upside that the baseline penalty doesn't capture.
    bool target_id_is_orange =
        id && state.variant->suits[id->suit_index].suit_type.inverted;
    bool target_possible_has_orange = false;
    for (Identity i : game.me().thoughts[da.order].possible) {
      if (state.variant->suits[i.suit_index].suit_type.inverted) {
        target_possible_has_orange = true;
        break;
      }
    }
    if (!is_trash) {
      if (target_id_is_orange && state.is_playable(*id)) {
        // Discard advances the orange stack — value at the play tier
        // regardless of endgame. The known-id PlayAction baseline is
        // `0.02 * (5 - rank)` (line 370) ≈ 0.02..0.08, small enough
        // that clue eval often beats it, so bump to 1.0. This must
        // override the in_endgame baseline (-1.0) too — discarding
        // a known-orange playable in endgame is still a stack-advance.
        value = 1.0;
      } else if (target_possible_has_orange && !target_id_is_orange) {
        // Possibly-orange unknown: in orange games the bot must be
        // willing to discard rather than fall back to clues that may
        // force a critical orange misplay. Floor materially above the
        // unknown-card baseline; the upside (might advance the orange
        // stack) justifies preferring this over a positively-scored
        // clue that the convention may have mis-evaluated.
        value = std::max(value, 0.5);
      }
      // Known-orange-unplayable falls through to the baseline
      // intentionally: PerformDiscard would strike, so the existing
      // penalty is the right ceiling.
    }

    if (!game.in_endgame()) {
      const Player& m = game.me();
      for (int o : m.obvious_playables(game, state.our_player_index)) {
        // If this discard IS the play (inverted-suit game-rule), don't
        // penalize — PerformDiscard{o} dispatches the play onto the stack.
        if (o == da.order) continue;
        // Known-orange-playable: the discard is also a play under the
        // inversion, so it isn't "blocking" the other playable — it's
        // a choice of which of two plays to take this turn.
        if (target_id_is_orange && state.is_playable(*id)) break;
        auto pid = m.thoughts[o].id(/*infer=*/true);
        if (!pid || !state.is_playable(*pid)) continue;
        if (identity_called_to_play_elsewhere(game, *pid, o)) continue;
        value -= 10.0;
        break;
      }
    }
  }

  if (value == -100.0) return -100.0;
  return value + advance(game, hypo_game, 1);
}

// --- eval_state ----------------------------------------------------------

double eval_state(const State& state, bool in_endgame) {
  int num_suits = static_cast<int>(state.variant->suits.size());
  double score_val =
      std::min(state.score(), 2 * num_suits) * 0.5 + state.score();
  double clue_val;
  if (in_endgame || state.clue_tokens == 0 || !state.can_clue()) {
    clue_val = 0.0;
  } else if (state.clue_tokens > 6) {
    clue_val = 3.0 + (state.clue_tokens - 6) * 0.25;
  } else {
    clue_val = state.clue_tokens / 2.0;
  }

  int score_loss = num_suits * 5 - state.max_score();
  double dc_crit_val = -20.0 * score_loss;

  double strikes_val;
  switch (state.strikes) {
    case 1: strikes_val = -1.5; break;
    case 2: strikes_val = -3.5; break;
    case 3: strikes_val = -100.0; break;
    default: strikes_val = 0.0; break;
  }
  return score_val + clue_val + dc_crit_val + strikes_val;
}

// --- eval_game -----------------------------------------------------------

double eval_game(const Game& orig, const Game& game) {
  hanabi::instr::ScopedTimer st("reactor.eval_game");
  const State& state = game.state;
  if (state.score() == orig.state.max_score()) return 100.0;

  bool in_endgame = orig.in_endgame() ||
                     orig.state.rem_score() <
                         static_cast<int>(state.variant->suits.size());
  double state_val = eval_state(state, in_endgame);

  double future_val = 0.0;
  for (const auto& hand : state.hands) {
    for (int order : hand) {
      CardStatus status = game.meta[order].status;
      if (status == CardStatus::CALLED_TO_PLAY) {
        auto id = game.me().thoughts[order].id(/*infer=*/true);
        if (!id) future_val += 0.4;
        else if (state.is_basic_trash(*id)) future_val -= 1.5;
        else if (id->rank == 5) future_val += 0.8;
        else future_val += 0.4;
      } else if (status == CardStatus::CALLED_TO_DISCARD) {
        auto by = game.meta[order].by;
        if (!by) continue;
        auto id = state.deck[order].id();
        if (!id) {
          if (*by != state.our_player_index) {
            // future_val += 0
          } else {
            future_val += 0.3;
          }
        } else if (state.is_basic_trash(*id)) {
          future_val += 0.3;
        } else if (game.me().is_sieved(game, *id, order)) {
          future_val += 0.2;
        } else if (state.is_critical(*id)) {
          future_val -= (5 - state.playable_away(*id)) * 10.0;
        } else if (*by != state.our_player_index) {
          // future_val += 0
        } else {
          future_val -= (5 - state.playable_away(*id)) * 0.5;
        }
      }
    }
  }

  double bdr_val = 0.0;
  for (Identity id : state.variant->all_ids()) {
    const auto& discarded = state.discard_stacks[id.suit_index][id.rank - 1];
    if (state.is_basic_trash(id) || id.rank == 5 || discarded.empty()) continue;
    std::optional<int> duplicate;
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if (state.deck[o].matches(id) ||
            (game.me().thoughts[o].matches(id, /*infer=*/true) && game.meta[o].focused)) {
          duplicate = o;
          break;
        }
      }
      if (duplicate) break;
    }
    bool duplicated = duplicate.has_value();
    if (!duplicated) {
      bool all_dups = !discarded.empty();
      for (int o : discarded) {
        if (game.meta[o].status != CardStatus::CALLED_TO_DISCARD) {
          all_dups = false;
          break;
        }
        if (!game.meta[o].by) {
          all_dups = false;
          break;
        }
        if (*game.meta[o].by == state.our_player_index) {
          all_dups = false;
          break;
        }
        bool any_in_us = false;
        for (int o2 : orig.state.our_hand()) {
          if (game.me().thoughts[o2].possible.contains(id)) {
            any_in_us = true;
            break;
          }
        }
        if (!any_in_us) {
          all_dups = false;
          break;
        }
      }
      if (all_dups) duplicated = true;
    }
    if (duplicated) continue;
    if (id.rank == 1) bdr_val -= static_cast<double>(discarded.size()) * discarded.size();
    else if (id.rank == 2) bdr_val -= 3.0;
    else if (id.rank == 3) bdr_val -= 1.5;
    else bdr_val -= 0.5;
  }
  bdr_val *= 2.5;

  int locked_count = 0;
  for (int i = 0; i < state.num_players; ++i) {
    if (game.common.thinks_locked(game, i)) ++locked_count;
  }
  double lock_penalty;
  switch (locked_count) {
    case 0: lock_penalty = 0.0; break;
    case 1: lock_penalty = -1.0; break;
    case 2: lock_penalty = -3.0; break;
    default: lock_penalty = -10.0; break;
  }

  double endgame_penalty = 0.0;
  if (orig.state.endgame_turns) {
    int turns = *orig.state.endgame_turns;
    std::vector<int> stacks = orig.state.play_stacks;
    for (int i = 0; i < turns; ++i) {
      int player_index = (orig.state.current_player_index + i + 1) % state.num_players;
      for (int o : orig.state.hands[player_index]) {
        auto pid = orig.state.deck[o].id();
        if (!pid) continue;
        if (orig.state.is_playable(*pid)) {
          stacks[pid->suit_index] = pid->rank;
          break;
        }
      }
    }
    int stacks_sum = 0;
    for (int v : stacks) stacks_sum += v;
    endgame_penalty = (stacks_sum - state.max_score()) * 5.0;
  }

  return state_val + future_val + bdr_val + lock_penalty + endgame_penalty;
}

}  // namespace hanabi::reactor
