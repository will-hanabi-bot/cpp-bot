#include "hanabi/conventions/reactor/interpret_clue.h"

#include <algorithm>
#include <unordered_set>

#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/fix.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/interp.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/conventions/reactor/interpret_reactive.h"
#include "hanabi/conventions/reactor/reactive_table.h"

namespace hanabi::reactor {

namespace {

// Substring predicates matching the Python's regex variants. Each name set
// here is the regex's alternation expanded as individual substrings.
bool includes_rainbowish(const State& state) {
  return state.includes_variant("Rainbow") || state.includes_variant("Omni");
}
bool includes_pinkish(const State& state) {
  return state.includes_variant("Pink") || state.includes_variant("Omni");
}
bool includes_brownish(const State& state) {
  return state.includes_variant("Brown") || state.includes_variant("Muddy") ||
         state.includes_variant("Cocoa") || state.includes_variant("Null");
}

bool contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

// Pink-promise: in a pinkish variant a rank clue that touches the receiver's
// chop (rightmost unclued card) promises that the chop has that rank. Two
// substitutions apply when the natural rank clue for the special rank is
// unavailable (pink_s makes rank-K clue not touch rank-K cards):
//   pink_s + special_rank=5: rank-4 promises rank-5
//   pink_s + special_rank=1: rank-2 promises rank-1
// Returns true when the giver can see that the chop's rank cannot satisfy
// the promise — i.e., the clue is illegal.
bool violates_pink_promise(const Game& prev, const ClueAction& action) {
  if (action.clue.kind != ClueKind::RANK) return false;

  const State& state = prev.state;
  const Variant& v = *state.variant;

  bool pinkish = v.pink_s;
  for (const Suit& s : v.suits) {
    if (s.suit_type.pinkish) { pinkish = true; break; }
  }
  if (!pinkish) return false;

  // Chop = rightmost (oldest, last vector index) unclued card before the
  // clue lands.
  std::optional<int> chop;
  for (auto it = state.hands[action.target].rbegin();
        it != state.hands[action.target].rend(); ++it) {
    if (!state.deck[*it].clued) { chop = *it; break; }
  }
  if (!chop) return false;

  // Promise only applies when the chop itself is touched.
  if (!contains(action.list_, *chop)) return false;

  // Promised ranks (set, to handle pink_s substitutions where a clue can
  // legitimately call either the spoken rank or the special rank).
  std::vector<int> promised{action.clue.value};
  if (v.pink_s && v.special_rank) {
    int sr = *v.special_rank;
    if (sr == 5 && action.clue.value == 4) promised.push_back(5);
    else if (sr == 1 && action.clue.value == 2) promised.push_back(1);
  }

  auto chop_id = state.deck[*chop].id();
  if (!chop_id) return false;  // observer's own hand — can't verify
  return std::find(promised.begin(), promised.end(), chop_id->rank) ==
         promised.end();
}

}  // namespace

// --- _reactive_focus -----------------------------------------------------

int reactive_focus(const State& state, int receiver, const ClueAction& action) {
  const auto& list_ = action.list_;
  const auto& clue = action.clue;
  const auto& hand = state.hands[receiver];

  // Touched cards with their slot index (0-based). Skip if none.
  std::vector<std::pair<int, int>> touched_with_index;
  for (size_t i = 0; i < hand.size(); ++i) {
    if (contains(list_, hand[i])) {
      touched_with_index.emplace_back(hand[i], static_cast<int>(i));
    }
  }
  if (touched_with_index.empty()) return 1;

  // max by (order, demoting newest slot to -1).
  auto cmp = [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
    int ka = (a.first == hand[0]) ? -1 : a.first;
    int kb = (b.first == hand[0]) ? -1 : b.first;
    return ka < kb;
  };
  auto focus_it = std::max_element(touched_with_index.begin(),
                                     touched_with_index.end(), cmp);
  int focus_i = focus_it->second;

  if (clue.kind == ClueKind::COLOUR) {
    if (includes_rainbowish(state) || state.variant->rainbow_s) {
      auto table = reactive_value_table(*state.variant, kHandSize[state.num_players]);
      return table[clue.value];
    }
    return focus_i + 1;
  }
  // Rank
  if (includes_pinkish(state) || state.variant->pink_s) {
    return clue.value;
  }
  return focus_i + 1;
}

// --- delayed_plays -------------------------------------------------------

std::vector<std::pair<int, Identity>> delayed_plays(const Game& game, int giver,
                                                       int receiver, bool stable) {
  const Player& common = game.common;
  const State& state = game.state;
  const auto& meta = game.meta;

  std::vector<std::pair<int, Identity>> result;
  for (int pi : players_until(state.num_players, state.next_player_index(giver),
                                  receiver)) {
    std::optional<int> urgent_order;
    for (int o : state.hands[pi]) {
      if (meta[o].urgent) {
        urgent_order = o;
        break;
      }
    }
    std::vector<int> playables;
    if (urgent_order) {
      if (meta[*urgent_order].status != CardStatus::CALLED_TO_DISCARD) {
        playables.push_back(*urgent_order);
      }
    } else {
      auto obvious = common.obvious_playables(game, pi);
      if (!stable && obvious.size() > 1) {
        // skip
      } else {
        playables = obvious;
      }
    }

    for (int o : playables) {
      bool dominated = false;
      for (int p : playables) {
        if (p > o && common.thoughts[p].possible == common.thoughts[o].possible) {
          dominated = true;
          break;
        }
      }
      if (dominated) continue;

      auto id = common.thoughts[o].id(/*infer=*/true);
      if (id) {
        if (auto next = id->next()) result.insert(result.begin(), {o, *next});
      } else {
        IdentitySet non_trash = common.thoughts[o].inferred.difference(state.trash_set);
        for (Identity i : non_trash) {
          if (auto next = i.next()) result.insert(result.begin(), {o, *next});
        }
      }
    }
  }
  return result;
}

// --- target_play ---------------------------------------------------------

std::optional<ClueInterp> target_play(Game& game, const ClueAction& action,
                                         int target, bool urgent, bool stable) {
  const State& state = game.state;
  int holder = state.holder_of(target);
  auto possible_conns = delayed_plays(game, action.giver, holder, stable);

  IdentitySet ps = state.playable_set;
  IdentitySet new_inferred = game.common.thoughts[target].inferred.filter(
      [&](Identity i) {
        if (ps.contains(i)) return true;
        for (const auto& [_, c] : possible_conns) {
          if (c == i) return true;
        }
        return false;
      });

  // If we have the actual id and a connector matches, mark connector urgent.
  auto target_id = state.deck[target].id();
  if (target_id) {
    for (const auto& [conn_order, conn_id] : possible_conns) {
      if (conn_id == *target_id) {
        auto prev_id = target_id->prev();
        if (!prev_id) continue;
        Identity pid = *prev_id;
        game.with_thought(conn_order, [&](const Thought& t) {
          Thought out = t;
          out.old_inferred = t.inferred;
          out.inferred = IdentitySet::single(pid);
          return out;
        });
        int turn = state.turn_count;
        int giver = action.giver;
        game.with_meta(conn_order, [turn, giver](ConvData& m) {
          m.urgent = true;
          m.status = CardStatus::CALLED_TO_PLAY;
          m.by = giver;
          m = m.reason(turn);
        });
        break;
      }
    }
  }

  game.with_thought(target, [&](const Thought& t) {
    Thought out = t;
    out.old_inferred = t.inferred;
    out.inferred = new_inferred;
    out.info_lock = new_inferred.non_empty()
                        ? std::optional<IdentitySet>{new_inferred}
                        : std::nullopt;
    return out;
  });
  int turn = state.turn_count;
  game.with_meta(target, [turn](ConvData& m) {
    m = m.reason(turn).signal(turn);
  });

  if (new_inferred.is_empty() ||
      !state.has_consistent_infs(game.common.thoughts[target])) {
    game.with_thought(target, [](const Thought& t) { return t.reset_inferences(); });
    if (stable && game.common.order_kt(game, target)) return ClueInterp::STALL;
    return std::nullopt;
  }

  int giver = action.giver;
  game.with_meta(target, [giver, urgent](ConvData& m) {
    m.status = CardStatus::CALLED_TO_PLAY;
    m.by = giver;
    m.focused = true;
    m.urgent = urgent;
  });
  return ClueInterp::PLAY;
}

// --- target_discard ------------------------------------------------------

std::optional<ClueInterp> target_discard(Game& game, const ClueAction& action,
                                            int target, bool urgent) {
  const State& state = game.state;
  game.with_thought(target, [&](const Thought& t) {
    Thought out = t;
    out.inferred = t.inferred.filter(
        [&](Identity i) { return !state.is_critical(i); });
    return out;
  });
  int turn = state.turn_count;
  int giver = action.giver;
  game.with_meta(target, [turn, giver, urgent](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.by = giver;
    m.urgent = urgent;
    m = m.reason(turn).signal(turn);
  });

  if (game.common.thoughts[target].inferred.is_empty()) {
    game.with_thought(target, [](const Thought& t) { return t.reset_inferences(); });
    return std::nullopt;
  }
  return ClueInterp::DISCARD;
}

// --- ref_play -----------------------------------------------------------

std::optional<ClueInterp> ref_play(const Game& prev, Game& game,
                                      const ClueAction& action) {
  const auto& hand = game.state.hands[action.target];
  std::vector<int> newly_touched;
  for (int o : action.list_) {
    if (!prev.state.deck[o].clued) newly_touched.push_back(o);
  }
  if (newly_touched.empty()) return std::nullopt;

  std::vector<int> target_candidates;
  for (int o : newly_touched) {
    target_candidates.push_back(game.common.refer(prev, hand, o, /*left=*/true));
  }
  int target = *std::max_element(target_candidates.begin(), target_candidates.end());

  if (game.is_blind_playing(target)) return std::nullopt;
  if (game.meta[target].status == CardStatus::CALLED_TO_DISCARD) return std::nullopt;
  return target_play(game, action, target, /*urgent=*/false, /*stable=*/true);
}

// --- ref_discard --------------------------------------------------------

std::optional<ClueInterp> ref_discard(const Game& prev, Game& game,
                                         const ClueAction& action, bool stall) {
  const State& state = game.state;
  int giver = action.giver;
  int receiver = action.target;
  const auto& list_ = action.list_;
  const auto& clue = action.clue;
  const auto& hand = state.hands[receiver];

  std::vector<int> newly_touched;
  for (int o : list_) {
    if (!prev.state.deck[o].clued) newly_touched.push_back(o);
  }
  std::vector<int> unclued_orders;
  for (int o : hand) {
    if (!prev.state.deck[o].clued) unclued_orders.push_back(o);
  }
  std::optional<int> lock_order;
  if (!unclued_orders.empty()) {
    lock_order = *std::min_element(unclued_orders.begin(), unclued_orders.end());
  }

  if (lock_order && contains(list_, *lock_order)) {
    if (stall && state.next_player_index(receiver) == giver) {
      return ClueInterp::STALL;
    }
    if (prev.common.thinks_locked(prev, receiver)) return ClueInterp::MISTAKE;
    // Lock interpretation.
    int lo = *lock_order;
    if (includes_pinkish(state)) {
      int rv = clue.value;
      game.with_thought(lo, [rv](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.filter([rv](Identity i) { return i.rank == rv; });
        return out;
      });
      game.with_meta(lo, [](ConvData& m) { m.focused = true; });
      auto lock_id = state.deck[lo].id();
      if (lock_id && lock_id->rank != clue.value) return std::nullopt;
    }
    int turn = state.turn_count;
    int g = giver;
    for (int o : hand) {
      game.with_meta(o, [turn, g](ConvData& m) {
        m.status = CardStatus::CHOP_MOVED;
        m.by = g;
        m = m.reason(turn);
      });
    }
    return ClueInterp::LOCK;
  }

  int focus = *std::max_element(newly_touched.begin(), newly_touched.end());
  int focus_pos = static_cast<int>(
      std::find(hand.begin(), hand.end(), focus) - hand.begin());
  int target_index = -1;
  for (int i = focus_pos + 1; i < static_cast<int>(hand.size()); ++i) {
    if (!state.deck[hand[i]].clued) {
      target_index = i;
      break;
    }
  }
  if (target_index == -1) return std::nullopt;

  std::vector<int> promised_orders;
  for (int o : list_) {
    if (o > hand[target_index]) promised_orders.push_back(o);
  }
  int promised_order = promised_orders.empty()
                            ? focus
                            : *std::min_element(promised_orders.begin(),
                                                  promised_orders.end());

  if (includes_pinkish(state)) {
    int rv = clue.value;
    game.with_thought(promised_order, [rv](const Thought& t) {
      Thought out = t;
      out.inferred = t.inferred.filter([rv](Identity i) { return i.rank == rv; });
      return out;
    });
    game.with_meta(promised_order, [](ConvData& m) { m.focused = true; });
    auto p_id = state.deck[promised_order].id();
    if (p_id && p_id->rank != clue.value) return std::nullopt;
  } else {
    game.with_meta(focus, [](ConvData& m) { m.focused = true; });
  }

  int target_order = hand[target_index];
  int turn = state.turn_count;
  int g = giver;
  game.with_meta(target_order, [turn, g](ConvData& m) {
    m.status = CardStatus::CALLED_TO_DISCARD;
    m.by = g;
    m = m.reason(turn).signal(turn);
  });
  return ClueInterp::DISCARD;
}

// --- try_stable ---------------------------------------------------------

std::optional<ClueInterp> try_stable(const Game& prev, Game& game,
                                        const ClueAction& action, bool stall) {
  const State& state = game.state;
  int giver = action.giver;
  int target = action.target;
  const auto& list_ = action.list_;
  const auto& clue = action.clue;
  int next_player_index = state.next_player_index(giver);

  std::vector<int> newly_touched;
  for (int o : list_) {
    if (!prev.state.deck[o].clued) newly_touched.push_back(o);
  }

  // Pink-promise gate: in pinkish variants a rank clue that newly touches
  // the chop "promises" the chop's rank. If we can see the chop and it
  // doesn't match, the clue is illegal — short-circuit to MISTAKE before
  // running any of the trash_push / playable_rank / ref_play / ref_discard
  // branches (each of which would otherwise stamp a partial interpretation).
  if (!newly_touched.empty() && violates_pink_promise(prev, action)) {
    return std::nullopt;
  }

  if (clue.kind == ClueKind::RANK && !newly_touched.empty()) {
    bool trash_push = true;
    bool playable_rank = true;
    // Iterate every identity the clue can actually touch (rank == clue.value,
    // plus pinkish/omni/brownish/special-rank ids per the variant). Looking
    // only at Identity(s, clue.value) would miss e.g. rank-5 cards under
    // Pink-Fives and falsely conclude trash_push.
    for (Identity id : state.variant->touch_possibilities(clue.kind, clue.value)) {
      bool basic = state.is_basic_trash(id);
      if (!basic) trash_push = false;
      if (!basic && !state.is_playable(id)) playable_rank = false;
    }

    if (trash_push) {
      int focus = *std::max_element(newly_touched.begin(), newly_touched.end());
      IdentitySet ts = state.trash_set;
      game.with_thought(focus, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.intersect(ts);
        return out;
      });
      game.with_meta(focus, [](ConvData& m) { m.trash = true; });
    } else if (playable_rank) {
      int focus;
      if (includes_pinkish(state)) {
        std::vector<int> touched_unclued;
        for (int o : state.hands[target]) {
          if (!prev.state.deck[o].clued && contains(list_, o)) {
            touched_unclued.push_back(o);
          }
        }
        focus = !touched_unclued.empty()
                    ? *std::min_element(touched_unclued.begin(), touched_unclued.end())
                    : *std::max_element(newly_touched.begin(), newly_touched.end());
      } else {
        focus = *std::max_element(newly_touched.begin(), newly_touched.end());
      }

      bool unnecessary_focus = game.common.thoughts[focus].possible.forall(
          [&](Identity i) {
            if (state.is_basic_trash(i)) return true;
            for (const auto& hand : state.hands) {
              for (int o : hand) {
                if (game.common.thoughts[o].matches(i)) return true;
              }
            }
            return false;
          });

      if (!unnecessary_focus) {
        // `inferred` has already been intersected with the clue's touch set
        // by on_clue, so checking is_playable alone covers e.g. (R,5) under
        // Pink-Fives where the touched playable doesn't equal clue.value.
        IdentitySet new_inferred = game.common.thoughts[focus].inferred.filter(
            [&](Identity i) { return state.is_playable(i); });
        game.with_thought(focus, [&](const Thought& t) {
          Thought out = t;
          out.inferred = new_inferred;
          out.info_lock = new_inferred.non_empty()
                              ? std::optional<IdentitySet>{new_inferred}
                              : std::nullopt;
          return out;
        });
        game.with_meta(focus, [](ConvData& m) {
          m.focused = true;
          m.status = CardStatus::CALLED_TO_PLAY;
        });
      }
    }
  }

