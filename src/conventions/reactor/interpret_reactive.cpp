#include "hanabi/conventions/reactor/interpret_reactive.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"

namespace hanabi::reactor {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

// Reactor + inverted (Orange / Dark Orange) suits: when the receiver's
// reactive target is on an inverted suit, the reacter must invert their
// physical action (play↔discard) so that the receiver's standard reading
// of (clue kind + reacter action) ends up calling them to perform the
// physical action that *advances* the orange stack (or sends the orange
// card to the discard pile, depending on play_target vs dc_target). This
// helper reads the giver-visible identity of the target order.
bool target_is_inverted(const State& state, int target_order) {
  int suit_index = state.deck[target_order].suit_index;
  if (suit_index < 0) return false;
  return state.variant->suits[suit_index].suit_type.inverted;
}

// For an inverted-suit (Orange / Dark Orange) reacter card, the convention's
// reacter call has these outcomes via the game-rule inversion:
//   * target_play(orange) ⇒ on the reacter's turn the bot would issue
//     PerformPlay, which the inversion turns into "discard the orange" —
//     the orange card goes to the discard pile with no stack progress.
//     Always a losing convention path.
//   * target_discard(orange) ⇒ PerformDiscard, which the inversion turns
//     into "play attempt" — if the orange is currently playable this
//     advances the orange stack (the intended outcome); otherwise it is
//     a misplay strike.
// The receiver-orange swap may toggle play↔discard for some
// clue/target combinations; this helper answers the post-swap question of
// whether we are about to take a losing path on an inverted reacter card.
bool would_lose_inverted_reacter(const State& state, int react_order,
                                  bool receiver_target_inverted,
                                  bool standard_is_target_play) {
  int suit_index = state.deck[react_order].suit_index;
  if (suit_index < 0) return false;
  if (!state.variant->suits[suit_index].suit_type.inverted) return false;
  // The receiver-orange swap toggles play↔discard. After the swap, we end
  // up calling target_play iff (standard_is_target_play XOR
  // receiver_target_inverted) is false.
  const bool final_is_target_play = standard_is_target_play
                                       ? !receiver_target_inverted
                                       : receiver_target_inverted;
  if (final_is_target_play) return true;
  // target_discard on an orange reacter card: physical discard maps to a
  // play attempt under the game-rule inversion. Safe only when the orange
  // is currently playable on the orange stack — otherwise it is a misplay
  // strike, so reject.
  auto id = state.deck[react_order].id();
  if (!id) return false;
  return !state.is_playable(*id);
}

// Narrow `thought.possible` by visibility — drop any identity whose
// total copies (base_count + visible-in-other-hands) already equal the
// variant's card count, since those copies are accounted for elsewhere
// and the card we're checking can't be that identity. This anticipates
// the post-clue elim's visibility step so the convention rejects react
// candidates whose narrowed inferred would be elim'd away afterward.
IdentitySet effective_possible_for(const State& state, const Thought& thought,
                                    int self_order) {
  return thought.possible.filter([&](Identity id) {
    int seen = state.base_count[id.to_ord()];
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        if (o == self_order) continue;
        auto did = state.deck[o].id();
        if (did && *did == id) ++seen;
      }
    }
    return seen < state.card_count[id.to_ord()];
  });
}

struct ReactiveContext {
  std::vector<std::pair<int, Identity>> possible_conns;
  std::vector<int> known_plays;
  State hypo_state;
};

