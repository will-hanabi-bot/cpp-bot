// Decision engine + reactor convention hooks for Game.
//
// Split out of game.cpp so that game.cpp holds only state/belief mechanics
// (action handlers, empathy elim, rewind, simulation) with no dependency on
// conventions/ or endgame/. Everything here is the half of Game that decides
// or interprets: the interpret_* hooks (delegating to src/conventions/reactor/),
// chop/has_ptd/find_all_clues/find_all_discards, and take_action() -- which
// invokes the endgame solver when in range.
#include "hanabi/basics/game.h"

#include <algorithm>
#include <stdexcept>

#include "hanabi/basics/clue_result.h"
#include "hanabi/basics/fix.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/sarcastic.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/forced_endgame.h"
#include "hanabi/endgame/solver.h"

namespace hanabi {

// --- Convention hooks: reactor implementation ----------------------------

void Game::interpret_clue(const Game& prev, const ClueAction& action) {
  using namespace hanabi::reactor;
  check_missed(action.giver, /*sentinel=*/99);

  // Detect a "deferred reactive": the giver of this clue was the
  // reacter the previous reactive expected to act. Deferring (= clue
  // instead of reacting) cancels the prior reactive's expected
  // reaction AND carries the reactive intent forward — the new clue
  // is itself reactive, with the natural next-player-after-giver
  // reacter. Guard on `!inverted` because response-inversion waitings
  // (set by some stable paths) just end on deferral, no chain
  // continuation. Capture the flag BEFORE clearing `waiting`.
  bool was_deferring = !waiting.empty() &&
                       waiting.front().reacter == action.giver &&
                       !waiting.front().inverted;
  if (!waiting.empty() && waiting.front().reacter == action.giver) {
    waiting.clear();
  }

  std::optional<ClueInterp> interp;

  if (next_interp) {
    if (std::holds_alternative<ClueInterp>(*next_interp)) {
      ClueInterp forced = std::get<ClueInterp>(*next_interp);
      if (forced == ClueInterp::REACTIVE) {
        int reacter = state.next_player_index(action.giver);
        interp = interpret_reactive(prev, *this, action, reacter,
                                       /*looks_stable=*/true);
      } else {
        interp = interpret_stable(prev, *this, action, /*stall=*/false);
      }
    }
  } else if (state.options.empty_clues && action.list_.empty()) {
    interp = ClueInterp::USELESS;
  } else if (was_deferring) {
    // Skip the standard stable/reactive heuristic — the deferral
    // context forces the reactive interpretation.
    int reacter = state.next_player_index(action.giver);
    interp = interpret_reactive(prev, *this, action, reacter,
                                   /*looks_stable=*/false);
  } else if (prev.common.obvious_locked(prev, action.giver) || in_endgame() ||
              prev.state.clue_tokens == 8) {
    int bob = state.next_player_index(action.giver);
    int cathy = state.next_player_index(bob);
    bool target_is_cathy = action.target == cathy && cathy != action.giver;
    bool bob_unloaded = target_is_cathy &&
                          prev.common.obvious_playables(prev, bob).empty();
    if (bob_unloaded) {
      interp = interpret_reactive(prev, *this, action, /*reacter=*/bob,
                                     /*looks_stable=*/false);
    } else {
      interp = interpret_stable(prev, *this, action, /*stall=*/true);
    }
  } else {
    int bob_idx = state.next_player_index(action.giver);
    // Vacuous-truth guard. When a player's `old_play` is empty the
    // for-loop below is vacuous and !any_kept is trivially true. That's
    // harmless when the resulting `reacter != action.target` (routes to
    // reactive — fine for a target!=bob clue). But when the vacuous
    // match yields `reacter == action.target` AND target != bob, the
    // dispatcher falsely routes a reactive-shape clue to stable, and
    // `try_stable`'s `ref_discard` stamps a spurious CTD on the
    // receiver. Replay 1899623 T16 was this case. Block the vacuous
    // pick only there; (reacter != target) and (target == bob) vacuous
    // matches still work because they route correctly.
    std::optional<int> reacter;
    for (int i = 1; i < state.num_players; ++i) {
      int pi = (action.giver + i) % state.num_players;
      auto old_play = prev.common.obvious_playables(prev, pi);
      auto new_play = common.obvious_playables(*this, pi);
      bool any_kept = false;
      for (int o : old_play) {
        if (std::find(new_play.begin(), new_play.end(), o) != new_play.end()) {
          any_kept = true;
          break;
        }
      }
      if (!any_kept) {
        bool vacuous_self_target_offsuit = old_play.empty() &&
                                               pi == action.target &&
                                               pi != bob_idx;
        if (vacuous_self_target_offsuit) continue;
        reacter = pi;
        break;
      }
    }

    FixResult fr = check_fix(prev, *this, action);
    std::vector<int> fixed;
    if (std::holds_alternative<FixResultNormal>(fr)) {
      const auto& f = std::get<FixResultNormal>(fr);
      fixed.insert(fixed.end(), f.clued_resets.begin(), f.clued_resets.end());
      fixed.insert(fixed.end(), f.duplicate_reveals.begin(),
                    f.duplicate_reveals.end());
    }
    bool allowable_fix = action.target == bob_idx && !fixed.empty();

    if (!reacter) {
      // No reacter found: every candidate either kept its plays (loaded)
      // or was the vacuous-truth-suppressed reacter==target pick.
      if (allowable_fix) {
        interp = ClueInterp::FIX;
      } else if (action.target != bob_idx) {
        // Clue to Cathy with everyone between the giver and the target
        // loaded: the convention reads STABLE unless the stable
        // interpretation is actually bad — interpret_stable's bad_stable
        // check handles that fallback (restoring the game and rerouting
        // to reactive with bob as the canonical reacter). Replay 1915981
        // T11: bob is loaded (CTP'd b1) and the blue clue to cathy is a
        // good ref_play (pushes a playable t2) → stable. Replay 1899623
        // T16: the stable rank-4 read CTD's a giver-visibly useful n3 →
        // bad_stable → reactive. Cathy herself can't verify stable-
        // goodness (own hand hidden; bad_stable defers) — she reads
        // stable provisionally, and bob's unexpected reaction triggers
        // the response-inversion rewind (replay 1882268 T8/T9).
        interp = interpret_stable(prev, *this, action, /*stall=*/false);
      } else {
        // Clue to a loaded bob with no reacter: keep the reactive(bob)
        // routing — reacter==receiver is degenerate and scores as a
        // MISTAKE, which is what lets the giver's eval reject clue
        // shapes the partners won't read cleanly (replay 1892197 T9).
        interp = interpret_reactive(prev, *this, action, bob_idx,
                                       /*looks_stable=*/false);
      }
    } else if (*reacter == action.target) {
      interp = interpret_stable(prev, *this, action, /*stall=*/false);
    } else {
      auto prev_playables =
          prev.players[action.target].obvious_playables(prev, action.target);
      bool any_in_prev = false;
      for (int o : fixed) {
        if (std::find(prev_playables.begin(), prev_playables.end(), o) !=
            prev_playables.end()) {
          any_in_prev = true;
          break;
        }
      }
      if (allowable_fix && any_in_prev) {
        interp = ClueInterp::FIX;
      } else {
        interp = interpret_reactive(prev, *this, action, *reacter,
                                       /*looks_stable=*/false);
      }
    }
  }

  if (!interp) interp = ClueInterp::MISTAKE;
  with_move(*interp);

  // Count newly-signalled plays before elim.
  std::vector<int> signalled_plays;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (prev.meta[o].status != CardStatus::CALLED_TO_PLAY &&
          meta[o].status == CardStatus::CALLED_TO_PLAY) {
        signalled_plays.push_back(o);
      }
    }
  }
  elim();
  std::vector<int> plays_after;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (meta[o].status == CardStatus::CALLED_TO_PLAY) plays_after.push_back(o);
    }
  }
  if (plays_after.size() < signalled_plays.size()) {
    with_move(ClueInterp::MISTAKE, /*overwrite=*/true);
  }
  if (prev.state.can_clue()) reset_zcs();
  if (!state.can_clue()) zcs_turn = state.turn_count;
  next_interp = std::nullopt;
}