  // Set up waiting connection (for response-inversion).
  if (game.waiting.empty() && next_player_index != target) {
    int focus_slot = reactive_focus(state, target, action);
    ReactorWC wc{giver, next_player_index, target, state.hands[target],
                  to_clue(clue, target), focus_slot, /*inverted=*/true, state.turn_count};
    game.waiting.push_back(std::move(wc));
  }

  // Check fix
  FixResult fix_result = check_fix(prev, game, action);
  if (std::holds_alternative<FixResultNormal>(fix_result)) {
    return ClueInterp::FIX;
  }

  // Compute prev_playables and playables.
  auto unique_concat = [](std::vector<int> a, const std::vector<int>& b) {
    std::unordered_set<int> seen(a.begin(), a.end());
    for (int x : b) {
      if (!seen.count(x)) {
        a.push_back(x);
        seen.insert(x);
      }
    }
    return a;
  };
  auto prev_playables = unique_concat(
      prev.common.obvious_playables(prev, target),
      connectable_simple(prev, prev.players[giver], next_player_index, target));

  // For the "playables" check, the Python builds a hypothetical game with
  // with_move(PLAY) — but with_move modifies move_history. The simpler
  // approach in C++ is to snapshot+restore game's move_history.
  std::vector<Interp> saved_history = game.move_history;
  game.move_history.push_back(ClueInterp::PLAY);
  auto playables = unique_concat(
      game.common.obvious_playables(game, target),
      connectable_simple(game, game.players[giver], next_player_index, target));
  game.move_history = std::move(saved_history);