// Compute (possible_conns, known_plays, hypo_state) for the reactive
// interpretation. Port of _reactive_context.
ReactiveContext reactive_context(const Game& prev, const Game& game,
                                    const ClueAction& action, int reacter) {
  int giver = action.giver;
  int receiver = action.target;
  const State& state = game.state;

  ReactiveContext ctx;
  ctx.possible_conns = delayed_plays(game, giver, receiver, /*stable=*/false);
  auto old_playables = prev.common.obvious_playables(prev, receiver);
  auto new_playables = game.common.obvious_playables(game, receiver);
  for (int o : old_playables) {
    if (contains(new_playables, o)) ctx.known_plays.push_back(o);
  }

  State after_others = prev.state;
  for (int i : players_until(state.num_players, state.next_player_index(giver),
                                reacter)) {
    Game hypo_prev = prev;
    hypo_prev.state = after_others;
    auto ps = prev.common.obvious_playables(hypo_prev, i);
    std::optional<int> playable_o;
    for (int o : ps) {
      if (prev.meta[o].urgent) {
        playable_o = o;
        break;
      }
    }
    if (!playable_o && !ps.empty()) playable_o = ps.front();
    if (!playable_o) continue;
    auto id = state.deck[*playable_o].id();
    if (id) after_others = after_others.try_play(*id);
  }

  ctx.hypo_state = after_others;
  Game hypo_prev = prev;
  hypo_prev.state = after_others;
  auto self_plays = prev.common.obvious_playables(hypo_prev, receiver);
  for (int o : self_plays) {
    auto id = state.deck[o].id();
    if (id) ctx.hypo_state = ctx.hypo_state.try_play(*id);
  }
  return ctx;
}

}  // namespace

// --- interpret_reactive_colour ------------------------------------------