void Game::interpret_discard(const Game& prev, const DiscardAction& action) {
  using namespace hanabi::reactor;
  check_missed(action.player_index_v, action.order);

  bool failed = action.failed;
  std::optional<Identity> id;
  if (action.suit_index != -1 && action.rank != -1) {
    id = Identity(action.suit_index, action.rank);
  }

  if (failed) {
    // Bombed - clear conv info, except for cards explicitly CALLED_TO_PLAY.
    // A strike (often a dupe-strike on an already-played card or a finesse
    // miscommunication) breaks the convention chain that produced the
    // misplayed card, but CTPs stamped by *separate* clues remain valid.
    // The previous nuclear reset wiped them, costing the team the queued
    // play; the v0.26 fix preserved CTP through `elim()`'s narrow-to-empty
    // path but that runs *after* this handler. Mirroring the same intent
    // here keeps the CTP committed across the strike. CTP cards retain
    // their `inferred`/`info_lock` narrowing too (the convention's promise
    // about the identity still holds).
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if (meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
        with_thought(o, [](const Thought& t) {
          Thought out = t;
          out.inferred = t.possible;
          out.old_inferred = std::nullopt;
          out.info_lock = std::nullopt;
          return out;
        });
        with_meta(o, [](ConvData& m) { m = m.cleared(); });
      }
    }
    waiting.clear();
  }

  bool useful_dc = !failed && prev.state.deck[action.order].clued && id.has_value() &&
                    state.is_useful(*id) &&
                    prev.meta[action.order].status != CardStatus::CALLED_TO_DISCARD &&
                    !(prev.common.thinks_locked(prev, action.player_index_v) &&
                      prev.state.clue_tokens == 0);

  if (!waiting.empty()) {
    bool rewound =
        react_discard(prev, *this, action.player_index_v, action.order, waiting.front());
    if (rewound) {
      // The rewind already replayed the action end-to-end (including
      // with_move + elim + reset_zcs); doing it again here would
      // double-record into move_history.
      return;
    }
  } else if (useful_dc && id) {
    DiscardResult dc_result = interpret_useful_dc(*this, action);
    if (std::holds_alternative<DiscardResultNone>(dc_result)) {
      with_move(DiscardInterp::NONE);
    } else if (std::holds_alternative<DiscardResultMistake>(dc_result)) {
      with_move(DiscardInterp::MISTAKE);
    } else if (std::holds_alternative<DiscardResultGentlemansDiscard>(dc_result)) {
      const auto& gd = std::get<DiscardResultGentlemansDiscard>(dc_result);
      State hypo_state = state;
      for (size_t k = 0; k < gd.orders.size(); ++k) {
        int o = gd.orders[k];
        bool hidden = k + 1 != gd.orders.size();
        IdentitySet inferred = hidden ? hypo_state.playable_set : IdentitySet::single(*id);
        auto me_id = me().thoughts[o].id();
        if (me_id) hypo_state = hypo_state.with_play(*me_id);
        IdentitySet inf_copy = inferred;
        with_thought(o, [inf_copy](const Thought& t) {
          Thought out = t;
          out.inferred = inf_copy;
          return out;
        });
        with_meta(o, [hidden](ConvData& m) {
          m.status = CardStatus::GENTLEMANS_DISCARD;
          m.hidden = hidden;
        });
      }
      with_move(DiscardInterp::GENTLEMANS_DISCARD);
    } else if (std::holds_alternative<DiscardResultSarcastic>(dc_result)) {
      const auto& s = std::get<DiscardResultSarcastic>(dc_result);
      SarcasticLink new_link{s.orders, *id};
      common.links.insert(common.links.begin(), Link{std::move(new_link)});
      with_move(DiscardInterp::SARCASTIC);
    }
  } else {
    with_move(DiscardInterp::NONE);
  }

  elim();
  if (prev.state.can_clue()) reset_zcs();
}