  auto find_reveal = [&]() -> std::optional<int> {
    for (int o : playables) {
      if (contains(list_, o) && !contains(prev_playables, o) &&
          (clue.kind == ClueKind::RANK || prev.state.deck[o].clued)) {
        return o;
      }
    }
    return std::nullopt;
  };

  if (newly_touched.empty()) {
    // Fill-in / hard-burn paths.
    auto safe_actions = unique_concat(playables, game.common.thinks_trash(game, target));
    auto old_safe = unique_concat(prev_playables, prev.common.thinks_trash(prev, target));
    std::vector<int> new_safe;
    for (int o : safe_actions) {
      if (!contains(old_safe, o)) new_safe.push_back(o);
    }
    if (!new_safe.empty()) return ClueInterp::REVEAL;

    if (stall) return ClueInterp::STALL;

    game.move_history.push_back(ClueInterp::REVEAL);
    auto connectable = connectable_simple(game, game.common,
                                              state.next_player_index(giver), target);
    game.move_history = saved_history;
    std::vector<int> connectable_filtered;
    for (int o : connectable) {
      if (!contains(old_safe, o)) connectable_filtered.push_back(o);
    }
    if (!connectable_filtered.empty()) return ClueInterp::REVEAL;

    // Try connecting through unknown playable
    if (!list_.empty()) {
      int max_o = *std::max_element(list_.begin(), list_.end());
      auto focus_id = state.deck[max_o].id();
      if (focus_id && next_player_index != target && state.playable_away(*focus_id) == 1) {
        for (int o : prev.common.obvious_playables(prev, next_player_index)) {
          auto prev_id = focus_id->prev();
          if (prev_id && game.common.thoughts[o].inferred.contains(*prev_id)) {
            Identity pid = *prev_id;
            game.with_thought(o, [pid](const Thought& t) {
              Thought out = t;
              out.inferred = IdentitySet::single(pid);
              return out;
            });
            return ClueInterp::REVEAL;
          }
        }
      }
    }
    return std::nullopt;
  }

