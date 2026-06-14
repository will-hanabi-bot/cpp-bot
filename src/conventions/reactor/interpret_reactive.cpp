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

// Narrow `thought.possible` by visibility from the HOLDER's POV.
//
// v0.23: previously this iterated *all* hands and used the computing
// bot's `state.deck[o].id()`. That mixed two visibilities — the
// computing bot's, and the holder's — and produced different
// answers depending on which bot was computing. Replay 1892112 T11
// surfaced this: the giver could see the reacter's own slot-3 p2,
// dropped p2 from the reacter's slot-5 `effective_possible`, and
// skipped the receiver-target it should have picked; the reacter
// couldn't see its own slot 3, so kept p2 and picked it. Two bots,
// two convention interpretations.
//
// Fix: use the HOLDER's POV by excluding the holder's entire hand
// from the visibility count. The reacter sees every non-self hand,
// and every other bot can model exactly that vision via
// `state.deck[o].id()` because every non-holder hand is visible to
// every non-holder bot. The remaining POV-asymmetry — a bot can't
// see its own hand — only matters when the computing bot IS in a
// non-holder hand, which is acceptable: the convention's role-
// specific decisions (reacter's action, receiver's narrowing) are
// the ones that need to agree, and those bots compute the same
// answer from their own POV via this rule.
//
// (Cross-test 1884192 + 1892112: the holder's POV rule lets a
// reacter who can see another player's i5 drop i5 from their own
// possible — the 1884192 fix — while keeping the reacter's slot p2
// possible when no non-reacter hand has p2 — the 1892112 fix.)
IdentitySet effective_possible_for(const Game& game, int self_order) {
  const State& state = game.state;
  const Thought& thought = game.common.thoughts[self_order];
  int holder = state.holder_of(self_order);
  return thought.possible.filter([&](Identity id) {
    int seen = state.base_count[id.to_ord()];
    for (int p = 0; p < static_cast<int>(state.hands.size()); ++p) {
      if (p == holder) continue;
      for (int o : state.hands[p]) {
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
  // Advance hypo_state through the receiver's empathy-known plays. Using
  // `thinks_playables(exclude_trash=true)` (inferred-aware via
  // `order_playable`) instead of `obvious_playables` (possible-only via
  // `order_kp`) catches good-touch-narrowed plays whose `status` is still
  // NONE — e.g. a clued slot whose `possible` is `{b1..b4, i4}` reduces
  // to the singleton playable `{i4}` once basic-trash ids are filtered
  // out. Without this advance, the play-target loop below treats those
  // cards as the convention's primary play target even though they would
  // play naturally; the next playable in the chain (the card that
  // actually needs the clue's narrowing) never surfaces.
  //
  // `good_touch_elim` is gated on `game.good_touch` (which the reactor
  // pipeline leaves at false), so `thoughts.inferred` itself isn't
  // narrowed by `trash_set`. We do the narrowing inline here against
  // `prev.common.thoughts[o].possibilities()` (= inferred if non-empty,
  // else possible), filtering out `trash_set`. POV invariance: this uses
  // `common.thoughts[o]` which every observer agrees on, unlike
  // `state.deck[o].id()` which is hidden for the receiver's own cards.
  // The `is_playable` guard prevents duplicate-singleton links (two
  // cards both narrowing to the same playable id) from double-advancing
  // the same stack.
  auto self_plays = prev.common.thinks_playables(hypo_prev, receiver,
                                                    /*exclude_trash=*/true);
  for (int o : self_plays) {
    // Only advance through cards the receiver would naturally PLAY —
    // skip cards already CTD'd (slated for discard) or otherwise
    // earmarked for non-play actions (sarcastic, gentleman's discard).
    // `thinks_playables` can pull in a CTD'd card if its (still-broad)
    // `inferred` happens to narrow to a single non-trash playable id
    // after the trash_set filter; the convention has explicitly told
    // the holder to discard it, so we must not assume it will play and
    // advance the stack. (Replay 1875304 T21: a CTD'd g1 narrowed to
    // {r4} via this trash filter; advancing r_stack via the false
    // "r4 play" pushed the convention's reactive picks to the wrong
    // slot.)
    CardStatus st = prev.meta[o].status;
    if (st != CardStatus::NONE && st != CardStatus::CALLED_TO_PLAY) continue;
    IdentitySet effective =
        prev.common.thoughts[o].possibilities().difference(prev.state.trash_set);
    if (effective.length() != 1) continue;
    Identity id = effective.head();
    if (ctx.hypo_state.is_playable(id)) {
      ctx.hypo_state = ctx.hypo_state.try_play(id);
    }
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
    // CTD'd cards are eligible as play targets *if* they're currently
    // playable — the stack may have caught up to a card we earlier
    // marked for discard. The is_playable check below filters
    // CTD'd-not-yet-playable cards.
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
    // v0.23: continue on target_play/discard failure (see comment in
    // rank path).
    if (!interp) continue;
    // Narrow the receiver target's inferred to (playable_set ∪
    // next-ranks-of-reacter-inferred). Without this, the CTP'd receiver
    // target retains the wide post-basic-clue-elim empathy, polluting
    // downstream [f] notes and finesse / connection inference (see
    // replay 1890204 T3 for the symptom). Mirrors target_play's
    // narrowing pattern (interpret_clue.cpp:189-260); `delayed_plays`
    // walks players between giver and the receiver and yields each
    // urgent/obvious-playable's next-rank successor.
    {
      int holder = state.holder_of(_target);
      auto receiver_conns =
          delayed_plays(game, action.giver, holder, /*stable=*/false);
      // Filter the receiver target's inferred against the playable set
      // *after* the receiver's empathy-known self-plays have advanced
      // hypo_state. Without this, a play target like i5 (which only
      // becomes playable once the receiver plays their already-known i4)
      // would be intersected against the pre-advance playable_set and
      // narrow to empty — falsely rejecting the convention's actual
      // intended target. Reacter checks elsewhere still use
      // state.playable_set because the reacter acts *before* the
      // receiver's self-plays.
      IdentitySet ps = ctx.hypo_state.playable_set;
      IdentitySet narrowed = game.common.thoughts[_target].inferred.filter(
          [&](Identity i) {
            if (ps.contains(i)) return true;
            for (const auto& [_, c] : receiver_conns) {
              if (c == i) return true;
            }
            return false;
          });
      if (narrowed.is_empty()) continue;
      game.with_thought(_target, [&narrowed](const Thought& t) {
        Thought out = t;
        out.old_inferred = t.inferred;
        out.inferred = narrowed;
        return out;
      });
    }
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

  // Pre-clued slots that post-clue common-knowledge `order_kt` marks
  // as basic-trash. When the receiver has a slot that was clued
  // *before* this turn AND the current clue's narrowing on
  // `common.thoughts.possible` is enough to push every remaining id
  // into `state.trash_set`, the clue's effect is to disambiguate
  // that slot as trash. The reactive interp promotes the reacter to
  // play the slot mapped via `calc_slot`, which:
  //   (a) advances a stack (the reacter's mapped slot is currently
  //       playable), vs. only revealing trash the receiver would
  //       discard on chop anyway;
  //   (b) gives the receiver the disambiguation that the giver's
  //       clue was *about* that pre-clued slot — they can confidently
  //       discard it in the future.
  // POV invariance: the membership uses `game.common.thinks_trash`
  // (post-clue, common-knowledge), so giver / receiver / reacter all
  // compute the same candidate set. The clued-before-this-turn check
  // uses `prev.state.deck[o].clued`, also POV-invariant.
  // Replay 1892505 T32: a colour-red clue to wb69 narrows the pre-
  // clued slot 4 (i2, prior possible {r2,r3,y2,g3,i2,i3}) to {y2,g3,
  // i2,i3} via the red-untouched filter — every survivor is basic-
  // trash at stacks y=5/g=5/i=3, so post-clue `order_kt(slot 4)` is
  // true. focus_slot=3 (newest-demoted) + target_slot=4 ⟹ react_slot=
  // 4, mapping to wb67's slot 4 = i4 (currently playable on prism
  // stack=3).
  auto game_kt = game.common.thinks_trash(game, receiver);
  std::vector<std::pair<int, int>> pre_clued_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (!prev.state.deck[o].clued) continue;
    if (!contains(game_kt, o)) continue;
    pre_clued_trash.emplace_back(o, static_cast<int>(i));
  }

  std::vector<std::pair<int, int>> dc_targets;
  if (!pre_clued_trash.empty()) dc_targets = pre_clued_trash;
  else if (!unknown_trash.empty()) dc_targets = unknown_trash;
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
    // v0.28: continue on target_play/target_discard failure rather than
    // bailing the entire reactive — mirrors the rank-path v0.23 fix at
    // `interpret_reactive_rank`. With the new `pre_clued_trash` pool
    // sometimes producing multiple candidate dc-targets, a single
    // failure on the first one shouldn't abort the whole interp.
    if (!interp) continue;
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
    // CTD'd cards are eligible as play targets *if* they're currently
    // playable — the stack may have caught up to a card we earlier
    // marked for discard. The is_playable check below filters the
    // not-yet-playable CTD'd cards.
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
    IdentitySet effective_possible = effective_possible_for(game, react_order);
    bool ok = effective_possible.exists([&](Identity i) {
      if (state.playable_set.contains(i)) return true;
      for (const auto& [_, c] : ctx.possible_conns) {
        if (c == i) return true;
      }
      return false;
    });
    if (!ok) continue;
    if (would_lose_inverted_reacter(state, react_order,
                                       target_is_inverted(state, target),
                                       /*standard_is_target_play=*/true)) {
      continue;
    }
    auto interp = target_is_inverted(state, target)
                       ? target_discard(game, action, react_order, /*urgent=*/true)
                       : target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false);
    // v0.23: continue on target_play failure rather than bailing the
    // entire reactive interp. Lets the convention fall through when
    // the first play_target's react_slot lands on an inconsistent
    // reacter card (e.g. visible-id doesn't match the narrowed
    // inferred). Replay 1885536 T22 + 1892112 T11 both exercise this.
    if (!interp) continue;
    // Narrow the receiver target's inferred to (playable_set ∪
    // next-ranks-of-reacter-inferred). Without this, the CTP'd receiver
    // target retains the wide post-basic-clue-elim empathy, polluting
    // downstream [f] notes and finesse / connection inference (see
    // replay 1890204 T3 for the symptom). Mirrors target_play's
    // narrowing pattern (interpret_clue.cpp:189-260); `delayed_plays`
    // walks players between giver and the receiver and yields each
    // urgent/obvious-playable's next-rank successor.
    {
      int holder = state.holder_of(target);
      auto receiver_conns =
          delayed_plays(game, action.giver, holder, /*stable=*/false);
      // Filter the receiver target's inferred against the playable set
      // *after* the receiver's empathy-known self-plays have advanced
      // hypo_state. Without this, a play target like i5 (which only
      // becomes playable once the receiver plays their already-known i4)
      // would be intersected against the pre-advance playable_set and
      // narrow to empty — falsely rejecting the convention's actual
      // intended target. Reacter checks elsewhere still use
      // state.playable_set because the reacter acts *before* the
      // receiver's self-plays.
      IdentitySet ps = ctx.hypo_state.playable_set;
      IdentitySet narrowed = game.common.thoughts[target].inferred.filter(
          [&](Identity i) {
            if (ps.contains(i)) return true;
            for (const auto& [_, c] : receiver_conns) {
              if (c == i) return true;
            }
            return false;
          });
      if (narrowed.is_empty()) continue;
      game.with_thought(target, [&narrowed](const Thought& t) {
        Thought out = t;
        out.old_inferred = t.inferred;
        out.inferred = narrowed;
        return out;
      });
    }
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
    IdentitySet effective_possible = effective_possible_for(game, react_order);
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
  IdentitySet chop_effective = effective_possible_for(game, chop_react_order);
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
