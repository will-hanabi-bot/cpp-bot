#include "diagnostics.h"

#include <algorithm>
#include <utility>

#include "hanabi/basics/interp.h"

namespace hanabi::selfplay {

using nlohmann::json;

namespace {

bool is_call(CardStatus s) {
  return s == CardStatus::CALLED_TO_PLAY || s == CardStatus::CALLED_TO_DISCARD;
}

std::optional<Identity> only(const IdentitySet& s) {
  if (s.length() != 1) return std::nullopt;
  for (Identity i : s) return i;
  return std::nullopt;
}

std::string interp_str(const Interp& i) {
  if (const auto* c = std::get_if<ClueInterp>(&i)) return std::string(name(*c));
  if (std::holds_alternative<PlayInterp>(i)) return "play";
  return "discard";
}

// A card the truth still needs: not yet played, and its suit can still reach it.
bool still_needed(const Variant& v, const TrueState& t, Identity id) {
  const int next = t.next_rank(v, id.suit_index);
  if (next == 0) return false;
  return v.suits[id.suit_index].suit_type.reversed ? id.rank <= next : id.rank >= next;
}

const ReactorWC* latest_wc_from(const Game& g, int giver) {
  for (auto it = g.waiting.rbegin(); it != g.waiting.rend(); ++it) {
    if (it->giver == giver) return &*it;
  }
  return nullptr;
}

}  // namespace

Diagnostics::Diagnostics(const Sim& sim)
    : variant_(&sim.variant()), np_(sim.num_players()) {
  for (int p = 0; p < np_; ++p) log_paths_.push_back(sim.log_path(p));
}

SimHooks Diagnostics::hooks() {
  SimHooks h;
  h.before_action = [this](const Sim& sim) { before_action(sim); };
  h.after_action = [this](const Sim& sim, const Outcome& o) { after_action(sim, o); };
  return h;
}

std::string Diagnostics::id_str(Identity id) const {
  std::string s;
  const auto& sf = variant_->short_forms;
  s += id.suit_index < static_cast<int>(sf.size()) ? sf[id.suit_index] : '?';
  s += std::to_string(id.rank);
  return s;
}

std::string Diagnostics::set_str(const IdentitySet& set) const {
  std::string s = "{";
  bool first = true;
  for (Identity i : set) {
    if (!first) s += ",";
    s += id_str(i);
    first = false;
  }
  return s + "}";
}

void Diagnostics::add(Issue i) {
  const auto key = std::make_tuple(i.cls, i.kind, i.seat, i.order,
                                   i.detail.value("view_seat", -1));
  auto [it, fresh] = seen_.emplace(key, 0);
  ++it->second;
  if (!fresh) return;  // first occurrence only
  if (i.seat >= 0 && i.seat < static_cast<int>(log_paths_.size()) &&
      !log_paths_[i.seat].empty()) {
    // The STATE a rerun reconstructs at turn N is the position BEFORE turn N's
    // action, so the position this action produced is turn + 1.
    i.detail["replay"] = "build/replay_log.exe " + log_paths_[i.seat] + " --turn " +
                         std::to_string(i.turn + 1) + " --stacks";
  }
  issues_.push_back(std::move(i));
}

void Diagnostics::before_action(const Sim& sim) {
  const TrueState& t = sim.truth();
  pre_status_.assign(np_, {});
  pre_state_.clear();
  for (int s = 0; s < np_; ++s) {
    const Game& g = sim.seat(s);
    for (const ConvData& m : g.meta) pre_status_[s].push_back(m.status);
    pre_state_.push_back(g.state);
  }
  pre_truth_ = t;
  actor_named_.clear();
  actor_named_common_.clear();
  const int a = t.current;
  const Game& ga = sim.seat(a);
  for (int o : t.hands[a]) {
    if (auto id = only(ga.players[a].thoughts[o].possibilities())) actor_named_[o] = *id;
    if (auto id = only(ga.common.thoughts[o].possibilities())) actor_named_common_[o] = *id;
  }
}

void Diagnostics::after_action(const Sim& sim, const Outcome& o) {
  const int turn = o.turn + 1;
  if (o.kind == Outcome::Kind::PLAY_LANDED) {
    Landed l{o.order, *o.id, o.actor, false, false};
    if (auto it = actor_named_.find(o.order); it != actor_named_.end()) {
      l.named_by_player = it->second == *o.id;
    }
    if (auto it = actor_named_common_.find(o.order); it != actor_named_common_.end()) {
      l.named_in_common = it->second == *o.id;
      l.named_by_player = l.named_by_player || l.named_in_common;
    }
    landed_.push_back(l);
  }
  if (o.kind == Outcome::Kind::PLAY_MISSED) check_strike(sim, o, turn);
  check_inferences(sim, turn);
  check_calls(sim, o, turn);
  if (o.kind == Outcome::Kind::CLUE) {
    check_clue_reading(sim, o, turn);
  } else {
    check_reaction(sim, o, turn);
  }
  check_stacks(sim, turn);
}

// --- (1) -------------------------------------------------------------------

void Diagnostics::check_inferences(const Sim& sim, int turn) {
  const TrueState& t = sim.truth();
  for (int s = 0; s < np_; ++s) {
    const Game& g = sim.seat(s);
    for (int h = 0; h < np_; ++h) {
      for (int o : t.hands[h]) {
        const Identity truth = t.deck[o];
        const Thought& c = g.common.thoughts[o];
        auto report = [&](const char* kind, const IdentitySet& set) {
          add(Issue{"1", kind, turn, s, o,
                    json{{"holder", h},
                         {"truth", id_str(truth)},
                         {"set", set_str(set)},
                         {"status", std::string(name(g.meta[o].status))}}});
        };
        if (!c.possible.contains(truth)) {
          report("common_possible", c.possible);
        } else if (c.inferred.non_empty() && !c.inferred.contains(truth)) {
          report("common_inferred", c.inferred);
        }
        if (h == s) {
          const Thought& mine = g.players[s].thoughts[o];
          if (!mine.possible.contains(truth)) {
            report("own_possible", mine.possible);
          } else if (mine.inferred.non_empty() && !mine.inferred.contains(truth)) {
            report("own_inferred", mine.inferred);
          }
        }
      }
    }
    // Hole cards: the superposition, and the set the shared view keeps.
    for (int o = 0; o < static_cast<int>(g.meta.size()) &&
                    o < static_cast<int>(t.deck.size());
         ++o) {
      const ConvData& m = g.meta[o];
      const Identity truth = t.deck[o];
      if (m.superposed() && !m.superposition.contains(truth)) {
        add(Issue{"1", "superposition", turn, s, o,
                  json{{"holder", t.holder[o]},
                       {"truth", id_str(truth)},
                       {"set", set_str(m.superposition)}}});
      }
      if (m.shared_left.non_empty() && !m.shared_left.contains(truth)) {
        add(Issue{"1", "shared_left", turn, s, o,
                  json{{"holder", t.holder[o]},
                       {"truth", id_str(truth)},
                       {"set", set_str(m.shared_left)}}});
      }
      if (m.named_in_hole.non_empty() && !m.named_in_hole.contains(truth)) {
        add(Issue{"1", "named_in_hole", turn, s, o,
                  json{{"holder", t.holder[o]},
                       {"truth", id_str(truth)},
                       {"set", set_str(m.named_in_hole)}}});
      }
    }
  }
}

// --- (2) -------------------------------------------------------------------

void Diagnostics::check_calls(const Sim& sim, const Outcome& o, int turn) {
  const TrueState& t = sim.truth();
  const Variant& v = *variant_;

  // The card the actor just actioned.
  if (o.order >= 0) {
    const CardStatus was = pre_status_[o.actor][o.order];
    if (was == CardStatus::CALLED_TO_PLAY && o.kind == Outcome::Kind::DISCARD &&
        !o.pressed_play && still_needed(v, t, *o.id)) {
      const bool playable = pre_truth_.playable(v, *o.id);
      add(Issue{"2", "discarded_call", turn, o.actor, o.order,
                json{{"truth", id_str(*o.id)}, {"truly_playable", playable}}});
    }
    calls_.erase(o.order);
  }

  // Idle: the actor held a standing, truly playable call and did something else.
  const int a = o.actor;
  for (int c : pre_truth_.hands[a]) {
    if (c == o.order) continue;
    if (c >= static_cast<int>(pre_status_[a].size())) continue;
    if (pre_status_[a][c] != CardStatus::CALLED_TO_PLAY) continue;
    const Identity id = t.deck[c];
    const bool was_playable = pre_truth_.playable(v, id);
    if (!was_playable) continue;
    CallTrack& tr = calls_[c];
    ++tr.idle_turns;
    if (tr.idle_turns >= 2 && !tr.idle_reported) {
      tr.idle_reported = true;
      add(Issue{"2w", "idle", turn, a, c,
                json{{"truth", id_str(id)},
                     {"idle_turns", tr.idle_turns},
                     {"inferred", set_str(sim.seat(a).common.thoughts[c].inferred)}}});
    }
  }

  // Status transitions, and calls the holder does not share.
  for (int h = 0; h < np_; ++h) {
    for (int c : t.hands[h]) {
      const Identity id = t.deck[c];
      const bool needed = still_needed(v, t, id);
      for (int s = 0; s < np_; ++s) {
        const Game& g = sim.seat(s);
        if (c >= static_cast<int>(g.meta.size())) continue;
        const CardStatus now = g.meta[c].status;
        const CardStatus before = c < static_cast<int>(pre_status_[s].size())
                                      ? pre_status_[s][c]
                                      : CardStatus::NONE;
        if (s != h) continue;
        if (now == CardStatus::CALLED_TO_PLAY && before != CardStatus::CALLED_TO_PLAY) {
          calls_[c] = CallTrack{turn, 0, false};
        }
        if (before == CardStatus::CALLED_TO_PLAY && now != CardStatus::CALLED_TO_PLAY) {
          const Thought& th = g.common.thoughts[c];
          add(Issue{needed ? "2" : "2-trash", "dropped", turn, s, c,
                    json{{"truth", id_str(id)},
                         {"truly_playable", t.playable(v, id)},
                         {"inferred", set_str(th.inferred)},
                         {"inferred_empty", th.inferred.is_empty()},
                         {"status_now", std::string(name(now))}}});
          calls_.erase(c);
        }
      }
      // Another seat sees a call on this card that its holder does not.
      if (sim.seat(h).meta[c].status != CardStatus::CALLED_TO_PLAY) {
        std::vector<int> others;
        for (int s = 0; s < np_; ++s) {
          if (s != h && sim.seat(s).meta[c].status == CardStatus::CALLED_TO_PLAY) {
            others.push_back(s);
          }
        }
        if (!others.empty()) {
          add(Issue{needed ? "2" : "2-trash", "not_at_holder", turn, h, c,
                    json{{"truth", id_str(id)},
                         {"seen_by", others},
                         {"holder_status", std::string(name(sim.seat(h).meta[c].status))},
                         {"truly_playable", t.playable(v, id)}}});
        }
      }
    }
  }

  // The end: a call still standing that its holder had a turn to play.
  if (t.ended && *t.ended != EndCondition::STRIKEOUT) {
    for (int h = 0; h < np_; ++h) {
      for (int c : t.hands[h]) {
        if (sim.seat(h).meta[c].status != CardStatus::CALLED_TO_PLAY) continue;
        auto it = calls_.find(c);
        const int idle = it == calls_.end() ? 0 : it->second.idle_turns;
        if (idle == 0) continue;
        add(Issue{"2", "unactioned_at_end", turn, h, c,
                  json{{"truth", id_str(t.deck[c])}, {"idle_turns", idle}}});
      }
    }
  }
}

// --- (3) -------------------------------------------------------------------

void Diagnostics::check_clue_reading(const Sim& sim, const Outcome& o, int turn) {
  std::vector<std::string> readings;
  for (int s = 0; s < np_; ++s) {
    const auto& mh = sim.seat(s).move_history;
    readings.push_back(mh.empty() ? "?" : interp_str(mh.back()));
  }
  const bool agree =
      std::all_of(readings.begin(), readings.end(),
                  [&](const std::string& r) { return r == readings[0]; });
  if (!agree) {
    add(Issue{"3", "dispatch_disagreement", turn, o.actor, -1,
              json{{"readings", readings},
                   {"clue_target", o.clue_target},
                   {"clue", (o.clue_kind == ClueKind::COLOUR ? "colour " : "rank ") +
                                std::to_string(o.clue_value)}}});
  }
  if (!variant_->throw_it_in_a_hole) return;
  if (readings[o.actor] != "Reactive") return;

  const ReactorWC* g = latest_wc_from(sim.seat(o.actor), o.actor);
  if (!g) return;
  const ReactorWC* r = latest_wc_from(sim.seat(g->reacter), o.actor);
  if (!r || r->react_order != g->react_order ||
      r->receiver_target_order != g->receiver_target_order) {
    add(Issue{"3", "pairing_mismatch", turn, g->reacter, g->receiver_target_order,
              json{{"giver", o.actor},
                   {"receiver", g->receiver},
                   {"giver_react_order", g->react_order},
                   {"giver_target", g->receiver_target_order},
                   {"reacter_react_order", r ? r->react_order : -2},
                   {"reacter_target", r ? r->receiver_target_order : -2}}});
  }
  if (g->receiver_target_order >= 0) {
    pending_[g->receiver] = PendingReaction{o.actor, g->reacter, g->receiver,
                                            g->receiver_target_order, g->react_order,
                                            turn};
  }
}

void Diagnostics::check_reaction(const Sim& sim, const Outcome& o, int turn) {
  const TrueState& t = sim.truth();
  for (auto it = pending_.begin(); it != pending_.end();) {
    const PendingReaction p = it->second;
    if (p.reacter != o.actor) {
      ++it;
      continue;
    }
    it = pending_.erase(it);
    const auto& hand = t.hands[p.receiver];
    if (std::find(hand.begin(), hand.end(), p.target) == hand.end()) continue;
    auto newly_called = [&](int seat) {
      std::vector<int> out;
      const Game& g = sim.seat(seat);
      for (int c : hand) {
        const CardStatus before = c < static_cast<int>(pre_status_[seat].size())
                                      ? pre_status_[seat][c]
                                      : CardStatus::NONE;
        if (!is_call(before) && is_call(g.meta[c].status)) out.push_back(c);
      }
      return out;
    };
    const auto at_receiver = newly_called(p.receiver);
    if (std::find(at_receiver.begin(), at_receiver.end(), p.target) != at_receiver.end()) {
      continue;
    }
    add(Issue{"3", "receiver_stamp_mismatch", turn, p.receiver, p.target,
              json{{"giver", p.giver},
                   {"reacter", p.reacter},
                   {"clue_turn", p.clue_turn},
                   {"giver_target", p.target},
                   {"giver_react_order", p.react_order},
                   {"reacter_actioned", o.order},
                   {"receiver_new_calls", at_receiver},
                   {"giver_new_calls", newly_called(p.giver)},
                   {"reacter_new_calls", newly_called(p.reacter)},
                   {"target_truth", id_str(t.deck[p.target])}}});
  }
}

// --- (4) -------------------------------------------------------------------

void Diagnostics::check_stacks(const Sim& sim, int turn) {
  const TrueState& t = sim.truth();
  const Variant& v = *variant_;
  const int suits = static_cast<int>(v.suits.size());

  if (!v.throw_it_in_a_hole) {
    // Everything is public: every seat's stacks are the true stacks.
    for (int s = 0; s < np_; ++s) {
      const auto& ps = sim.seat(s).state.play_stacks;
      for (int k = 0; k < suits; ++k) {
        if (v.suits[k].suit_type.reversed) continue;
        if (ps[k] != t.stacks[k]) {
          add(Issue{"4", "public_mismatch", turn, s, k,
                    json{{"view", ps[k]}, {"truth", t.stacks[k]}}});
        }
      }
    }
    return;
  }

  auto player_names = [&](const Landed& l) {
    if (l.named_by_player) return true;
    const Game& g = sim.seat(l.player);
    return l.order < static_cast<int>(g.meta.size()) &&
           !g.meta[l.order].superposed();
  };
  auto common_names = [&](const Landed& l) {
    if (l.named_in_common) return true;
    for (int s = 0; s < np_; ++s) {
      const Game& g = sim.seat(s);
      if (l.order < static_cast<int>(g.meta.size()) &&
          g.meta[l.order].named_in_hole.contains(l.id)) {
        return true;
      }
    }
    return false;
  };
  // floor(P, k): the highest landed rank every member of P saw.
  auto floor_for = [&](const std::vector<int>& members, bool common) {
    std::vector<int> f(suits, 0);
    for (const Landed& l : landed_) {
      if (v.suits[l.id.suit_index].suit_type.reversed) continue;
      bool all = true;
      if (common) {
        all = common_names(l);
      } else {
        for (int m : members) {
          if (m == l.player && !player_names(l)) all = false;
        }
      }
      if (all) f[l.id.suit_index] = std::max(f[l.id.suit_index], static_cast<int>(l.id.rank));
    }
    return f;
  };
  auto check = [&](int s, const char* view, int view_seat, const std::vector<int>& stacks,
                   const std::vector<int>& floor) {
    for (int k = 0; k < suits && k < static_cast<int>(stacks.size()); ++k) {
      if (v.suits[k].suit_type.reversed) continue;
      if (stacks[k] < floor[k]) {
        add(Issue{"4", std::string(view) + "_below", turn, s, k,
                  json{{"view_seat", view_seat},
                       {"suit", std::string(1, v.short_forms[k])},
                       {"view", stacks[k]},
                       {"floor", floor[k]},
                       {"truth", t.stacks[k]}}});
      }
      if (stacks[k] > t.stacks[k]) {
        add(Issue{"4-above", std::string(view) + "_above", turn, s, k,
                  json{{"view_seat", view_seat},
                       {"suit", std::string(1, v.short_forms[k])},
                       {"view", stacks[k]},
                       {"truth", t.stacks[k]}}});
      }
    }
  };

  const auto common_floor = floor_for({}, true);
  for (int s = 0; s < np_; ++s) {
    const State& st = sim.seat(s).state;
    check(s, "private", -1, st.play_stacks, floor_for({s}, false));
    for (int p = 0; p < np_; ++p) {
      if (p == s || p >= static_cast<int>(st.pairwise_play_stacks.size())) continue;
      check(s, "pair", p, st.pairwise_play_stacks[p], floor_for({s, p}, false));
    }
    check(s, "common", -1, st.common_play_stacks, common_floor);
  }
}

// --- (5) -------------------------------------------------------------------

void Diagnostics::check_strike(const Sim& sim, const Outcome& o, int turn) {
  const Game& g = sim.seat(o.actor);
  std::vector<int> saw_dead;
  for (int s = 0; s < np_; ++s) {
    if (s == o.actor) continue;
    if (!pre_state_[s].is_playable(*o.id)) saw_dead.push_back(s);
  }
  add(Issue{"5", "strike", turn, o.actor, o.order,
            json{{"truth", id_str(*o.id)},
                 {"status", std::string(name(pre_status_[o.actor][o.order]))},
                 {"inferred", set_str(g.common.thoughts[o.order].inferred)},
                 {"teammates_saw_dead", saw_dead}}});
}

void Diagnostics::finish(const Sim& sim, const SimResult& r) {
  if (r.error) {
    add(Issue{r.error->kind, "error", r.error->turn + 1, r.error->seat, -1,
              json{{"what", r.error->what}}});
  }
  (void)sim;
}

}  // namespace hanabi::selfplay