  auto revealed = find_reveal();
  if (revealed) return ClueInterp::REVEAL;

  int max_nt = *std::max_element(newly_touched.begin(), newly_touched.end());
  if (game.common.order_kt(game, max_nt)) {
    bool brownish_tcm = false;
    if (includes_brownish(state) && clue.kind == ClueKind::RANK &&
        !prev.common.obvious_loaded(game, target)) {
      bool no_newest_in_touched =
          !state.hands[target].empty() &&
          !contains(newly_touched, state.hands[target][0]);
      if (no_newest_in_touched) {
        for (size_t i = 0; i < state.variant->suits.size(); ++i) {
          const auto& s = state.variant->suits[i];
          bool brown = s.suit_type.brownish;
          if (brown && state.play_stacks[i] + 1 < state.max_ranks[i]) {
            brownish_tcm = true;
            break;
          }
        }
      }
    }
    if (brownish_tcm) return ClueInterp::REVEAL;
    return ref_play(prev, game, action);
  }
  if (clue.kind == ClueKind::COLOUR) return ref_play(prev, game, action);
  return ref_discard(prev, game, action, stall);
}

// --- _alternative_clue --------------------------------------------------

namespace {

std::optional<Clue> alternative_clue(const Game& game, int clue_target, bool play_only) {
  if (game.no_recurse) return std::nullopt;
  const Player& common = game.common;
  const State& state = game.state;
  for (const Clue& clue : state.all_valid_clues(clue_target)) {
    auto list_ = state.clue_touched(state.hands[clue_target], clue.kind, clue.value);
    const auto& hand = state.hands[clue_target];
    std::vector<int> newly_touched;
    for (int o : list_) {
      if (!state.deck[o].clued) newly_touched.push_back(o);
    }
    if (newly_touched.empty()) continue;
    if (clue.kind == ClueKind::COLOUR) {
      std::vector<int> refs;
      for (int o : newly_touched) refs.push_back(common.refer(game, hand, o, true));
      int play_target = *std::max_element(refs.begin(), refs.end());
      auto play_id = state.deck[play_target].id();
      if (!play_id || !state.is_playable(*play_id)) continue;

      auto all_useful = [&]() {
        for (int o : newly_touched) {
          auto id = state.deck[o].id();
          if (!id || !state.is_useful(*id)) return false;
        }
        return true;
      };
      auto all_basic_trash_poss = [&]() {
        ClueKind k = clue.kind;
        int v = clue.value;
        IdentitySet poss;
        for (Identity i : state.variant->all_ids()) {
          if (state.variant->id_touched(i, k, v)) poss = poss.add(i);
        }
        for (int o : newly_touched) {
          if (!common.thoughts[o].possible.intersect(poss).forall(
                  [&](Identity i) { return state.is_basic_trash(i); })) {
            return false;
          }
        }
        return true;
      };
      if (all_useful() || all_basic_trash_poss()) return clue;
    } else {
      if (play_only) continue;
      std::vector<int> unclued;
      for (int o : hand) {
        if (!state.deck[o].clued) unclued.push_back(o);
      }
      if (unclued.empty()) continue;
      int min_unclued = *std::min_element(unclued.begin(), unclued.end());
      if (contains(list_, min_unclued)) continue;
      int focus = *std::max_element(newly_touched.begin(), newly_touched.end());
      int focus_pos = static_cast<int>(
          std::find(hand.begin(), hand.end(), focus) - hand.begin());
      int target_index = -1;
      for (int i = focus_pos + 1; i < static_cast<int>(hand.size()); ++i) {
        if (!state.deck[hand[i]].clued) {
          target_index = i;
          break;
        }
      }
      if (target_index == -1) continue;
      auto id = state.deck[hand[target_index]].id();
      if (!id || !state.is_basic_trash(*id)) continue;
      bool all_useful_p = true;
      for (int o : newly_touched) {
        auto oid = state.deck[o].id();
        if (!oid || !state.is_useful(*oid)) {
          all_useful_p = false;
          break;
        }
      }
      if (!all_useful_p) continue;
      return clue;
    }
  }
  return std::nullopt;
}

bool bad_stable(const Game& prev, const Game& game, const ClueAction& action,
                 ClueInterp interp, bool stall) {
  const State& state = game.state;
  int target = action.target;
  if (interp == ClueInterp::MISTAKE) return true;
  if (prev.state.turn_count == 1 && action.clue.kind == ClueKind::RANK &&
      alternative_clue(prev, target, /*play_only=*/true)) {
    return true;
  }
  if (target == state.our_player_index) return false;

  std::optional<int> bad_playable;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      bool now_inconsistent =
          game.meta[o].status == CardStatus::CALLED_TO_PLAY &&
          !state.has_consistent_infs(game.common.thoughts[o]) &&
          (prev.meta[o].status != CardStatus::CALLED_TO_PLAY ||
            prev.state.has_consistent_infs(prev.common.thoughts[o]));
      if (now_inconsistent) {
        bad_playable = o;
        break;
      }
    }
    if (bad_playable) break;
  }
  if (bad_playable) return true;

  std::optional<int> bad_discard;
  for (int o : state.hands[target]) {
    if (game.meta[o].status != CardStatus::CALLED_TO_DISCARD) continue;
    if (prev.meta[o].status == CardStatus::CALLED_TO_DISCARD) continue;
    auto oid = state.deck[o].id();
    if (!oid) continue;
    if (state.is_critical(*oid)) {
      bad_discard = o;
      break;
    }
    if (stall && state.is_useful(*oid) && alternative_clue(game, target, false)) {
      bad_discard = o;
      break;
    }
  }
  if (bad_discard) return true;

  if (interp == ClueInterp::LOCK && alternative_clue(game, target, false)) return true;
  if (!stall) return false;
  return interp == ClueInterp::STALL && alternative_clue(game, target, false).has_value();
}

}  // namespace