void Game::interpret_play(const Game& prev, const PlayAction& action) {
  using namespace hanabi::reactor;
  // reinterp_play (gentleman's-discard reinterp) isn't ported; skip it for now.
  check_missed(action.player_index_v, action.order);
  if (!waiting.empty()) {
    bool rewound =
        react_play(prev, *this, action.player_index_v, action.order, waiting.front());
    if (rewound) {
      // The rewind already replayed the action end-to-end (including
      // with_move + elim); skip the post-react bookkeeping.
      return;
    }
  }
  with_move(PlayInterp::NONE, /*overwrite=*/true);
  elim();
  if (prev.state.can_clue()) reset_zcs();
}

void Game::update_turn(const TurnAction& action) {
  int cpi = action.current_player_index;
  if (cpi == -1) return;

  std::optional<int> next_queued_playable;
  int best_signal = 99;
  for (int o : state.hands[cpi]) {
    if (meta[o].status == CardStatus::CALLED_TO_PLAY &&
        !common.thoughts[o].id(/*infer=*/true)) {
      int s = meta[o].signal_turn.value_or(99);
      if (!next_queued_playable || s < best_signal) {
        next_queued_playable = o;
        best_signal = s;
      }
    }
  }

  if (!waiting.empty() &&
      waiting.front().reacter == state.last_player_index(cpi)) {
    waiting.clear();
  }

  if (next_queued_playable) {
    int order = *next_queued_playable;
    IdentitySet new_inferred =
        common.thoughts[order].inferred.intersect(state.playable_set);
    // Narrow inferred to currently-playable identities when possible, but do
    // NOT clear CTP when the intersection is empty. A CTP'd card may be
    // waiting on a delayed-play chain (e.g. brown 4 called to play after
    // brown 3 plays); clearing here would lose the convention signal before
    // the chain resolves. CTP is cleared only by on_play (card leaves hand)
    // or by elim's post-card_elim sweep (card proven trash).
    if (!new_inferred.is_empty()) {
      IdentitySet ni = new_inferred;
      with_thought(order, [ni](const Thought& t) {
        Thought out = t;
        out.inferred = ni;
        return out;
      });
    }
  }
  elim();
}

void Game::refresh_after_play(const Game&, const PlayAction&) {}
void Game::clean_hypo() {}

std::vector<int> Game::filter_playables(const Player&, int,
                                          const std::vector<int>& orders, bool) const {
  return orders;
}

bool Game::valid_arr(Identity id, int order) const {
  // Reactor: respect info_lock when assigning identities to unknown cards.
  // Port of conventions/reactor/reactor.py: Reactor.valid_arr.
  const auto& lock = me().thoughts[order].info_lock;
  return !lock || lock->contains(id);
}

double Game::eval_action(const Action&) const { return 0.0; }

// --- Reactor helpers (faithful port of reactor.scala chop/hasPtd) --------

// Reactor's `in_endgame` is one turn earlier than the Game default. Since we
// don't subclass, we check whether the game has reactor data and adjust.
bool Game::in_endgame() const {
  // Reactor signals presence via waiting/zcs_turn fields existing on Game; the
  // conditional below matches Reactor's override of `in_endgame`.
  return state.pace() < state.num_players - 1;
}

std::optional<int> Game::chop(int player_index) const {
  // First pass: explicit CalledToDiscard.
  for (int o : state.hands[player_index]) {
    if (meta[o].status == CardStatus::CALLED_TO_DISCARD) return o;
  }
  // Second pass: newest unclued + status NONE, gated on zcs_turn.
  for (int o : state.hands[player_index]) {
    bool zcs_ok = zcs_turn == -1 || zcs_turn >= state.deck[o].turn_drawn;
    if (zcs_ok && !state.deck[o].clued && meta[o].status == CardStatus::NONE) {
      return o;
    }
  }
  return std::nullopt;
}