std::optional<ClueInterp> interpret_reactive_colour(const Game& prev, Game& game,
                                                       const ClueAction& action,
                                                       int focus_slot, int reacter,
                                                       bool looks_stable) {
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  // Find play targets in receiver's hand.
  std::vector<std::pair<int, int>> play_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    if (contains(ctx.known_plays, o)) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    play_targets.emplace_back(o, static_cast<int>(i));
  }

  // Sort: unclued dupe → 99 (last); else by slot index.
  auto sort_key = [&](const std::pair<int, int>& t) {
    int o = t.first;
    bool unclued_dupe = !prev.state.deck[o].clued;
    if (unclued_dupe) {
      for (int o2 : state.hands[receiver]) {
        if (o2 < o && prev.state.deck[o2].clued &&
            state.deck[o].matches(state.deck[o2])) {
          return 99;
        }
      }
    }
    return t.second;
  };
  std::sort(play_targets.begin(), play_targets.end(),
             [&](const auto& a, const auto& b) { return sort_key(a) < sort_key(b); });

  int hand_size = kHandSize[state.num_players];
  for (const auto& [_target, index] : play_targets) {
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    if (looks_stable) {
      auto rkt = prev.common.thinks_trash(prev, reacter);
      auto rop = prev.common.obvious_playables(prev, reacter);
      if (contains(rkt, react_order) && rop.empty()) continue;
    }
    if (game.common.thoughts[react_order].possible.forall(
            [&](Identity i) { return state.is_critical(i); })) {
      continue;
    }
    // Skip this play-target if the resulting convention call would be
    // target_play on an inverted-suit reacter card — that path would
    // PerformPlay the orange and dump it to the discard pile via the
    // game-rule inversion (no stack advance, orange copy lost). Try the
    // next target instead.
    if (would_lose_inverted_reacter(state, react_order,
                                       target_is_inverted(state, _target),
                                       /*standard_is_target_play=*/false)) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    // Inverted-suit target: swap reacter's intended physical action so the
    // receiver's standard (clue kind + reacter action) reading ends up
    // calling them to perform the orange-aware action.
    auto interp = target_is_inverted(state, _target)
                       ? target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false)
                       : target_discard(game, action, react_order, /*urgent=*/true);
    if (!interp) return std::nullopt;
    // Stamp the receiver's `_target` so hypo_plays sees the second
    // play (parallel to the rank path above; see the rank comment for
    // the rationale).
    {
      int turn = state.turn_count;
      int giver = action.giver;
      CardStatus target_status = target_is_inverted(state, _target)
                                      ? CardStatus::CALLED_TO_DISCARD
                                      : CardStatus::CALLED_TO_PLAY;
      game.with_meta(_target, [turn, giver, target_status](ConvData& m) {
        m.status = target_status;
        m.by = giver;
        m = m.reason(turn).signal(turn);
      });
    }
    return ClueInterp::REACTIVE;
  }

  // Discard targets.
  auto prev_kt = prev.common.thinks_trash(prev, receiver);
  std::vector<std::pair<int, int>> unknown_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    auto deck_id = state.deck[o].id();
    if (!deck_id) continue;
    bool is_trash = state.is_basic_trash(*deck_id);
    if (!is_trash) {
      for (int o2 : state.hands[receiver]) {
        if (o2 < o && state.deck[o].matches(state.deck[o2])) {
          is_trash = true;
          break;
        }
      }
    }
    if (is_trash) unknown_trash.emplace_back(o, static_cast<int>(i));
  }
  std::sort(unknown_trash.begin(), unknown_trash.end(),
             [&](const auto& a, const auto& b) {
               int ka = prev.state.deck[a.first].clued ? -1 : 1;
               int kb = prev.state.deck[b.first].clued ? -1 : 1;
               return ka < kb;
             });

  std::vector<std::pair<int, int>> unknown_dupes;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    bool dup = false;
    for (const auto& hand : state.hands) {
      for (int o2 : hand) {
        if (o2 != o && game.common.thoughts[o2].matches(state.deck[o], /*infer=*/true)) {
          dup = true;
          break;
        }
      }
      if (dup) break;
    }
    if (dup) unknown_dupes.emplace_back(o, static_cast<int>(i));
  }

  std::vector<std::pair<int, int>> known_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    auto id = state.deck[o].id();
    if (id && state.is_basic_trash(*id)) {
      known_trash.emplace_back(o, static_cast<int>(i));
    }
  }

  std::vector<std::pair<int, int>> sacrifices;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(prev_kt, o)) continue;
    auto id = state.deck[o].id();
    if (id && !state.is_critical(*id)) sacrifices.emplace_back(o, static_cast<int>(i));
  }
  std::sort(sacrifices.begin(), sacrifices.end(),
             [&](const auto& a, const auto& b) {
               auto ai = state.deck[a.first].id();
               auto bi = state.deck[b.first].id();
               int ka = -game.common.playable_away(*ai) * 10 + (5 - ai->rank);
               int kb = -game.common.playable_away(*bi) * 10 + (5 - bi->rank);
               return ka < kb;
             });

  std::vector<std::pair<int, int>> dc_targets;
  if (!unknown_trash.empty()) dc_targets = unknown_trash;
  else if (!known_trash.empty()) dc_targets = known_trash;
  else if (!unknown_dupes.empty()) dc_targets = unknown_dupes;
  else dc_targets = sacrifices;

  if (dc_targets.empty()) return std::nullopt;

  for (const auto& [target, index] : dc_targets) {
    if (state.next_player_index(action.giver) != reacter &&
        game.meta[target].status == CardStatus::CALLED_TO_PLAY) {
      continue;
    }
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    bool ok = game.common.thoughts[react_order].possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    // Skip this dc-target if it would resolve to target_play on an
    // inverted-suit reacter (orange would go to discard pile, lost).
    if (would_lose_inverted_reacter(state, react_order,
                                       target_is_inverted(state, target),
                                       /*standard_is_target_play=*/true)) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    // Inverted-suit dc-target: swap reacter intent to keep the receiver's
    // physical action consistent with sending the orange card to the
    // discard pile (which is what the convention's "dc target" implies).
    auto interp = target_is_inverted(state, target)
                       ? target_discard(game, action, react_order, /*urgent=*/true)
                       : target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false);
    if (!interp) return std::nullopt;
    return ClueInterp::REACTIVE;
  }
  return std::nullopt;
}

// --- interpret_reactive_rank --------------------------------------------