// --- interpret_stable ----------------------------------------------------

std::optional<ClueInterp> interpret_stable(const Game& prev, Game& game,
                                              const ClueAction& action, bool stall) {
  const State& state = game.state;
  int target = action.target;
  int bob = state.next_player_index(action.giver);

  // Snapshot game to retry with reactive if stable looks bad.
  Game saved = game;
  auto interp = try_stable(prev, game, action, stall);

  if (target != bob) {
    ClueInterp actual = interp.value_or(ClueInterp::MISTAKE);
    if (bad_stable(prev, game, action, actual, stall)) {
      // Restore game and try reactive instead. The Python builds a hypothetical
      // game by appending the action and calling on_clue + elim - we do
      // the equivalent: restore + simulate the clue + elim.
      game = saved;
      Game hypo = prev;
      hypo.on_clue(action);
      hypo.elim();
      game = std::move(hypo);
      return interpret_reactive(prev, game, action, bob, /*looks_stable=*/true);
    }
  }
  return interp;
}

// --- interpret_reactive (top-level) -------------------------------------

std::optional<ClueInterp> interpret_reactive(const Game& prev, Game& game,
                                                const ClueAction& action,
                                                int reacter, bool looks_stable) {
  const State& state = game.state;
  int giver = action.giver;
  int receiver = action.target;
  const auto& clue = action.clue;

  int focus_slot = reactive_focus(state, receiver, action);
  ReactorWC wc{giver, reacter, receiver, state.hands[receiver],
                to_clue(clue, receiver), focus_slot, /*inverted=*/false,
                state.turn_count, /*all_plays=*/game.all_plays};
  game.waiting.clear();
  game.waiting.push_back(std::move(wc));

  if (receiver == state.our_player_index) return ClueInterp::REACTIVE;
  if (clue.kind == ClueKind::COLOUR && !game.all_plays) {
    return interpret_reactive_colour(prev, game, action, focus_slot, reacter,
                                        looks_stable);
  }
  return interpret_reactive_rank(prev, game, action, focus_slot, reacter);
}

}  // namespace hanabi::reactor