bool Game::has_ptd() const {
  int player_index = state.current_player_index;
  int zelda = state.last_player_index(player_index);
  int bob = state.next_player_index(player_index);
  auto bob_chop = chop(bob);

  std::optional<Identity> bob_chop_id;
  if (bob_chop) bob_chop_id = state.deck[*bob_chop].id();

  auto known_dupe = [&]() -> bool {
    if (!bob_chop_id) return false;
    for (int o : state.hands[bob]) {
      if (o == *bob_chop) continue;
      if (players[zelda].thoughts[o].matches(*bob_chop_id) &&
          me().thoughts[o].matches(*bob_chop_id)) {
        return true;
      }
    }
    return false;
  };

  auto unknown_play = [&]() -> bool {
    if (!last_actions[zelda]) return false;
    const Action& last = *last_actions[zelda];
    if (!std::holds_alternative<PlayAction>(last)) return false;
    if (!bob_chop_id) return false;
    const auto& pa = std::get<PlayAction>(last);
    Identity played_id{pa.suit_index, pa.rank};
    if (played_id != *bob_chop_id) return false;
    const auto& old_inf = common.thoughts[pa.order].old_inferred;
    return !old_inf || *old_inf != IdentitySet::single(played_id);
  };

  if (common.obvious_loaded(*this, bob)) return true;
  if (bob_chop_id && state.is_critical(*bob_chop_id)) return false;
  if (bob_chop_id && state.is_basic_trash(*bob_chop_id)) return !unknown_play();
  if (known_dupe()) return true;
  return !(bob_chop_id &&
            (state.is_playable(*bob_chop_id) || bob_chop_id->rank == 2));
}

