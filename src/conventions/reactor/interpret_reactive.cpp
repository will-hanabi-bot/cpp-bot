#include "hanabi/conventions/reactor/interpret_reactive.h"

#include <algorithm>
#include <set>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/variants/inverted.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/decide_trace.h"

namespace hanabi::reactor {

namespace {

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

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

namespace {

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
  // v0.37: `known_plays` is the set of receiver cards that should be
  // excluded from the reactive convention's play-target pool because
  // "the receiver already knows to play this — the clue shouldn't be
  // wasted re-signalling it." Tightened to require a *strictly*
  // singleton common-knowledge identity (`thought.id()` with no
  // `infer` flag, no trash-elim narrowing). The looser pre-v0.37
  // construction — intersection of `obvious_playables(prev) ∩
  // obvious_playables(game)` — pulled in cards that were merely
  // good-touch-narrowed to "the only useful identity in inferred",
  // which conflicts with the human reactive convention that wraps a
  // known-playable focus card with the reacter's blind play for a
  // 2-play tempo gain (replay 1899567 T21).
  for (int o : state.hands[receiver]) {
    auto id = game.common.thoughts[o].id();
    if (id && state.is_playable(*id)) ctx.known_plays.push_back(o);
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
  // v0.37: was `prev.common.thinks_playables(hypo_prev, receiver,
  // exclude_trash=true)` — pulled in cards whose order_playable was
  // only true via the trash-elim narrowing (clued card whose possible
  // = {r1, r2, r3, r5} reduces to "must be r5 since the others are
  // trash"). Advancing hypo_state through such cards (as if the
  // receiver were guaranteed to play r5 naturally) caused
  // `is_playable(r5)` to return false in the all_playable loop below,
  // dropping r5 from the reactive play-target pool. Replay 1899567
  // T21: wb67 then fell through to a b4 finesse instead of the
  // intended r5 reactive double-play.
  //
  // Strict replacement: only advance through cards whose effective
  // identity is determined WITHOUT trash-elim — i.e. inferred is a
  // strict singleton, or info_lock is a strict singleton that the
  // visible card matches.
  auto self_plays = prev.common.obvious_playables(hypo_prev, receiver);
  for (int o : self_plays) {
    // Only advance through cards the receiver would naturally PLAY —
    // skip cards already CTD'd (slated for discard) or otherwise
    // earmarked for non-play actions (sarcastic, gentleman's discard).
    CardStatus st = prev.meta[o].status;
    if (st != CardStatus::NONE && st != CardStatus::CALLED_TO_PLAY) continue;
    // Strict singleton: require the card to be unambiguously identified
    // (no good-touch / trash-elim narrowing).
    auto id = prev.common.thoughts[o].id();
    if (!id) continue;
    if (ctx.hypo_state.is_playable(*id)) {
      ctx.hypo_state = ctx.hypo_state.try_play(*id);
    }
  }
  // v1.5 (replay 1916815): additionally simulate ALL of the receiver's
  // pending CTP'd cards through the hypo stacks, using observer-visible
  // deck ids — the receiver will play their queue on their own, so the
  // convention's next play target is picked against the post-queue
  // stacks. This both retires the CTP'd card from the play-target pool
  // (m1 stops being hypo-playable once simulated) and surfaces its
  // successor (the newly drawn m2 becomes hypo-playable). A play target
  // must never stack on a card that is already CALLED_TO_PLAY. POV: the
  // receiver's deck ids are visible to the giver and the reacter; the
  // receiver's own POV never runs this path (interpret_reactive returns
  // early for receiver == our_player_index). Fixpoint so CTP chains
  // advance in stack order.
  bool advanced = true;
  while (advanced) {
    advanced = false;
    for (int o : state.hands[receiver]) {
      if (prev.meta[o].status != CardStatus::CALLED_TO_PLAY) continue;
      auto id = state.deck[o].id();
      if (!id || !ctx.hypo_state.is_playable(*id)) continue;
      ctx.hypo_state = ctx.hypo_state.try_play(*id);
      advanced = true;
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
  hanabi::instr::ScopedTimer st("reactor.interpret_reactive_colour");
  hanabi::logging::LogScope ls(
      "reactor.interpret_reactive_colour",
      {{"focus_slot", focus_slot}, {"reacter", reacter},
        {"looks_stable", looks_stable}});
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  // Find play targets in receiver's hand.
  // v0.33: among multiple copies of the same playable identity in
  // the receiver's hand, only the RIGHTMOST copy (highest slot index
  // = oldest) is a "primary" play target. The lefter copies are
  // treated as trash for primary selection and tried only as a
  // right-to-left fallback if no primary react_slot resolves on Bob.
  // CTD'd cards are eligible as play targets *if* they're currently
  // playable — the stack may have caught up to a card we earlier
  // marked for discard. The is_playable check below filters
  // CTD'd-not-yet-playable cards.
  std::vector<std::pair<int, int>> all_playable;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(ctx.known_plays, o)) continue;
    // A play target never stacks on an already-CTP'd card (replay
    // 1916815): its play is already queued, and the pending-CTP
    // simulation in reactive_context makes its successor hypo-playable
    // — the successor is the eligible target.
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    all_playable.emplace_back(o, static_cast<int>(i));
  }
  std::vector<std::pair<int, int>> play_targets;
  std::vector<std::pair<int, int>> dupe_targets;
  {
    // Walk slot DESCENDING so the first encounter of each identity
    // is the rightmost copy → that one becomes primary; subsequent
    // (lefter) copies of the same identity become dupes.
    std::vector<std::pair<int, int>> by_slot_desc = all_playable;
    std::sort(by_slot_desc.begin(), by_slot_desc.end(),
               [](const auto& a, const auto& b) { return a.second > b.second; });
    std::set<int> seen_id;
    for (const auto& p : by_slot_desc) {
      auto id = state.deck[p.first].id();
      int id_ord = id->to_ord();
      if (seen_id.insert(id_ord).second) {
        play_targets.push_back(p);
      } else {
        dupe_targets.push_back(p);
      }
    }
  }
  // Primary play_targets sorted slot ASCENDING (leftmost first).
  std::sort(play_targets.begin(), play_targets.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
  // dupe_targets stays slot DESCENDING (iterated right-to-left).
  play_targets.insert(play_targets.end(), dupe_targets.begin(),
                       dupe_targets.end());

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
    if (variants::would_lose_inverted_reacter(state, react_order,
                                       variants::target_is_inverted(state, _target),
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
    auto interp = variants::target_is_inverted(state, _target)
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
      CardStatus target_status = variants::target_is_inverted(state, _target)
                                      ? CardStatus::CALLED_TO_DISCARD
                                      : CardStatus::CALLED_TO_PLAY;
      game.with_meta(_target, [turn, giver, target_status](ConvData& m) {
        m.status = target_status;
        m.by = giver;
        m = m.reason(turn).signal(turn);
      });
    }
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
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
  // v1.4 dc-target rule (replay 1916813; ordering confirmed by user):
  //   1.1 target the LEFTMOST CLUED trash card that is not already
  //       globally known to be trash — marking it teaches the receiver
  //       that a card they were keeping for its clue is actually trash
  //       (1916813: the clued n2 behind a useful-looking {g2,g5,b2,b5}
  //       empathy);
  //   1.2 else the LEFTMOST UNCLUED not-known trash card;
  //   2.1 else the LEFTMOST globally-known trash card (known_trash pool);
  //   2.2 else sacrifice.
  // `unknown_trash` is built slot-ascending, so a stable partition by
  // clued-ness keeps "leftmost" within each group. Known-ness is
  // evaluated on the PREV state (`prev_kt` filter above), so a pre-clued
  // card that this clue disambiguates into trash sits at the front of
  // the clued group (replay 1892505 T32).
  std::stable_sort(unknown_trash.begin(), unknown_trash.end(),
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

  // Pre-clued slots that post-clue common-knowledge `order_kt` marks as
  // basic-trash: the clue's point is disambiguating that slot as trash,
  // so it outranks the other pools (v0.38, replay 1892505 T32 — see
  // tests/test_basics/test_reactive.cpp for the full narrative). POV
  // invariance: membership uses `game.common.thinks_trash` (post-clue,
  // common knowledge) and `prev.state.deck[o].clued`.
  auto game_kt = game.common.thinks_trash(game, receiver);
  std::vector<std::pair<int, int>> pre_clued_trash;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (!prev.state.deck[o].clued) continue;
    if (!contains(game_kt, o)) continue;
    pre_clued_trash.emplace_back(o, static_cast<int>(i));
  }

  // Cascade per the v1.4 rule above.
  std::vector<std::pair<int, int>> dc_targets;
  bool chose_sacrifices = false;
  if (!pre_clued_trash.empty()) dc_targets = pre_clued_trash;
  else if (!unknown_trash.empty()) dc_targets = unknown_trash;
  else if (!known_trash.empty()) dc_targets = known_trash;
  else if (!unknown_dupes.empty()) dc_targets = unknown_dupes;
  else { dc_targets = sacrifices; chose_sacrifices = true; }

  // v0.30 convention: don't retarget to a card that is already CTD'd
  // (re-CTD adds no info — see Example 3) or whose identity duplicates
  // an existing CTD'd card in the same hand (double-discarding the
  // identity would lose it entirely — see Example 4). When the filter
  // empties the chosen pool, fall back to filtered sacrifices.
  auto target_is_blocked = [&](int target) {
    if (game.meta[target].status == CardStatus::CALLED_TO_DISCARD) return true;
    auto tid = state.deck[target].id();
    if (!tid) return false;
    for (int o2 : state.hands[receiver]) {
      if (o2 == target) continue;
      auto oid = state.deck[o2].id();
      if (oid && *oid == *tid &&
          game.meta[o2].status == CardStatus::CALLED_TO_DISCARD) {
        return true;
      }
    }
    return false;
  };
  dc_targets.erase(
      std::remove_if(dc_targets.begin(), dc_targets.end(),
                      [&](const auto& p) { return target_is_blocked(p.first); }),
      dc_targets.end());
  if (dc_targets.empty() && !chose_sacrifices) {
    sacrifices.erase(
        std::remove_if(sacrifices.begin(), sacrifices.end(),
                        [&](const auto& p) { return target_is_blocked(p.first); }),
        sacrifices.end());
    dc_targets = sacrifices;
  }

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
    if (variants::would_lose_inverted_reacter(state, react_order,
                                       variants::target_is_inverted(state, target),
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
    auto interp = variants::target_is_inverted(state, target)
                       ? target_discard(game, action, react_order, /*urgent=*/true)
                       : target_play(game, action, react_order, /*urgent=*/true,
                                        /*stable=*/false);
    // v0.28: continue on target_play/target_discard failure rather than
    // bailing the entire reactive — mirrors the rank-path v0.23 fix at
    // `interpret_reactive_rank`. With the new `pre_clued_trash` pool
    // sometimes producing multiple candidate dc-targets, a single
    // failure on the first one shouldn't abort the whole interp.
    if (!interp) continue;
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }
  return std::nullopt;
}

// --- interpret_reactive_rank --------------------------------------------

std::optional<ClueInterp> interpret_reactive_rank(const Game& prev, Game& game,
                                                     const ClueAction& action,
                                                     int focus_slot, int reacter) {
  hanabi::instr::ScopedTimer st("reactor.interpret_reactive_rank");
  hanabi::logging::LogScope ls(
      "reactor.interpret_reactive_rank",
      {{"focus_slot", focus_slot}, {"reacter", reacter}});
  const State& state = game.state;
  int receiver = action.target;
  ReactiveContext ctx = reactive_context(prev, game, action, reacter);

  // v0.33: among multiple copies of the same playable identity in
  // the receiver's hand, only the RIGHTMOST copy (highest slot index
  // = oldest) is a "primary" play target. The lefter copies are
  // treated as trash for primary selection and tried only as a
  // right-to-left fallback if no primary react_slot resolves on Bob.
  // CTD'd cards are eligible as play targets *if* they're currently
  // playable — the stack may have caught up to a card we earlier
  // marked for discard. The is_playable check below filters the
  // not-yet-playable CTD'd cards.
  std::vector<std::pair<int, int>> all_playable;
  for (size_t i = 0; i < state.hands[receiver].size(); ++i) {
    int o = state.hands[receiver][i];
    if (contains(ctx.known_plays, o)) continue;
    // A play target never stacks on an already-CTP'd card (replay
    // 1916815): its play is already queued, and the pending-CTP
    // simulation in reactive_context makes its successor hypo-playable
    // — the successor is the eligible target.
    if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) continue;
    auto id = state.deck[o].id();
    if (!id || !ctx.hypo_state.is_playable(*id)) continue;
    all_playable.emplace_back(o, static_cast<int>(i));
  }
  std::vector<std::pair<int, int>> play_targets;
  std::vector<std::pair<int, int>> dupe_targets;
  {
    std::vector<std::pair<int, int>> by_slot_desc = all_playable;
    std::sort(by_slot_desc.begin(), by_slot_desc.end(),
               [](const auto& a, const auto& b) { return a.second > b.second; });
    std::set<int> seen_id;
    for (const auto& p : by_slot_desc) {
      auto id = state.deck[p.first].id();
      int id_ord = id->to_ord();
      if (seen_id.insert(id_ord).second) {
        play_targets.push_back(p);
      } else {
        dupe_targets.push_back(p);
      }
    }
  }
  std::sort(play_targets.begin(), play_targets.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
  play_targets.insert(play_targets.end(), dupe_targets.begin(),
                       dupe_targets.end());

  int hand_size = kHandSize[state.num_players];
  for (const auto& [target, index] : play_targets) {
    int target_slot = index + 1;
    // Older-CTP guard: if the receiver has an already-CTP'd card at a
    // HIGHER slot index (= older) than this play_target, the receiver
    // will play the older CTP first, breaking the chain (the new
    // target won't fire before the reacter acts). Replay 1899623 T7
    // exhibits this: yagami had m2 (slot 5) CTP'd before the rank-3
    // clue; the convention picked yagami's slot 1 (n1) as the chain
    // target, but yagami played m2 at T8 (older CTP), leaving stack
    // n=0 — wb69 then played the supposed n2 connector and bombed at
    // T9. With this guard the play_target is skipped, falling through
    // to the finesse fallback whose POV-invariant guard validates the
    // chain.
    //
    // The guard only applies to targets that are already playable on
    // the REAL stacks. A target that becomes playable only through the
    // receiver's pending CTP plays (i.e. playable in `hypo_state` but
    // not in `state`) is *enabled* by those older CTPs, not blocked:
    // per the convention, the receiver's queued plays are simulated
    // first and the leftmost post-sim playable is the intended target.
    // Replay 1892197 T9: yagami's p3 sits behind the CTP'd p2 — the
    // rank-4 read must map p3 (→ wb67's slot 2 = i2, a misplay the
    // giver's eval then rejects), not skip it for the very queue that
    // enables it.
    auto guard_tid = state.deck[target].id();
    bool target_needs_pending = guard_tid && !state.is_playable(*guard_tid);
    bool older_ctp_blocks = false;
    if (!target_needs_pending) {
      for (size_t i = index + 1; i < state.hands[receiver].size(); ++i) {
        int o = state.hands[receiver][i];
        if (game.meta[o].status == CardStatus::CALLED_TO_PLAY) {
          older_ctp_blocks = true;
          break;
        }
      }
    }
    if (older_ctp_blocks) continue;
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
    if (variants::would_lose_inverted_reacter(state, react_order,
                                       variants::target_is_inverted(state, target),
                                       /*standard_is_target_play=*/true)) {
      continue;
    }
    auto interp = variants::target_is_inverted(state, target)
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
      // v0.37: when `common.thoughts[target].inferred` is already empty
      // — typically because the T21 untouching just removed every
      // surviving id from a previously-narrowed inferred (e.g. info_lock
      // = {r4} stamped at a stable colour-red clue, then a rank-4
      // untouching empties it) — narrowing produces an empty result and
      // the loop would skip this otherwise-valid play_target. Reset to
      // the empathy baseline (possible) before narrowing, matching the
      // "if inferred goes empty, fall back to empathy" principle the
      // rest of the interpreter uses (`Thought::reset_inferences` +
      // `elim` Step 1).
      const Thought& target_thought = game.common.thoughts[target];
      IdentitySet base = target_thought.inferred.non_empty()
                            ? target_thought.inferred
                            : target_thought.possible;
      IdentitySet narrowed = base.filter(
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
      CardStatus target_status = variants::target_is_inverted(state, target)
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
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
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
    if (variants::would_lose_inverted_reacter(state, react_order,
                                       variants::target_is_inverted(state, receive_order),
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
    auto interp = variants::target_is_inverted(state, receive_order)
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
    if (!game.waiting.empty()) game.waiting.front().react_order = react_order;
    return ClueInterp::REACTIVE;
  }

  // Orange chop-save fallback (see variants/inverted.h).
  return variants::orange_chop_save(prev, game, action, focus_slot, reacter,
                                    ctx.possible_conns);
}

}  // namespace hanabi::reactor
