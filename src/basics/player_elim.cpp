#include "hanabi/basics/player_elim.h"

#include <algorithm>

#include "hanabi/basics/card.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/state.h"

namespace hanabi {
namespace {

void replace_or_prepend_entry(std::vector<MatchEntry>& entries, int order,
                                MatchEntry new_entry) {
  for (auto& e : entries) {
    if (e.order == order) {
      e = new_entry;
      return;
    }
  }
  entries.insert(entries.begin(), new_entry);
}

// Forward declaration.
CardElimResult basic_elim(Player p, const State& state, IdentitySet ids);

// Returns a result with `player` reflecting the post-pass state.
// Updates: removes `id` from possibilities everywhere it's not pinned;
// queues newly-singleton possibilities into recursive_ids.
CardElimResult update_map(Player p, const State& state, Identity id,
                            const std::unordered_set<int>& exclude = {},
                            const std::unordered_set<int>& exclude_own = {}) {
  CardElimResult res;
  res.player = std::move(p);

  for (int player_index = 0; player_index < state.num_players; ++player_index) {
    if (player_index != res.player.player_index && exclude.count(player_index)) continue;
    for (int order : state.hands[player_index]) {
      const Thought& thought = res.player.thoughts[order];
      if (!thought.possible.contains(id)) continue;

      bool covered = false;
      for (const auto& e : res.player.certain_map[id.to_ord()]) {
        if (e.order == order || e.unknown_to == player_index) {
          covered = true;
          break;
        }
      }
      if (covered) continue;
      if (exclude_own.count(order)) continue;

      res.changed = true;
      IdentitySet new_inferred = thought.inferred.difference(id);
      IdentitySet new_possible = thought.possible.difference(id);
      bool reset_card = new_inferred.is_empty() && !thought.reset;

      Thought next = thought;
      next.possible = new_possible;
      if (reset_card) {
        next = next.reset_inferences();
      } else if (thought.info_lock) {
        IdentitySet new_lock_set = thought.info_lock->difference(id);
        next.inferred = new_inferred;
        next.info_lock = new_lock_set.is_empty() ? std::optional<IdentitySet>{} : new_lock_set;
      } else {
        next.inferred = new_inferred;
      }

      res.player.thoughts[order] = next;
      res.player.dirty.insert(order);
      if (reset_card) res.resets.insert(order);

      if (new_possible.length() == 1) {
        Identity rec_id = new_possible.head();
        replace_or_prepend_entry(res.player.certain_map[rec_id.to_ord()], order,
                                  MatchEntry{order, -1});
        res.recursive_ids = res.recursive_ids.add(rec_id);
        res.removals.insert(order);
      }
    }
  }
  return res;
}

CardElimResult basic_elim(Player p, const State& state, IdentitySet ids) {
  CardElimResult res;
  res.player = std::move(p);
  IdentitySet eliminated;

  for (Identity id : ids) {
    int known_count = static_cast<int>(res.player.certain_map[id.to_ord()].size());
    if (known_count == state.card_count[id.to_ord()]) {
      CardElimResult inner = update_map(res.player, state, id);
      res.merge(std::move(inner));
      eliminated = eliminated.add(id);
    }
  }

  if (res.recursive_ids.non_empty()) {
    IdentitySet rec = res.recursive_ids;
    res.recursive_ids = IdentitySet::empty();
    CardElimResult inner = basic_elim(res.player, state, rec);
    res.merge(std::move(inner));
  }

  res.player.all_possible = res.player.all_possible.difference(eliminated);
  return res;
}

CardElimResult perform_cross_elim(Player p, const State& state,
                                    const std::vector<int>& entries_vec,
                                    const std::unordered_set<int>& holders,
                                    IdentitySet ids) {
  CardElimResult res;
  res.player = std::move(p);

  const int num_ords = static_cast<int>(state.variant->suits.size()) * 5;
  std::vector<std::vector<int>> groups(num_ords);
  IdentitySet group_ids;

  std::unordered_set<int> entries_set(entries_vec.begin(), entries_vec.end());

  for (int o : entries_vec) {
    auto id = state.deck[o].id();
    if (id) {
      groups[id->to_ord()].insert(groups[id->to_ord()].begin(), o);
      group_ids = group_ids.add(*id);
    }
  }

  for (Identity id : group_ids) {
    const auto& group = groups[id.to_ord()];
    int certains_outside = 0;
    for (const auto& c : res.player.certain_map[id.to_ord()]) {
      if (std::find(group.begin(), group.end(), c.order) == group.end()) ++certains_outside;
    }
    if (static_cast<int>(group.size()) ==
        state.card_count[id.to_ord()] - certains_outside) {
      std::unordered_set<int> exclude_holders;
      for (int o : group) exclude_holders.insert(state.holder_of(o));
      CardElimResult inner = update_map(res.player, state, id, exclude_holders);
      res.merge(std::move(inner));
    }
  }

  std::unordered_set<int> own_hand_entries;
  if (!res.player.is_common) {
    const auto& own_hand = state.hands[res.player.player_index];
    for (int e : entries_vec) {
      if (std::find(own_hand.begin(), own_hand.end(), e) != own_hand.end()) {
        own_hand_entries.insert(e);
      }
    }
  }

  for (Identity id : ids) {
    CardElimResult inner = update_map(res.player, state, id, holders, own_hand_entries);
    res.merge(std::move(inner));
  }

  CardElimResult inner = basic_elim(res.player, state, ids);
  res.merge(std::move(inner));
  return res;
}

CardElimResult cross_elim(Player p, const State& state,
                            const std::vector<int>& remaining,
                            const std::unordered_set<int>& contained = {},
                            const std::unordered_set<int>& holders = {},
                            IdentitySet acc_ids = IdentitySet::empty(),
                            const std::unordered_set<int>& certains = {}) {
  int multiplicity = state.multiplicity(acc_ids);
  bool impossible =
      multiplicity - static_cast<int>(certains.size()) >
      static_cast<int>(contained.size() + remaining.size());

  CardElimResult res;
  res.player = std::move(p);
  if (impossible) return res;

  if (contained.size() > 1 && multiplicity - static_cast<int>(certains.size()) ==
                                    static_cast<int>(contained.size())) {
    std::vector<int> contained_vec(contained.begin(), contained.end());
    CardElimResult inner = perform_cross_elim(res.player, state, contained_vec,
                                               holders, acc_ids);
    if (inner.changed) return inner;
    res.merge(std::move(inner));
  }

  if (remaining.empty()) return res;

  int order = remaining.front();
  std::vector<int> rest(remaining.begin() + 1, remaining.end());

  IdentitySet new_acc_ids = acc_ids.union_with(res.player.thoughts[order].possible);
  std::unordered_set<int> next_contained = contained;
  next_contained.insert(order);

  IdentitySet delta = res.player.thoughts[order].possible.difference(acc_ids);
  std::unordered_set<int> all_certains = certains;
  if (delta.non_empty()) {
    for (Identity id : delta) {
      for (const auto& c : res.player.certain_map[id.to_ord()]) {
        all_certains.insert(c.order);
      }
    }
  }
  std::unordered_set<int> next_certains;
  for (int c : all_certains) {
    if (!next_contained.count(c)) next_certains.insert(c);
  }

  std::unordered_set<int> next_holders = holders;
  next_holders.insert(state.holder_of(order));

  CardElimResult with_branch = cross_elim(res.player, state, rest, next_contained,
                                             next_holders, new_acc_ids, next_certains);
  if (with_branch.changed) {
    res.merge(std::move(with_branch));
    return res;
  }
  res.merge(std::move(with_branch));
  CardElimResult without_branch =
      cross_elim(res.player, state, rest, contained, holders, acc_ids, certains);
  res.merge(std::move(without_branch));
  return res;
}

}  // namespace

// --- CardElimResult::merge ------------------------------------------------

void CardElimResult::merge(CardElimResult other) {
  player = std::move(other.player);
  changed = changed || other.changed;
  for (int o : other.removals) removals.insert(o);
  for (int o : other.resets) resets.insert(o);
  recursive_ids = recursive_ids.union_with(other.recursive_ids);
}

// --- card_elim ------------------------------------------------------------

std::pair<std::unordered_set<int>, Player> card_elim(Player p, const State& state) {
  if (p.dirty.empty()) return {{}, std::move(p)};

  // Step 1: refresh certain_map for newly-known cards in dirty.
  for (int order : p.dirty) {
    const Thought& thought = p.thoughts[order];
    auto id = thought.id(/*infer=*/false, /*symmetric=*/p.is_common);
    if (!id) continue;
    int unknown_to = thought.id(/*infer=*/false, /*symmetric=*/true).has_value()
                          ? -1
                          : state.holder_of(order);
    auto& certains = p.certain_map[id->to_ord()];
    int idx = -1;
    for (int i = 0; i < static_cast<int>(certains.size()); ++i) {
      if (certains[i].order == order) {
        idx = i;
        break;
      }
    }
    if (idx != -1) {
      if (thought.possible.length() == 1 && certains[idx].unknown_to != -1) {
        certains[idx] = MatchEntry{order, -1};
      }
    } else {
      certains.insert(certains.begin(), MatchEntry{order, unknown_to});
    }
  }

  // Step 2: basic_elim across all variant identities.
  CardElimResult basic = basic_elim(std::move(p), state, state.all_ids);
  Player new_player = std::move(basic.player);
  std::unordered_set<int> resets = std::move(basic.resets);

  // Step 3: collect cross-elim candidates.
  std::vector<int> cross_candidates;
  for (int player_index = 0; player_index < state.num_players; ++player_index) {
    for (int order : state.hands[player_index]) {
      const Thought& thought = new_player.thoughts[order];
      if (thought.possible.length() > 1 && state.multiplicity(thought.possible) <= 9) {
        cross_candidates.insert(cross_candidates.begin(), order);
      }
    }
  }

  auto keep = [&](int order) {
    const Thought& thought = new_player.thoughts[order];
    std::unordered_set<int> certs;
    for (Identity id : thought.possible) {
      for (const auto& c : new_player.certain_map[id.to_ord()]) certs.insert(c.order);
    }
    int bound = std::min(9, static_cast<int>(cross_candidates.size()));
    return state.multiplicity(thought.possible) - static_cast<int>(certs.size()) <= bound;
  };
  std::vector<int> candidates;
  for (int o : cross_candidates) {
    if (keep(o)) candidates.push_back(o);
  }

  // Step 4: iterate cross_elim until no more progress.
  bool changed = true;
  while (candidates.size() > 1 && changed) {
    CardElimResult inner = cross_elim(new_player, state, candidates);
    changed = inner.changed;
    std::vector<int> next;
    for (int o : candidates) {
      if (!inner.removals.count(o)) next.push_back(o);
    }
    candidates = std::move(next);
    for (int o : inner.resets) resets.insert(o);
    new_player = std::move(inner.player);
  }

  return {std::move(resets), std::move(new_player)};
}

// --- good_touch_elim ------------------------------------------------------

std::pair<std::unordered_set<int>, Player> good_touch_elim(Player p, const Game& game,
                                                              std::optional<int> except_) {
  const State& state = game.state;

  auto can_elim = [&](int order) {
    const Thought& thought = p.thoughts[order];
    return !game.meta[order].trash &&
           game.meta[order].status != CardStatus::CALLED_TO_DISCARD &&
           !thought.id(/*infer=*/false, /*symmetric=*/true).has_value() &&
           !thought.inferred.is_empty() &&
           thought.possible.difference(state.trash_set).non_empty() &&
           game.is_touched(order);
  };

  std::unordered_set<int> resets;

  for (int i = 0; i < state.num_players; ++i) {
    if (except_ && *except_ == i) continue;
    for (int order : state.hands[i]) {
      if (!can_elim(order)) continue;
      Thought& thought = p.thoughts[order];
      IdentitySet new_inferred = thought.inferred.difference(state.trash_set);
      bool reset_card = new_inferred.is_empty() && !thought.reset;
      p.dirty.insert(order);
      if (reset_card) {
        thought = thought.reset_inferences();
        resets.insert(order);
      } else {
        thought.inferred = new_inferred;
      }
    }
  }
  return {std::move(resets), std::move(p)};
}

// --- Link maintenance -----------------------------------------------------

Player elim_link(Player p, const Game&, const std::vector<int>& matches, int focus,
                  Identity id) {
  for (int order : matches) {
    Thought& thought = p.thoughts[order];
    IdentitySet new_inferred =
        (order == focus) ? IdentitySet::single(id) : thought.inferred.difference(id);
    if (new_inferred.is_empty() && !thought.reset) {
      thought = thought.reset_inferences();
    } else {
      thought.inferred = new_inferred;
    }
    p.dirty.insert(order);
  }
  return p;
}

Player find_links(Player p, const Game& game) {
  const State& state = game.state;

  auto linkable = [&](int order) {
    const Thought& thought = p.thoughts[order];
    if (thought.id(/*infer=*/false, /*symmetric=*/true).has_value()) return false;
    if (thought.inferred.length() > 2) return false;
    if (thought.inferred.difference(state.trash_set).is_empty()) return false;
    for (const Link& link : p.links) {
      const auto& orders = link_orders(link);
      if (std::find(orders.begin(), orders.end(), order) != orders.end()) return false;
    }
    return true;
  };

  for (const auto& hand : state.hands) {
    // Group hand orders by their inferred IdentitySet (use bits as key).
    std::vector<std::pair<IdentitySet::Bits, std::vector<int>>> inf_map;
    auto find_key = [&](IdentitySet::Bits b) -> std::vector<int>* {
      for (auto& [k, v] : inf_map) {
        if (k == b) return &v;
      }
      inf_map.push_back({b, {}});
      return &inf_map.back().second;
    };
    for (int o : hand) {
      if (!linkable(o)) continue;
      auto* bucket = find_key(p.thoughts[o].inferred.bits());
      bucket->insert(bucket->begin(), o);
    }

    for (const auto& [bits, orders] : inf_map) {
      if (orders.size() <= 1) continue;
      IdentitySet inferred{bits};
      std::vector<int> focused;
      for (int o : orders) {
        if (game.meta[o].focused) focused.push_back(o);
      }
      if (focused.size() == 1 && inferred.length() == 1) {
        p = elim_link(std::move(p), game, orders, focused.front(), inferred.head());
      } else if (orders.size() > static_cast<size_t>(inferred.length())) {
        UnpromisedLink new_link{orders, inferred};
        p.links.insert(p.links.begin(), std::move(new_link));
      }
    }
  }

  return p;
}

std::pair<std::vector<int>, Player> refresh_links(Player p, const Game& game) {
  const State& state = game.state;
  std::vector<Link> old_links = std::move(p.links);
  p.links.clear();
  std::vector<int> sarcastics;

  // foldRight: walk in reverse and prepend to preserve order.
  for (auto it = old_links.rbegin(); it != old_links.rend(); ++it) {
    const Link& link = *it;
    std::visit(
        [&](const auto& l) {
          using T = std::decay_t<decltype(l)>;
          if constexpr (std::is_same_v<T, PromisedLink>) {
            // Resolved if any card already symmetrically matches.
            for (int o : l.orders) {
              if (p.thoughts[o].matches(l.id, /*infer=*/false, /*symmetric=*/true)) {
                return;
              }
            }
            // Target lost the relevant suit -> irrelevant.
            bool any_with_suit = false;
            for (Identity i : p.thoughts[l.target].possible) {
              if (i.suit_index == l.id.suit_index) {
                any_with_suit = true;
                break;
              }
            }
            if (!any_with_suit) return;

            std::vector<int> viable;
            for (int o : l.orders) {
              if (p.thoughts[o].possible.contains(l.id)) viable.push_back(o);
            }
            if (viable.empty()) return;
            if (viable.size() == 1) {
              Identity id = l.id;
              p = p.with_thought(viable.front(), [&](const Thought& t) {
                Thought out = t;
                out.inferred = IdentitySet::single(id);
                return out;
              });
            } else {
              PromisedLink kept{std::move(viable), l.id, l.target};
              p.links.insert(p.links.begin(), std::move(kept));
            }
          } else if constexpr (std::is_same_v<T, SarcasticLink>) {
            for (int o : l.orders) {
              if (p.thoughts[o].matches(l.id)) return;
            }
            std::vector<int> viable;
            for (int o : l.orders) {
              if (p.thoughts[o].possible.contains(l.id)) viable.push_back(o);
            }
            if (viable.empty()) return;
            if (viable.size() == 1) {
              int o = viable.front();
              Identity id = l.id;
              p = p.with_thought(o, [&](const Thought& t) {
                Thought out = t;
                out.inferred = IdentitySet::single(id);
                return out;
              });
              sarcastics.insert(sarcastics.begin(), o);
            } else {
              SarcasticLink kept{std::move(viable), l.id};
              p.links.insert(p.links.begin(), std::move(kept));
            }
          } else {
            std::unordered_set<int> in_play;
            for (const auto& hand : state.hands) {
              for (int o : hand) in_play.insert(o);
            }
            bool revealed = false;
            for (int o : l.orders) {
              const Thought& thought = p.thoughts[o];
              if (thought.id(/*infer=*/false, /*symmetric=*/true)) {
                revealed = true;
                break;
              }
              bool all_in_possible = true;
              for (Identity i : l.ids) {
                if (!thought.possible.contains(i)) {
                  all_in_possible = false;
                  break;
                }
              }
              if (!all_in_possible) {
                revealed = true;
                break;
              }
              if (!in_play.count(o)) {
                revealed = true;
                break;
              }
            }
            if (revealed) return;

            std::vector<int> focused;
            for (int o : l.orders) {
              if (game.meta[o].focused) focused.push_back(o);
            }
            if (focused.size() == 1 && l.ids.length() == 1) {
              p = elim_link(std::move(p), game, l.orders, focused.front(), l.ids.head());
            } else {
              bool lost = false;
              for (Identity i : l.ids) {
                for (int o : l.orders) {
                  if (!p.thoughts[o].inferred.contains(i)) {
                    lost = true;
                    break;
                  }
                }
                if (lost) break;
              }
              if (!lost) {
                UnpromisedLink kept{l.orders, l.ids};
                p.links.insert(p.links.begin(), Link{std::move(kept)});
              }
            }
          }
        },
        link);
  }

  Player new_player = find_links(std::move(p), game);
  return {std::move(sarcastics), std::move(new_player)};
}

Player refresh_play_links(Player p, const Game& game) {
  std::vector<PlayLink> old_links = std::move(p.play_links);
  p.play_links.clear();

  std::unordered_set<int> in_play;
  for (const auto& hand : game.state.hands) {
    for (int o : hand) in_play.insert(o);
  }

  for (auto it = old_links.rbegin(); it != old_links.rend(); ++it) {
    const PlayLink& pl = *it;
    std::vector<int> rem;
    for (int o : pl.orders) {
      if (in_play.count(o)) rem.push_back(o);
    }
    if (rem.empty()) {
      // v0.26: previously this narrowed `inferred` to
      // `intersect(state.playable_set)` whenever the link resolved
      // (= all prereq orders left hands). That intersection fires
      // every play-event and over-narrows CTP'd cards based on the
      // CURRENT playable_set rather than physical evidence about the
      // card's identity. Replay 1892397: yagami's g2 inferred
      // {r3,y1,g2,b3,p3,i1} narrowed all the way down to {r3} via
      // repeated playable_set intersections, even though only the
      // p3-by-will-bot69 play provided actual visibility evidence.
      //
      // The visibility-based narrowing (`card_elim` in this file
      // earlier) handles legitimate "all copies accounted for"
      // shrinking. Resolved links no longer touch `inferred` — the
      // link just drops.
    } else {
      PlayLink kept{std::move(rem), pl.prereqs, pl.target};
      p.play_links.insert(p.play_links.begin(), std::move(kept));
    }
  }

  return p;
}

}  // namespace hanabi
