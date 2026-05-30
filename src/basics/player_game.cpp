// Player methods that require Game's full type. Implementing them here keeps
// game.h independent of player.h's full body and matches the Python split
// (the .py file declares both groups; we use file boundaries for include hygiene).
#include "hanabi/basics/game.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"

#include <algorithm>
#include <unordered_set>

namespace hanabi {
namespace {

bool any_contains(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

// --- refer ----------------------------------------------------------------

int Player::refer(const Game& game, const std::vector<int>& hand, int order,
                    bool left) const {
  const int offset = left ? -1 : 1;
  const int n = static_cast<int>(hand.size());
  auto it = std::find(hand.begin(), hand.end(), order);
  const int index = static_cast<int>(it - hand.begin());
  int target = ((index + offset) % n + n) % n;
  while (game.is_touched(hand[target]) && target != index) {
    target = ((target + offset) % n + n) % n;
  }
  return hand[target];
}

// --- chop_newest ----------------------------------------------------------

std::optional<int> Player::chop_newest(const Game& game, int player_index) const {
  for (int o : game.state.hands[player_index]) {
    if (!game.state.deck[o].clued && game.meta[o].status == CardStatus::NONE) {
      return o;
    }
  }
  return std::nullopt;
}

// --- is_duped / is_trash --------------------------------------------------

bool Player::is_duped(const Game& game, Identity id, int exclude_order) const {
  std::vector<int> candidates;
  if (game.good_touch) {
    for (const auto& hand : game.state.hands) {
      for (int o : hand) {
        if (game.is_touched(o)) candidates.push_back(o);
      }
    }
  } else {
    candidates = game.state.hands[game.state.holder_of(exclude_order)];
  }

  for (int o : candidates) {
    if (o == exclude_order) continue;
    if (!thoughts[o].matches(id, /*infer=*/true)) continue;
    if (!game.state.deck[o].matches(id, /*assume=*/true)) continue;

    bool shared = false;
    for (const Link& link : links) {
      bool matched_via_link = std::visit(
          [&](const auto& l) -> bool {
            using T = std::decay_t<decltype(l)>;
            if constexpr (std::is_same_v<T, UnpromisedLink>) {
              return any_contains(l.orders, exclude_order) &&
                     any_contains(l.orders, o) && l.ids.contains(id);
            } else {
              return any_contains(l.orders, exclude_order) &&
                     any_contains(l.orders, o) && l.id == id;
            }
          },
          link);
      if (matched_via_link) {
        shared = true;
        break;
      }
    }
    if (!shared) return true;
  }
  return false;
}

bool Player::is_trash(const Game& game, Identity id, int exclude_order) const {
  return game.state.is_basic_trash(id) || is_duped(game, id, exclude_order);
}

// --- order_kt / order_trash / order_kp / order_playable -------------------

bool Player::order_kt(const Game& game, int order) const {
  const Thought& thought = thoughts[order];
  const int holder = game.state.holder_of(order);

  auto same_hand_dupe = [&](Identity i) {
    if (game.state.is_basic_trash(i)) return true;
    for (int o : game.state.hands[holder]) {
      if (o != order && thoughts[o].matches(i)) return true;
    }
    return false;
  };

  if (game.meta[order].trash &&
      thought.possible.forall([&](Identity i) { return !game.state.is_critical(i); })) {
    return true;
  }
  return thought.possible.forall(same_hand_dupe);
}

bool Player::order_trash(const Game& game, int order) const {
  const ConvData& meta_ = game.meta[order];
  const Thought& thought = thoughts[order];

  if (order_kt(game, order)) return true;
  if (thought.possible.forall([&](Identity i) { return game.state.is_critical(i); })) {
    return false;
  }

  auto is_trash_cb = [&](Identity i) { return is_trash(game, i, order); };
  bool conventional_trash =
      thought.possible.forall(is_trash_cb) ||
      (thought.info_lock && thought.info_lock->forall(is_trash_cb)) ||
      meta_.trash || meta_.status == CardStatus::CALLED_TO_DISCARD ||
      meta_.status == CardStatus::PERMISSION_TO_DISCARD;
  if (conventional_trash) return true;
  return thought.possibilities().forall(is_trash_cb);
}

bool Player::order_kp(const Game& game, int order, bool exclude_trash) const {
  const State& state = game.state;
  const Thought& thought = thoughts[order];

  if (thought.possible.forall([&](Identity i) { return !state.is_playable(i); })) {
    return false;
  }

  auto poss_playable = [&](IdentitySet poss) {
    IdentitySet p = exclude_trash ? poss.difference(state.trash_set) : poss;
    return p.non_empty() && p.intersect(state.playable_set) == p;
  };

  CardStatus status = game.meta[order].status;
  if (status == CardStatus::CALLED_TO_PLAY) {
    bool has_playable = thought.possible.intersect(state.playable_set).non_empty();
    bool lock_ok = !thought.info_lock ||
                   thought.info_lock->intersect(state.playable_set).non_empty();
    return has_playable && lock_ok;
  }
  if (status == CardStatus::SARCASTIC || status == CardStatus::GENTLEMANS_DISCARD) {
    return poss_playable(thought.inferred);
  }
  return poss_playable(thought.possible) ||
         (thought.info_lock && poss_playable(*thought.info_lock));
}

bool Player::order_playable(const Game& game, int order, bool exclude_trash) const {
  const State& state = game.state;
  if (order_kp(game, order, exclude_trash)) return true;
  if (game.meta[order].trash) return false;
  const bool infer = game.meta[order].status != CardStatus::CALLED_TO_DISCARD;
  const Thought& thought = thoughts[order];
  IdentitySet poss = infer ? thought.possibilities() : thought.possible;
  IdentitySet p = exclude_trash ? poss.difference(state.trash_set) : poss;
  return p.non_empty() && p.intersect(state.playable_set) == p;
}

// --- obvious_* / thinks_* / locked / loaded / sieved / discardable --------

std::vector<int> Player::obvious_playables(const Game& game, int player_index) const {
  std::vector<int> candidates;
  for (int o : game.state.hands[player_index]) {
    if (order_kp(game, o)) candidates.push_back(o);
  }
  return game.filter_playables(*this, player_index, candidates);
}

std::vector<int> Player::thinks_playables(const Game& game, int player_index,
                                             bool exclude_trash, bool assume) const {
  std::vector<int> candidates;
  for (int o : game.state.hands[player_index]) {
    bool et = exclude_trash && game.is_touched(o);
    if (order_playable(game, o, et)) candidates.push_back(o);
  }
  std::vector<int> filtered;
  for (int p1 : candidates) {
    if (!thoughts[p1].id()) {
      bool duplicated = false;
      for (int p2 : candidates) {
        if (p1 == p2) continue;
        auto p2_id = thoughts[p2].id();
        if (p2_id && thoughts[p1].matches(*p2_id, /*infer=*/true)) {
          duplicated = true;
          break;
        }
      }
      if (duplicated) continue;
    }
    filtered.push_back(p1);
  }
  return game.filter_playables(*this, player_index, filtered, assume);
}

std::vector<int> Player::thinks_trash(const Game& game, int player_index) const {
  std::vector<int> out;
  for (int o : game.state.hands[player_index]) {
    if (order_trash(game, o)) out.push_back(o);
  }
  return out;
}

bool Player::thinks_loaded(const Game& game, int player_index) const {
  return !thinks_playables(game, player_index).empty() ||
         !thinks_trash(game, player_index).empty();
}

bool Player::thinks_locked(const Game& game, int player_index) const {
  if (thinks_loaded(game, player_index)) return false;
  for (int order : game.state.hands[player_index]) {
    CardStatus status = game.meta[order].status;
    if (game.state.deck[order].clued) continue;
    if (status == CardStatus::NONE) return false;
    if (status == CardStatus::CALLED_TO_DISCARD) return false;
    if (status == CardStatus::FINESSED && game.meta[order].hidden) return false;
  }
  return true;
}

bool Player::obvious_loaded(const Game& game, int player_index) const {
  return !obvious_playables(game, player_index).empty() ||
         !thinks_trash(game, player_index).empty();
}

bool Player::obvious_locked(const Game& game, int player_index) const {
  if (obvious_loaded(game, player_index)) return false;
  for (int order : game.state.hands[player_index]) {
    CardStatus status = game.meta[order].status;
    if (game.state.deck[order].clued) continue;
    if (status == CardStatus::NONE || status == CardStatus::CALLED_TO_DISCARD) return false;
  }
  return true;
}

bool Player::is_sieved(const Game& game, Identity id, int exclude_order) const {
  for (int player_index = 0; player_index < game.state.num_players; ++player_index) {
    bool loaded = thinks_loaded(game, player_index);
    auto chop = chop_newest(game, player_index);
    for (int o : game.state.hands[player_index]) {
      if (o == exclude_order) continue;
      if (!thoughts[o].matches(id, /*infer=*/true)) continue;
      if (loaded) {
        if (game.meta[o].status != CardStatus::CALLED_TO_DISCARD) return true;
      } else if (!chop || *chop != o) {
        return true;
      }
    }
  }
  for (const Link& link : links) {
    bool matched = std::visit(
        [&](const auto& l) -> bool {
          using T = std::decay_t<decltype(l)>;
          if constexpr (std::is_same_v<T, UnpromisedLink>) {
            return !any_contains(l.orders, exclude_order) && l.ids.contains(id);
          } else {
            return !any_contains(l.orders, exclude_order) && l.id == id;
          }
        },
        link);
    if (matched) return true;
  }
  return false;
}

std::vector<int> Player::discardable(const Game& game, int player_index,
                                        bool allow_locked_sacrifice) const {
  std::vector<int> out;
  for (int order : game.state.hands[player_index]) {
    if (order_trash(game, order)) {
      out.push_back(order);
      continue;
    }
    if (thoughts[order].possibilities().forall(
            [&](Identity i) { return is_sieved(game, i, order); })) {
      out.push_back(order);
      continue;
    }
    if (allow_locked_sacrifice && game.common.thinks_locked(game, player_index) &&
        game.state.deck[order].clued &&
        thoughts[order].possibilities().intersect(game.state.critical_set).is_empty()) {
      out.push_back(order);
    }
  }
  return out;
}

// --- valid_prompt / find_prompt / find_clued ------------------------------

bool Player::valid_prompt(const Game& prev, int order, Identity id,
                            const std::unordered_set<int>& connected,
                            bool force_pink) const {
  const State& state = prev.state;
  const Card& card = state.deck[order];
  const Thought& thought = thoughts[order];

  if (connected.count(order)) return false;
  if (!card.clued) return false;
  if (!thought.possible.contains(id)) return false;
  if (thought.info_lock && !thought.info_lock->contains(id)) return false;
  if (thought.inferred.length() == 1 && !thought.inferred.contains(id)) return false;

  bool touched_by_clue = false;
  for (const CardClue& c : card.clues) {
    if (state.variant->id_touched(id, c.kind, c.value)) {
      touched_by_clue = true;
      break;
    }
  }
  if (!touched_by_clue) return false;

  // Pink-prompt exception.
  if (state.variant->suits[id.suit_index].suit_type.pinkish && !force_pink) {
    if (!card.clues.empty()) {
      const CardClue& head = card.clues.front();
      bool same_clue = true;
      for (const auto& c : card.clues) {
        if (c.kind != head.kind || c.value != head.value) {
          same_clue = false;
          break;
        }
      }
      const bool misranked = same_clue && head.kind == ClueKind::RANK && head.value != id.rank;
      if (misranked || !prev.known_as(order, "Pink")) return false;
      // Note: PINKISH in Python is the broader regex "Pink|Omni". The Python uses
      // it as the "needle" for known_as; we mirror that by matching just "Pink"
      // and "Omni" separately. Since substring "Pink" wouldn't match "Omni",
      // also try "Omni" as a fallback:
      if (misranked || (!prev.known_as(order, "Pink") && !prev.known_as(order, "Omni"))) {
        return false;
      }
    }
  }
  return true;
}

std::optional<int> Player::find_prompt(const Game& prev, int player_index, Identity id,
                                          const std::unordered_set<int>& connected,
                                          const std::unordered_set<int>& ignore,
                                          bool force_pink, bool rightmost) const {
  const State& state = prev.state;
  std::vector<int> hand = state.hands[player_index];
  if (rightmost) std::reverse(hand.begin(), hand.end());

  std::vector<int> valid;
  for (int o : hand) {
    if (valid_prompt(prev, o, id, connected, force_pink)) valid.push_back(o);
  }
  if (valid.empty()) return std::nullopt;

  auto positive_clue_count = [&](int o) {
    std::unordered_set<int> distinct;  // pack (kind, value) into one int
    for (const auto& c : state.deck[o].clues) {
      distinct.insert(static_cast<int>(c.kind) * 100 + c.value);
    }
    return static_cast<int>(distinct.size());
  };
  int best = valid.front();
  int best_count = positive_clue_count(best);
  for (size_t i = 1; i < valid.size(); ++i) {
    int count = positive_clue_count(valid[i]);
    if (count > best_count) {
      best = valid[i];
      best_count = count;
    }
  }
  if (ignore.count(best)) return std::nullopt;
  return best;
}

std::vector<int> Player::find_clued(const Game& prev, int player_index, Identity id,
                                       const std::unordered_set<int>& ignore) const {
  const State& state = prev.state;
  std::vector<int> result;
  for (int order : state.hands[player_index]) {
    if (ignore.count(order)) continue;
    if (!state.deck[order].clued) continue;
    const Thought& thought = thoughts[order];
    if (!thought.possible.contains(id)) continue;
    if (thought.info_lock && !thought.info_lock->contains(id)) continue;
    if (thought.inferred.length() == 1 && !thought.inferred.contains(id)) continue;
    result.push_back(order);
  }
  return result;
}

// --- locked_discard / anxiety_play ----------------------------------------

int Player::locked_discard(const State& state, int player_index) const {
  const auto& hand = state.hands[player_index];
  std::vector<std::pair<int, double>> crit_percents;
  crit_percents.reserve(hand.size());
  for (int o : hand) {
    IdentitySet poss = thoughts[o].possibilities();
    double percent = 0.0;
    if (poss.length()) {
      percent = static_cast<double>(poss.intersect(state.critical_set).length()) /
                 static_cast<double>(poss.length());
    }
    crit_percents.emplace_back(o, percent);
  }
  std::sort(crit_percents.begin(), crit_percents.end(),
             [](const auto& a, const auto& b) { return a.second < b.second; });
  double min_percent = crit_percents.front().second;
  std::vector<std::pair<int, double>> least_crits;
  for (const auto& t : crit_percents) {
    if (t.second == min_percent) least_crits.push_back(t);
  }

  auto total_score = [&](const std::pair<int, double>& t) {
    int total = 0;
    for (Identity p : thoughts[t.first].possibilities()) {
      if (state.is_basic_trash(p)) {
        total += 5;
      } else {
        int extra = (t.second == 1.0) ? p.rank * 5 : 0;
        total += extra + p.rank - hypo_stacks[p.suit_index];
      }
    }
    return total;
  };

  auto best_it = std::max_element(
      least_crits.begin(), least_crits.end(),
      [&](const auto& a, const auto& b) { return total_score(a) < total_score(b); });
  return best_it->first;
}

std::optional<int> Player::anxiety_play(const State& state, int player_index) const {
  const auto& hand = state.hands[player_index];
  bool any_playable = false;
  for (int o : hand) {
    if (thoughts[o].possibilities().intersect(state.playable_set).non_empty()) {
      any_playable = true;
      break;
    }
  }
  if (!any_playable) return std::nullopt;

  auto score = [&](int i, int o) {
    IdentitySet poss = thoughts[o].possibilities();
    if (poss.length() == 0) return static_cast<double>(-i);
    double percent = static_cast<double>(poss.intersect(state.playable_set).length()) /
                       static_cast<double>(poss.length());
    return percent * 1000.0 - i;
  };

  int best_idx = 0;
  double best_score = score(0, hand[0]);
  for (size_t i = 1; i < hand.size(); ++i) {
    double s = score(static_cast<int>(i), hand[i]);
    if (s > best_score) {
      best_score = s;
      best_idx = static_cast<int>(i);
    }
  }
  return hand[best_idx];
}

// --- update_hypo_stacks ---------------------------------------------------

Player Player::update_hypo_stacks(const Game& game) const {
  // Build a hypothetical Game, mutate it as plays "happen".
  Game hypo = game;
  if (is_common) {
    hypo.common = *this;
  } else {
    hypo.players[player_index] = *this;
  }
  hypo.no_recurse = true;
  hypo.clean_hypo();

  std::unordered_set<int> unknown_plays_set;
  std::unordered_set<int> played;
  std::unordered_set<int> attempted;
  int linked_plays_count = 0;

  auto get_player = [&]() -> Player& {
    return is_common ? hypo.common : hypo.players[player_index];
  };

  auto play_order = [&](int order) {
    int holder = hypo.state.holder_of(order);
    auto id = get_player().thoughts[order].id(/*infer=*/true);

    if (!id) {
      // Linked play resolution.
      for (const Link& link : links) {
        bool in_link = false;
        std::visit(
            [&](const auto& l) {
              if (any_contains(l.orders, order)) in_link = true;
            },
            link);
        if (!in_link) continue;
        bool siblings_played = true;
        const auto& orders = link_orders(link);
        for (int o : orders) {
          if (o != order && !played.count(o)) {
            siblings_played = false;
            break;
          }
        }
        if (siblings_played) {
          auto promise = link_promise(link);
          if (promise && hypo.state.is_playable(*promise)) {
            ++linked_plays_count;
            hypo.with_state([&](State& s) { s = s.with_play(*promise); });
          }
        }
      }
      unknown_plays_set.insert(order);
      played.insert(order);
    } else if (hypo.state.is_playable(*id)) {
      PlayAction pa{holder, order, id->suit_index, id->rank};
      Game prev_hypo = hypo;
      hypo.on_play(pa);
      hypo.refresh_after_play(prev_hypo, pa);
      played.insert(order);
    } else {
      attempted.insert(order);
    }

    // Whether played or attempted, remove from holder's hand in the hypo state.
    hypo.with_state([&](State& s) {
      auto& h = s.hands[holder];
      h.erase(std::remove(h.begin(), h.end(), order), h.end());
    });
  };

  bool changed = true;
  while (changed) {
    changed = false;
    Player& player = get_player();
    bool outer_break = false;
    for (int i = 0; i < hypo.state.num_players; ++i) {
      std::vector<int> playables;
      if (game.good_touch) {
        playables = player.thinks_playables(hypo, i, /*exclude_trash=*/true);
      } else {
        playables = player.obvious_playables(hypo, i);
      }
      for (int o : playables) {
        if (played.count(o) || attempted.count(o)) continue;
        if (!hypo.state.has_consistent_infs(thoughts[o])) continue;
        play_order(o);
        changed = true;
        outer_break = true;
        break;
      }
      if (outer_break) break;
    }
    if (changed) continue;

    // No card played this round; try play_links.
    Player& player2 = get_player();
    for (const PlayLink& link : player2.play_links) {
      bool all_played = true;
      for (int o : link.orders) {
        if (!played.count(o)) {
          all_played = false;
          break;
        }
      }
      if (!all_played || played.count(link.target)) continue;
      bool in_play = false;
      for (const auto& h : hypo.state.hands) {
        if (any_contains(h, link.target)) {
          in_play = true;
          break;
        }
      }
      if (!in_play) continue;
      auto target_id = hypo.state.deck[link.target].id();
      if (!target_id || hypo.state.is_useful(*target_id)) {
        play_order(link.target);
        changed = true;
        break;
      }
    }
  }

  Player out = *this;
  out.hypo_stacks = hypo.state.play_stacks;
  out.unknown_plays = std::move(unknown_plays_set);
  out.hypo_plays = std::move(played);
  out.linked_plays = linked_plays_count;
  return out;
}

}  // namespace hanabi