std::optional<ClueInterp> interpret_reactive_rank(const Game& prev, Game& game,
                                                     const ClueAction& action,
                                                     int focus_slot, int reacter) {
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  std::vector<std::pair<int, int>> play_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (game.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    if (contains(ctx.known_plays, o)) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    play_targets.emplace_back(o, static_cast<int>(i));
  }
  auto sort_key = [&](const std::pair<int, int>& t) {
    int o = t.first;
    if (!prev.state.deck[o].clued) {
      for (int o2 : state.hands[receiver]) {
        if (o2 != o && prev.state.deck[o2].clued &&
            state.deck[o].matches(state.deck[o2])) {
          return 99;
        }
      }
    }
    return t.second;
  };
  std::sort(play_targets.begin(), play_targets.end(),
             [&](const auto& a, const auto& b) { return sort_key(a) < sort_key(b); });

  int hand_size = kHandSize[state.num_players];
  for (const auto& [target, index] : play_targets) {
    int target_slot = index + 1;
    int react_slot = calc_slot(focus_slot, target_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    IdentitySet effective_possible =
        effective_possible_for(game.state, game.common.thoughts[react_order],
                               react_order);
    bool ok = effective_possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    // Skip this rank play-target if it would resolve to target_play on an
    // inverted-suit reacter (orange goes to discard pile, lost).
    if (would_lose_inverted_reacter(state, react_order,
                                       target_is_inverted(state, target),
                                       /*standard_is_target_play=*/true)) {
      continue;
    }
    // Inverted-suit play-target on a rank clue: swap reacter intent to
    // discard so the receiver reads (rank + reacter discards) →
    // target_i_discard on the orange target, whose physical discard
    // advances the orange stack via the game-rule inversion.
    auto interp = target_is_inverted(state, target)
                       ? target_discard(game, action, react_order, /*urgent=*/true)
                       : target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false);
    if (!interp) return std::nullopt;
    // Stamp the receiver's `target` so hypo_plays sees it as the
    // second play. The receiver figures out their target via the
    // slot-mapping rule, but until this stamp the convention only
    // recorded the reacter's CTP — `playables_result` returned
    // size=1 for genuine 2-play reactives, and the +10 REACTIVE
    // 2-play eval bonus never fired. For an inverted-suit target the
    // receiver's physical action is PerformDiscard (= play attempt
    // via on_discard inversion), so the stamp is CTD; otherwise CTP.
    {
      int turn = state.turn_count;
      int giver = action.giver;
      CardStatus target_status = target_is_inverted(state, target)
                                      ? CardStatus::CALLED_TO_DISCARD
                                      : CardStatus::CALLED_TO_PLAY;
      game.with_meta(target, [turn, giver, target_status](ConvData& m) {
        m.status = target_status;
        m.by = giver;
        m = m.reason(turn).signal(turn);
      });
    }
    auto target_id = state.deck[target].id();
    if (target_id) {
      Identity ti = *target_id;
      game.with_thought(react_order, [ti](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(ti);
        return out;
      });
    }
    return ClueInterp::REACTIVE;
  }

  // Finesse fallback.
  std::vector<std::pair<int, int>> finesse_targets;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    auto id = state.deck[o].id();
    if (id && state.playable_away(*id) == 1) {
      finesse_targets.emplace_back(o, static_cast<int>(i));
    }
  }
  if (finesse_targets.empty()) return std::nullopt;

  for (int react_slot : {1, 5, 4, 3, 2}) {
    int target_slot = calc_slot(focus_slot, react_slot, hand_size);
    if (react_slot < 1 ||
        react_slot > static_cast<int>(state.hands[reacter].size())) {
      continue;
    }
    int react_order = state.hands[reacter][react_slot - 1];
    std::optional<std::pair<int, int>> receive;
    for (const auto& [o, i] : finesse_targets) {
      if (i + 1 == target_slot) {
        receive = std::make_pair(o, i);
        break;
      }
    }
    if (!receive) continue;
    int receive_order = receive->first;
    auto prev_plays = prev.common.obvious_playables(prev, reacter);
    if (contains(prev_plays, react_order)) continue;
    auto deck_id = state.deck[receive_order].id();
    if (!deck_id) continue;
    auto prev_id = deck_id->prev();
    if (!prev_id) continue;
    IdentitySet effective_possible =
        effective_possible_for(game.state, game.common.thoughts[react_order],
                               react_order);
    bool ok = effective_possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    if (!effective_possible.contains(*prev_id)) continue;
    // POV-invariant guard. The reacter picks `react_slot` from
    // {1, 5, 4, 3, 2} using only `effective_possible` (common
    // knowledge), so giver / receiver / reacter all agree on which
    // slot the convention points at. From the giver's POV (or any POV
    // that can see the reacter's hand directly) we *additionally* know
    // whether the reacter's actual card is the prereq. If it isn't,
    // the reacter will PerformDiscard (or PerformPlay) a card that
    // strikes — abort the whole reactive interpretation as a MISTAKE
    // rather than silently picking a different slot. (No "try next
    // slot" — the reacter, from her own POV, will still pick THIS
    // slot when she acts; no later iteration can rescue it.)
    auto react_actual_id = state.deck[react_order].id();
    if (react_actual_id && *react_actual_id != *prev_id) {
      return std::nullopt;
    }
    // Skip this finesse if it would resolve to target_play on an
    // inverted-suit reacter (orange goes to discard pile, lost).
    if (would_lose_inverted_reacter(state, react_order,
                                       target_is_inverted(state, receive_order),
                                       /*standard_is_target_play=*/true)) {
      continue;
    }
    game.with_thought(react_order, [](const Thought& t) {
      Thought out = t;
      out.old_inferred = t.inferred;
      return out;
    });
    // Inverted-suit finesse target: swap reacter intent so the receiver
    // ends up performing the physical action that advances the orange
    // stack on the finessed orange card.
    auto interp = target_is_inverted(state, receive_order)
                       ? target_discard(game, action, react_order, /*urgent=*/true)
                       : target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false);
    if (!interp) return std::nullopt;
    Identity pi = *prev_id;
    game.with_thought(react_order, [pi](const Thought& t) {
      Thought out = t;
      out.inferred = IdentitySet::single(pi);
      return out;
    });
    return ClueInterp::REACTIVE;
  }

  // Chop-save fallback. If no play_target / finesse worked and the
  // receiver's chop is an inverted-suit (orange) card, encode the rank
  // reactive as "receiver PerformPlay's chop (= orange game-rule discard
  // pile, a clean voluntary loss that avoids the misplay strike that
  // would come from BOB PerformDiscard'ing a non-playable orange chop).
  // Reacter does PerformPlay → CTP on reacter, mirroring the convention
  // "rank + reacter plays = receiver plays."
  std::optional<int> receiver_chop;
  for (int o : state.hands[receiver]) {
    if (!state.deck[o].clued && game.meta[o].status == CardStatus::NONE) {
      receiver_chop = o;
      break;
    }
  }
  if (!receiver_chop) return std::nullopt;
  auto chop_id = state.deck[*receiver_chop].id();
  if (!chop_id ||
      !state.variant->suits[chop_id->suit_index].suit_type.inverted) {
    // Only the orange (inverted) case is encoded as a chop-save here.
    // Non-orange chops would require a target_discard on the reacter,
    // which is unsafe without a critical-check from the giver's POV
    // that the observer can't perform on their own card. Bail.
    return std::nullopt;
  }
  int chop_index = -1;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    if (state.hands[receiver][i] == *receiver_chop) {
      chop_index = static_cast<int>(i);
      break;
    }
  }
  if (chop_index < 0) return std::nullopt;
  int chop_target_slot = chop_index + 1;
  int chop_react_slot = calc_slot(focus_slot, chop_target_slot, hand_size);
  if (chop_react_slot < 1 ||
      chop_react_slot > static_cast<int>(state.hands[reacter].size())) {
    return std::nullopt;
  }
  int chop_react_order = state.hands[reacter][chop_react_slot - 1];
  auto chop_prev_plays = prev.common.obvious_playables(prev, reacter);
  if (contains(chop_prev_plays, chop_react_order)) return std::nullopt;
  IdentitySet chop_effective =
      effective_possible_for(game.state,
                              game.common.thoughts[chop_react_order],
                              chop_react_order);
  bool chop_has_playable = chop_effective.exists([&](Identity i) {
    if (state.playable_set.contains(i)) return true;
    for (const auto& [_, c] : ctx.possible_conns) {
      if (c == i) return true;
    }
    return false;
  });
  if (!chop_has_playable) return std::nullopt;
  // Don't choose a reacter whose own physical play would lose an
  // inverted card (orange PerformPlay = discard pile).
  if (would_lose_inverted_reacter(state, chop_react_order,
                                     /*receiver_target_inverted=*/true,
                                     /*standard_is_target_play=*/false)) {
    return std::nullopt;
  }
  auto chop_interp =
      target_play(game, action, chop_react_order, /*urgent=*/true,
                  /*stable=*/false);
  if (!chop_interp) return std::nullopt;
  return ClueInterp::REACTIVE;
}

}  // namespace hanabi::reactor