// find_all_clues: enumerate clue candidates, filtering out mistakes and
// useless duplicates. Mirrors reactor.py find_all_clues — we evaluate each
// candidate by simulating it, drop MISTAKE interpretations, and rank
// surviving useful clues by get_result score. At most one representative
// "useless" clue is included so the solver still has a stall option.
std::vector<PerformAction> Game::find_all_clues(int giver) const {
  std::vector<PerformAction> out;
  bool added_useless_clue = false;
  std::vector<std::pair<Clue, double>> scored;

  for (int target = 0; target < state.num_players; ++target) {
    if (target == giver) continue;
    for (const Clue& clue : state.all_valid_clues(target)) {
      auto list_orders =
          state.clue_touched(state.hands[target], clue.kind, clue.value);

      // Only touches previously-clued trash — useless.
      bool all_trash_touch = !list_orders.empty();
      for (int o : list_orders) {
        if (!state.deck[o].clued) { all_trash_touch = false; break; }
        auto id = state.deck[o].id();
        if (!id || !state.is_basic_trash(*id)) { all_trash_touch = false; break; }
      }
      if (all_trash_touch) {
        if (added_useless_clue) continue;
        added_useless_clue = true;
        scored.emplace_back(clue, 0.0);
        continue;
      }

      ClueAction action{giver, clue.target, list_orders, clue.base()};
      Game hypo = simulate_clue(action);
      auto last = hypo.last_move();
      if (last && std::holds_alternative<ClueInterp>(*last) &&
          std::get<ClueInterp>(*last) == ClueInterp::MISTAKE) {
        continue;
      }

      // Critical-discard guard. The reactor's reactive interp can stamp a
      // reacter slot CALLED_TO_DISCARD (via `target_discard` from the
      // colour-reactive play_target path, the rank-reactive inverted-target
      // path, etc.) whenever `calc_slot(focus_slot, target_slot, hand_size)`
      // lands on a slot whose `inferred` filters to a non-empty non-critical
      // set from the holder's POV. `target_discard` doesn't verify that the
      // *actual* identity of that card (visible to the giver) is non-
      // critical — so a clue can promise the reacter "this is safe to
      // discard" when the giver can see plainly that it isn't. The reacter's
      // solver then follows the urgent CTD signal and discards a critical
      // card.
      //
      // Replay 1892428 T47 (the surfacing case): will-bot67 cluing green to
      // will-bot69 picked wb69's slot 1 (i3 playable) as the receiver play
      // target. The newest-demoted focus rule chose g2 (slot 3) for
      // focus_slot=3, so calc_slot(3, 1, 5) = 2 landed the reacter CTD on
      // yagami's slot 2 = i4 — the only remaining i4 in a Dark Prism game.
      // yagami's solver followed the CTD and discarded i4, capping the
      // max_score. The mirror of the 1892259 reacter-strike fix on the
      // discard side: filter at find_all_clues time so the giver doesn't
      // issue the clue. Done only here (not inside `target_discard`) so
      // every observer interpreting an already-given clue runs the same
      // POV-invariant convention pipeline and stays consistent — exactly the
      // approach the v0.25 fix (interpret_clue.cpp:265-278 commentary) took
      // for the play-side.
      bool reacter_critical_discard = false;
      for (const auto& hand : state.hands) {
        for (int o : hand) {
          if (hypo.meta[o].status != CardStatus::CALLED_TO_DISCARD) continue;
          if (meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
          auto actual = state.deck[o].id();
          if (!actual) continue;
          if (state.is_critical(*actual)) {
            reacter_critical_discard = true;
            break;
          }
        }
        if (reacter_critical_discard) break;
      }
      if (reacter_critical_discard) continue;

      double clue_result = hanabi::reactor::get_result(*this, hypo, action);
      bool reactive_move =
          last && std::holds_alternative<ClueInterp>(*last) &&
          std::get<ClueInterp>(*last) == ClueInterp::REACTIVE;
      bool hypo_score_up = hypo.common.hypo_score() > common.hypo_score();
      bool shrunk_possible = false;
      for (int o : state.hands[clue.target]) {
        auto did = state.deck[o].id();
        bool useful_card = !did || state.is_useful(*did);
        bool clued_in_hypo = hypo.state.deck[o].clued;
        bool shrunk = hypo.common.thoughts[o].possible.length() <
                      common.thoughts[o].possible.length();
        if (useful_card && clued_in_hypo && shrunk) {
          shrunk_possible = true;
          break;
        }
      }
      bool useful = clue_result > -1.0 &&
                    (reactive_move || hypo_score_up || shrunk_possible);

      if (useful) {
        scored.emplace_back(clue, clue_result);
      } else if (!added_useless_clue) {
        added_useless_clue = true;
        scored.emplace_back(clue, 0.0);
      }
      // else: skip; redundant useless clue
    }
  }

  std::stable_sort(scored.begin(), scored.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
  for (const auto& [clue, _] : scored) {
    if (clue.kind == ClueKind::COLOUR) {
      out.push_back(PerformColour{clue.target, clue.value});
    } else {
      out.push_back(PerformRank{clue.target, clue.value});
    }
  }
  return out;
}

// --- take_action (the main decision function) ---------------------------

namespace {

double clue_eval_value(const Game& game, const Clue& clue) {
  const State& state = game.state;
  ClueAction act{state.our_player_index, clue.target,
                  state.clue_touched(state.hands[clue.target], clue.kind, clue.value),
                  clue.base()};
  return hanabi::reactor::eval_action(game, Action{act});
}

bool contains_v(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

PerformAction Game::take_action() const {
  using namespace hanabi::reactor;
  const State& s = state;
  const Player& m = me();
  int next_player_index = s.next_player_index(s.our_player_index);

  // --- Handle urgent (signalled to play / signalled to discard) cards. ---
  std::optional<int> urgent_order;
  for (int o : s.our_hand()) {
    if (meta[o].urgent) {
      urgent_order = o;
      break;
    }
  }
  std::optional<PerformAction> urgent_action;
  if (urgent_order) {
    bool urgent_bob_save = s.can_clue() && !waiting.empty() &&
                            waiting.front().reacter == s.our_player_index &&
                            waiting.front().receiver != next_player_index &&
                            !common.obvious_loaded(*this, next_player_index);
    if (urgent_bob_save) {
      Game tmp = *this;
      tmp.zcs_turn = -1;
      auto bob_chop = tmp.chop(next_player_index);
      if (bob_chop) {
        auto bob_chop_id = s.deck[*bob_chop].id();
        if (bob_chop_id && s.is_critical(*bob_chop_id)) {
          auto clues = s.can_clue() ? s.all_valid_clues(next_player_index)
                                     : std::vector<Clue>{};
          if (!clues.empty()) {
            auto best = *std::max_element(
                clues.begin(), clues.end(),
                [&](const Clue& a, const Clue& b) {
                  return clue_eval_value(*this, a) < clue_eval_value(*this, b);
                });
            urgent_action = best.kind == ClueKind::COLOUR
                                ? PerformAction{PerformColour{best.target, best.value}}
                                : PerformAction{PerformRank{best.target, best.value}};
          }
        }
      }
    }
    if (!urgent_action) {
      CardStatus status = meta[*urgent_order].status;
      const Thought& thought = m.thoughts[*urgent_order];
      // CTP/CTD are PHYSICAL action labels: CTP → PerformPlay, CTD →
      // PerformDiscard. For an inverted (Orange / Dark Orange) suit the
      // game-rule inversion in on_play / on_discard converts the physical
      // action into the right semantic outcome (PerformDiscard on a
      // playable orange advances the orange stack; PerformPlay on any
      // orange sends it to the discard pile). The convention is
      // responsible for marking CTP/CTD on the card that *matches* the
      // physical action it wants — e.g. try_stable's playable_rank branch
      // marks CTD on an orange focus so PerformDiscard advances the
      // stack.
      if (status == CardStatus::CALLED_TO_PLAY &&
          !thought.possible.forall([&](Identity i) { return s.is_basic_trash(i); })) {
        urgent_action = PerformPlay{*urgent_order};
      } else if (status == CardStatus::CALLED_TO_DISCARD &&
                  !thought.possible.forall([&](Identity i) { return s.is_critical(i); })) {
        urgent_action = PerformDiscard{*urgent_order};
      }
    }
  }

  // --- Endgame solver fork ---
  if (s.rem_score() <= static_cast<int>(s.variant->suits.size()) + 1) {
    // Forced-endgame layer: mechanical rules that override the search
    // when the correct action is hardcoded. See
    // `src/endgame/forced_endgame.cpp` for the rule list. Cheap enough
    // (O(n × hand × suits)) to check before the solver kicks in.
    if (auto forced = hanabi::endgame::forced_endgame_action(*this); forced) {
      return *forced;
    }
    // 6 s is enough budget for the solver to find a near-optimal action in
    // the positions we've seen on hanab.live (the deeper search rarely
    // changes the picked action) while keeping per-turn compute well under
    // the server's per-turn clock allowance. The solver gracefully returns
    // its best-found result on timeout.
    hanabi::endgame::EndgameSolver solver(/*mc=*/true, /*timeout=*/6.0);
    auto result = solver.solve(*this);
    if (result.ok() && result.winrate >= hanabi::endgame::Fraction(1, 100)) {
      return result.action;
    }
    // Solver returned no winning action or winrate < 1%; fall through to heuristic.
  }

  if (urgent_action) return *urgent_action;

  // --- Find playable orders ---
  auto common_p = common.obvious_playables(*this, s.our_player_index);
  auto known_p = m.obvious_playables(*this, s.our_player_index);

  std::vector<int> possible_connectors;
  if (!common_p.empty() && !waiting.empty() &&
      waiting.front().receiver == s.our_player_index) {
    int reacter = waiting.front().reacter;
    for (int p : common_p) {
      for (Identity i : m.thoughts[p].inferred) {
        auto nxt = i.next();
        if (!nxt) continue;
        bool match = false;
        for (int o : s.hands[reacter]) {
          if (m.thoughts[o].matches(*nxt)) {
            match = true;
            break;
          }
        }
        if (match) {
          possible_connectors.push_back(p);
          break;
        }
      }
    }
  }

  std::vector<int> playable_orders;
  if (!possible_connectors.empty()) {
    int target = possible_connectors.front();
    int best_signal = meta[target].signal_turn.value_or(99);
    for (int o : possible_connectors) {
      int s_v = meta[o].signal_turn.value_or(99);
      if (s_v < best_signal) {
        best_signal = s_v;
        target = o;
      }
    }
    playable_orders = {target};
  } else if (!known_p.empty()) {
    for (int order : known_p) {
      if (meta[order].status == CardStatus::CALLED_TO_PLAY) {
        playable_orders.push_back(order);
        continue;
      }
      bool same_focused_dup = false;
      for (int o : s.our_hand()) {
        if (o != order && m.thoughts[o].possible == m.thoughts[order].possible &&
            meta[o].focused) {
          same_focused_dup = true;
          break;
        }
      }
      if (!same_focused_dup) playable_orders.push_back(order);
    }
    // Queue-order honoring: when several plays sit in the queue,
    // prefer the ones whose inferred identity is a *definite* singleton
    // playable on the current play stacks. Otherwise the bot can pick an
    // ambiguous CTP card ahead of an empathy-locked prerequisite (e.g. a
    // reactive-pushed slot inferred as {b2, n3} ranked ahead of the
    // already-empathy-known b2 sitting in a later slot — playing the
    // ambiguous one risks misplaying b3 onto a blue stack still at 1).
    if (playable_orders.size() > 1) {
      std::vector<int> definite;
      for (int o : playable_orders) {
        auto id = m.thoughts[o].id(/*infer=*/true);
        if (id && s.is_playable(*id)) definite.push_back(o);
      }
      if (!definite.empty() && definite.size() < playable_orders.size()) {
        playable_orders = std::move(definite);
      }
    }
    // Queue-order honoring (second pass): when multiple plays remain
    // tied after the definite-singleton filter — e.g. two CTP'd cards
    // both with definite singletons that are independently playable on
    // the current stacks — pick the earliest queued one by signal_turn.
    // This mirrors the precedent in the possible_connectors branch a
    // few lines up. Cards without signal_turn (e.g. an empathy-playable
    // that became playable because a prerequisite just played) are
    // treated as "always known" (signal_turn = 0), running ahead of any
    // later convention-marked play.
    if (playable_orders.size() > 1) {
      auto it = std::min_element(
          playable_orders.begin(), playable_orders.end(),
          [&](int a, int b) {
            return meta[a].signal_turn.value_or(0) <
                   meta[b].signal_turn.value_or(0);
          });
      playable_orders = {*it};
    }
  } else {
    playable_orders = m.thinks_playables(*this, s.our_player_index);
  }

  bool can_clue_now = s.can_clue() &&
                       (waiting.empty() || waiting.front().receiver != s.our_player_index);

  std::vector<std::pair<PerformAction, Action>> all_clues;
  if (can_clue_now) {
    for (int target = 0; target < s.num_players; ++target) {
      if (target == s.our_player_index) continue;
      for (const Clue& clue : s.all_valid_clues(target)) {
        PerformAction perform = clue.kind == ClueKind::COLOUR
                                     ? PerformAction{PerformColour{clue.target, clue.value}}
                                     : PerformAction{PerformRank{clue.target, clue.value}};
        ClueAction act{s.our_player_index, clue.target,
                        s.clue_touched(s.hands[target], clue.kind, clue.value),
                        clue.base()};
        all_clues.emplace_back(perform, Action{act});
      }
    }
  }

  std::vector<std::pair<PerformAction, Action>> all_plays;
  for (int o : playable_orders) {
    auto inferred = m.thoughts[o].id(/*infer=*/true);
    // Inverted (Orange) suit: an empathy-playable orange card advances the
    // orange stack via PerformDiscard, not PerformPlay (the orange play
    // rule sends physical PLAYs to the discard pile). When the inferred
    // id pins the card to an inverted suit, route through DiscardAction.
    if (inferred &&
        s.variant->suits[inferred->suit_index].suit_type.inverted) {
      Action act = Action{DiscardAction{s.our_player_index, o,
                                            inferred->suit_index, inferred->rank,
                                            /*failed=*/false}};
      all_plays.emplace_back(PerformDiscard{o}, std::move(act));
    } else {
      Action act = inferred
                        ? Action{PlayAction{s.our_player_index, o, inferred->suit_index, inferred->rank}}
                        : Action{PlayAction{s.our_player_index, o, -1, -1}};
      all_plays.emplace_back(PerformPlay{o}, std::move(act));
    }
  }

  // Forced-play detection.
  bool potential_forced_play = false;
  if (!all_plays.empty() && !waiting.empty() &&
      waiting.front().reacter == next_player_index) {
    for (int o : playable_orders) {
      for (Identity id : m.thoughts[o].inferred) {
        auto nxt = id.next();
        if (!nxt) continue;
        for (int o2 : s.hands[next_player_index]) {
          auto deck_id = s.deck[o2].id();
          if (deck_id && *deck_id == *nxt) {
            potential_forced_play = true;
            break;
          }
        }
        if (potential_forced_play) break;
      }
      if (potential_forced_play) break;
    }
  }

  bool cant_discard = s.clue_tokens == 8 ||
                       (s.pace() == 0 && (!all_clues.empty() || !all_plays.empty())) ||
                       potential_forced_play;

  std::vector<std::pair<PerformAction, Action>> all_discards;
  if (!cant_discard) {
    auto trash = m.thinks_trash(*this, s.our_player_index);
    // Orange-safety filter. Auto-discarding an empathy-trash card whose
    // `possible` still includes an inverted-suit (Orange / Dark Orange)
    // identity risks the engine's `on_discard(inverted, failed=false)`
    // play-attempt — which strikes when the actual id is an orange
    // basic_trash (e.g. o1 on orange stack 1). Drop those candidates so
    // the bot falls back to the chop instead. Keep:
    //   * CTD'd cards (the convention explicitly told us to discard),
    //   * cards with a singleton inferred (the dispatch swap a few lines
    //     down routes orange singletons through PerformPlay correctly),
    //   * cards whose possible has no inverted-suit id (cannot be orange).
    auto trash_is_orange_safe = [&](int o) -> bool {
      if (meta[o].status == CardStatus::CALLED_TO_DISCARD) return true;
      if (m.thoughts[o].id(/*infer=*/true)) return true;
      for (Identity i : m.thoughts[o].possible) {
        if (s.variant->suits[i.suit_index].suit_type.inverted) return false;
      }
      return true;
    };
    std::vector<int> safe_trash;
    for (int o : trash) {
      if (trash_is_orange_safe(o)) safe_trash.push_back(o);
    }
    std::vector<int> expected;
    if (!safe_trash.empty()) {
      expected = safe_trash;
    } else if (!m.obvious_locked(*this, s.our_player_index) && all_plays.empty() &&
                has_ptd()) {
      auto chop_o = chop(s.our_player_index);
      if (chop_o) expected = {*chop_o};
    }
    std::vector<int> discard_orders;
    if (!waiting.empty() && waiting.front().receiver == s.our_player_index) {
      discard_orders = expected;
    } else {
      auto discardable = m.discardable(*this, s.our_player_index);
      std::vector<int> seen;
      for (int o : expected) {
        if (!contains_v(seen, o)) {
          discard_orders.push_back(o);
          seen.push_back(o);
        }
      }
      for (int o : discardable) {
        if (contains_v(seen, o)) continue;
        // Re-apply the orange-safety filter on empathy-trash entries
        // that `discardable` may have added — `safe_trash` only
        // controlled `expected`.
        if (m.order_trash(*this, o) && !trash_is_orange_safe(o)) continue;
        discard_orders.push_back(o);
        seen.push_back(o);
      }
    }
    // v0.30: most-recent-CTD enforcement. When the hand has multiple
    // CTD'd cards (older CTDs that the bot hasn't yet discarded plus a
    // newer CTD from this turn's clue), the convention says the bot
    // always discards the most-recently-signaled CTD. Drop older CTDs
    // from the candidate pool so eval can't pick them this turn — they
    // remain CTD'd for future turns once the newer CTD is consumed.
    {
      int newest_signal = -1;
      for (int o : discard_orders) {
        if (meta[o].status == CardStatus::CALLED_TO_DISCARD) {
          int sig = meta[o].signal_turn.value_or(-1);
          if (sig > newest_signal) newest_signal = sig;
        }
      }
      if (newest_signal >= 0) {
        discard_orders.erase(
            std::remove_if(
                discard_orders.begin(), discard_orders.end(),
                [&](int o) {
                  return meta[o].status == CardStatus::CALLED_TO_DISCARD &&
                         meta[o].signal_turn.value_or(-1) != newest_signal;
                }),
            discard_orders.end());
      }
    }
    for (int o : discard_orders) {
      auto inferred = m.thoughts[o].id(/*infer=*/true);
      // Inverted (Orange) suit: PerformDiscard on a known orange would
      // attempt a play onto the orange stack via the game-rule inversion
      // (advance if currently playable, else a misplay strike). To actually
      // *discard* a known-orange card to the discard pile and regain a
      // clue, the bot must PerformPlay it.
      if (inferred &&
          s.variant->suits[inferred->suit_index].suit_type.inverted) {
        Action act = Action{PlayAction{s.our_player_index, o,
                                            inferred->suit_index, inferred->rank}};
        all_discards.emplace_back(PerformPlay{o}, std::move(act));
      } else {
        Action act = inferred
                          ? Action{DiscardAction{s.our_player_index, o, inferred->suit_index, inferred->rank, false}}
                          : Action{DiscardAction{s.our_player_index, o, -1, -1, false}};
        all_discards.emplace_back(PerformDiscard{o}, std::move(act));
      }
    }
  }

  // Force-play override: when we have a pending play AND no clue
  // would create a play in any other player's hand AND Bob's next
  // turn is safe (Bob has a known trash to discard, or his chop
  // isn't critical from our full visibility), trust the convention's
  // CTP and play — don't let a marginally-positive clue eval-win
  // against a small-rank play. (The forced-endgame and urgent paths
  // earlier in this function handle the corresponding "must clue"
  // and "must perform urgent" overrides — this is the symmetric
  // "must play" override.)
  if (!all_plays.empty()) {
    bool any_clue_creates_play = false;
    for (const auto& [perform_unused, act] : all_clues) {
      if (!std::holds_alternative<ClueAction>(act)) continue;
      Game hypo = simulate(act);
      auto [_blind, hypo_playables] = hanabi::playables_result(*this, hypo);
      if (!hypo_playables.empty()) {
        any_clue_creates_play = true;
        break;
      }
    }

    if (!any_clue_creates_play) {
      int bob = s.next_player_index(s.our_player_index);
      bool bob_safe = false;
      if (!common.thinks_trash(*this, bob).empty()) {
        bob_safe = true;
      } else if (auto bob_chop = chop(bob); !bob_chop) {
        // Locked — Bob won't discard, so no critical-loss risk.
        bob_safe = true;
      } else if (auto chop_id = s.deck[*bob_chop].id();
                 chop_id && !s.is_critical(*chop_id)) {
        bob_safe = true;
      }

      if (bob_safe) {
        auto best_play = std::max_element(
            all_plays.begin(), all_plays.end(),
            [&](const auto& a, const auto& b) {
              return hanabi::reactor::eval_action(*this, a.second) <
                     hanabi::reactor::eval_action(*this, b.second);
            });
        // Don't force-play a card whose inferred id is already CTP'd
        // (or visibly known to be played) in another player's hand —
        // that's a wasted duplicate. (Mirrors the existing
        // `identity_called_to_play_elsewhere` check in state_eval.cpp.)
        bool dup_ctp_elsewhere = false;
        int play_order = -1;
        if (auto* pp = std::get_if<PerformPlay>(&best_play->first)) {
          play_order = pp->target;
        } else if (auto* pd = std::get_if<PerformDiscard>(&best_play->first)) {
          play_order = pd->target;
        }
        if (play_order != -1) {
          auto play_id = m.thoughts[play_order].id(/*infer=*/true);
          if (play_id) {
            for (const auto& hand : s.hands) {
              for (int o : hand) {
                if (o == play_order) continue;
                if (meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
                auto cid = m.thoughts[o].id(/*infer=*/true);
                if (cid && *cid == *play_id) { dup_ctp_elsewhere = true; break; }
                auto did = s.deck[o].id();
                if (did && *did == *play_id) { dup_ctp_elsewhere = true; break; }
              }
              if (dup_ctp_elsewhere) break;
            }
          }
        }
        if (!dup_ctp_elsewhere) {
          return best_play->first;
        }
      }
    }
  }

  std::vector<std::pair<PerformAction, Action>> all_actions;
  for (auto& a : all_clues) all_actions.push_back(std::move(a));
  for (auto& a : all_plays) all_actions.push_back(std::move(a));
  for (auto& a : all_discards) all_actions.push_back(std::move(a));

  if (all_actions.empty()) {
    if (s.clue_tokens == 8) return PerformPlay{s.our_hand().front()};
    return PerformDiscard{m.locked_discard(s, s.our_player_index)};
  }

  auto best = std::max_element(
      all_actions.begin(), all_actions.end(),
      [&](const auto& a, const auto& b) {
        return hanabi::reactor::eval_action(*this, a.second) <
               hanabi::reactor::eval_action(*this, b.second);
      });
  return best->first;
}

std::vector<PerformAction> Game::find_all_discards(int player_index) const {
  auto trash = common.thinks_trash(*this, player_index);
  int target;
  if (!trash.empty()) {
    target = trash.front();
  } else if (auto c = chop(player_index)) {
    target = *c;
  } else {
    target = players[player_index].locked_discard(state, player_index);
  }
  return {PerformDiscard{target}};
}

}  // namespace hanabi
