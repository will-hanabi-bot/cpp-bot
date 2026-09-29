#include "diagnostics.h"

#include <algorithm>
#include <utility>

#include "hanabi/basics/interp.h"
#include "hanabi/conventions/variants/hole.h"

namespace hanabi::selfplay {

using nlohmann::json;

namespace {

std::string str_of(const std::vector<int>& v) {
  std::string c;
  for (int k : v) c += std::to_string(k);
  return c;
}

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
  pre_actor_common_.clear();
  for (int c : t.hands[t.current]) {
    for (int s = 0; s < np_; ++s) pre_actor_common_[c].push_back(sim.seat(s).common.thoughts[c].inferred);
  }
  pre_urgent_.clear();
  for (int c = 0; c < static_cast<int>(sim.seat(t.current).meta.size()); ++c) {
    pre_urgent_.push_back(sim.seat(t.current).meta[c].urgent);
  }
  pre_inferred_.clear();
  for (int h = 0; h < np_; ++h) {
    for (int c : t.hands[h]) {
      std::vector<int> others;
      for (int s = 0; s < np_; ++s) {
        if (s != h) others.push_back(s);
      }
      pre_inferred_[c] = {sim.seat(others[0]).common.thoughts[c].inferred,
                          sim.seat(others[1]).common.thoughts[c].inferred};
    }
  }
  pre_reverse_.assign(np_, false);
  if (variant_->throw_it_in_a_hole) {
    for (int s = 0; s < np_; ++s) {
      pre_reverse_[s] =
          hanabi::reactor::variants::reverse_reactive_position(sim.seat(s), t.current);
    }
  }
  actor_named_.clear();
  actor_named_common_.clear();
  named_at_.clear();
  const int a = t.current;
  const Game& ga = sim.seat(a);
  for (int o : t.hands[a]) {
    if (auto id = only(ga.players[a].thoughts[o].possibilities())) actor_named_[o] = *id;
    // Named in COMMON only if every seat's common reading names it: a seat's
    // `common` can hold what only it and one partner know.
    std::optional<Identity> all;
    bool agree = true;
    std::vector<bool> at(np_, false);
    for (int s = 0; s < np_; ++s) {
      auto id = only(sim.seat(s).common.thoughts[o].possibilities());
      at[s] = id && *id == t.deck[o];
      if (!id || (all && *all != *id)) agree = false;
      all = id;
    }
    named_at_[o] = at;
    if (agree && all) actor_named_common_[o] = *all;
  }
}

void Diagnostics::after_action(const Sim& sim, const Outcome& o) {
  const int turn = o.turn + 1;
  if (o.kind == Outcome::Kind::PLAY_LANDED) {
    Landed l{o.order, *o.id, o.actor, false, false, std::vector<bool>(np_, false)};
    if (auto it = named_at_.find(o.order); it != named_at_.end()) l.named_at = it->second;
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
  // Every urgent (reacter) blind play: did it land, did the reacter's reading hold
  // the truth, and how much hole uncertainty stood at the reacter's seat?
  if ((o.kind == Outcome::Kind::PLAY_LANDED || o.kind == Outcome::Kind::PLAY_MISSED) &&
      o.order < static_cast<int>(pre_status_[o.actor].size()) &&
      pre_status_[o.actor][o.order] == CardStatus::CALLED_TO_PLAY) {
    const Game& ga = sim.seat(o.actor);
    int superposed = 0, own_superposed = 0;
    for (int c = 0; c < static_cast<int>(ga.meta.size()); ++c) {
      if (c == o.order || !ga.meta[c].superposed()) continue;
      ++superposed;
      if (ga.state.holder_of(c) == o.actor) ++own_superposed;
    }
    issues_.push_back(Issue{"stat", "called_play", turn, o.actor, o.order,
                            json{{"landed", o.kind == Outcome::Kind::PLAY_LANDED},
                                 {"urgent", ga.meta[o.order].urgent},
                                 {"reading_had_truth",
                                  ga.common.thoughts[o.order].inferred.contains(*o.id)},
                                 {"superposed", superposed},
                                 {"own_superposed", own_superposed}}});
  }
  // A card the team could no longer get back: the reachable max score fell.
  if (o.order >= 0 && sim.truth().max_score(*variant_) < pre_truth_.max_score(*variant_)) {
    const Game& ga = sim.seat(o.actor);
    const auto& th = ga.common.thoughts[o.order];
    add(Issue{"lost", o.kind == Outcome::Kind::DISCARD ? "discarded" : "struck", turn,
              o.actor, o.order,
              json{{"truth", id_str(*o.id)},
                   {"status", std::string(name(pre_status_[o.actor][o.order]))},
                   {"clued", pre_state_[o.actor].deck[o.order].clued},
                   {"inferred", set_str(th.inferred)},
                   {"clues", pre_truth_.clues},
                   {"lost", pre_truth_.max_score(*variant_) - sim.truth().max_score(*variant_)}}});
  }
  check_inferences(sim, turn);
  check_calls(sim, o, turn);
  if (o.kind == Outcome::Kind::CLUE) {
    check_clue_reading(sim, o, turn);
  } else {
    check_reaction(sim, o, turn);
  }
  check_stacks(sim, turn);
  note_onsets(sim, o, turn);
}

void Diagnostics::note_onsets(const Sim& sim, const Outcome& o, int turn) {
  if (!variant_->throw_it_in_a_hole) return;
  // The TEAM's set for every hole card, which the shared view is built from: the
  // name the team gave it, else the set the shared view keeps for a card we
  // settled privately, else its superposition. One thing at every seat.
  {
    const TrueState& t = sim.truth();
    for (int ord = 0; ord < static_cast<int>(t.deck.size()); ++ord) {
      if (t.holder[ord] < 0) continue;
      std::vector<std::string> sets;
      bool any = false;
      for (int s = 0; s < np_; ++s) {
        const Game& g = sim.seat(s);
        if (ord >= static_cast<int>(g.meta.size())) { sets.push_back("?"); continue; }
        const ConvData& m = g.meta[ord];
        IdentitySet team = m.named_in_hole.non_empty() ? m.named_in_hole
                           : m.shared_left.non_empty() ? m.shared_left
                                                       : m.superposition;
        if (team.non_empty()) any = true;
        sets.push_back(team.non_empty() ? set_str(team) : "-");
      }
      if (!any) continue;
      if (std::any_of(sets.begin(), sets.end(),
                      [&](const std::string& x) { return x != sets[0]; })) {
        add(Issue{"div", "holeset", turn, t.holder[ord], ord,
                  json{{"sets", sets}, {"truth", id_str(t.deck[ord])},
                       {"action_actor", o.actor}, {"action_order", o.order},
                       {"action", o.kind == Outcome::Kind::CLUE ? "clue" : "card"}}});
      }
    }
  }
  auto str = [](const std::vector<int>& v) {
    std::string c;
    for (int k : v) c += std::to_string(k);
    return c;
  };
  // Index 0: common agreement; then one per pair (a,b).
  std::vector<std::string> now;
  {
    std::vector<std::string> commons;
    for (int s = 0; s < np_; ++s) commons.push_back(str(sim.seat(s).state.common_play_stacks));
    const bool agree = std::all_of(commons.begin(), commons.end(),
                                   [&](const std::string& c) { return c == commons[0]; });
    now.push_back(agree ? "=" : "!");
  }
  for (int a = 0; a < np_; ++a) {
    for (int b = a + 1; b < np_; ++b) {
      const auto& ra = sim.seat(a).state.pairwise_play_stacks;
      const auto& rb = sim.seat(b).state.pairwise_play_stacks;
      const bool agree = b < static_cast<int>(ra.size()) &&
                         a < static_cast<int>(rb.size()) && ra[b] == rb[a];
      now.push_back(agree ? "=" : "!");
    }
  }
  if (last_views_.size() == now.size()) {
    std::string kind;
    switch (o.kind) {
      case Outcome::Kind::CLUE: kind = "clue"; break;
      case Outcome::Kind::PLAY_LANDED: kind = "play"; break;
      case Outcome::Kind::PLAY_MISSED: kind = "miss"; break;
      case Outcome::Kind::DISCARD: kind = "discard"; break;
    }
    const CardStatus st = o.order >= 0 && o.order < static_cast<int>(pre_status_[o.actor].size())
                              ? pre_status_[o.actor][o.order]
                              : CardStatus::NONE;
    for (std::size_t k = 0; k < now.size(); ++k) {
      if (last_views_[k] == "=" && now[k] == "!") {
        json d{{"action", kind},
               {"status", std::string(name(st))},
               {"view", static_cast<int>(k)}};
        if (o.order >= 0 && pre_actor_common_.count(o.order)) {
          json reads = json::array();
          for (const IdentitySet& r : pre_actor_common_[o.order]) reads.push_back(set_str(r));
          d["pre_common_readings"] = reads;
          d["urgent"] = o.order < static_cast<int>(pre_urgent_.size()) && pre_urgent_[o.order];
          if (o.id) d["truth"] = id_str(*o.id);
          json commons = json::array();
          for (int s = 0; s < np_; ++s) commons.push_back(str(sim.seat(s).state.common_play_stacks));
          d["commons_after"] = commons;
        }
        issues_.push_back(Issue{"onset", k == 0 ? "common" : "pair", turn, o.actor, o.order, d});
      }
    }
  }
  last_views_ = std::move(now);
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
  // Critical when it touches REACTIVE target selection: some seat reads the clue as
  // reactive and another does not, or the giver and the clue's target disagree.
  // Otherwise it is an outside seat reading a stable clue its own way -- reported,
  // but not class 3.
  const bool any_reactive =
      std::any_of(readings.begin(), readings.end(),
                  [](const std::string& r) { return r == "Reactive"; });
  const bool pair_disagrees = readings[o.actor] != readings[o.clue_target];
  if (!agree) {
    add(Issue{any_reactive || pair_disagrees ? "3" : "3-outside",
              "dispatch_disagreement", turn, o.actor, -1,
              json{{"readings", readings},
                   {"reverse_position",
                    json::array({static_cast<bool>(pre_reverse_[0]),
                                 static_cast<bool>(pre_reverse_[1]),
                                 static_cast<bool>(pre_reverse_[2])})},
                   {"clue_target", o.clue_target},
                   {"clue", (o.clue_kind == ClueKind::COLOUR ? "colour " : "rank ") +
                                std::to_string(o.clue_value)}}});
  }
  if (!variant_->throw_it_in_a_hole) return;
  // Reading ONSETS: cards whose common reading this clue left different at two
  // non-holder seats that had agreed on it before (informational).
  {
    const TrueState& t = sim.truth();
    for (int h = 0; h < np_; ++h) {
      for (int c : t.hands[h]) {
        std::vector<int> others;
        for (int s = 0; s < np_; ++s) {
          if (s != h) others.push_back(s);
        }
        const auto& g0 = sim.seat(others[0]);
        const auto& g1 = sim.seat(others[1]);
        const bool now_differ =
            g0.common.thoughts[c].inferred != g1.common.thoughts[c].inferred;
        if (!now_differ) continue;
        bool differed_before = false;
        if (auto it = pre_inferred_.find(c); it != pre_inferred_.end()) {
          differed_before = it->second.first != it->second.second;
        }
        if (differed_before) continue;
        issues_.push_back(Issue{"onset", "reading", turn, h, c,
                                json{{"giver_reading", readings[o.actor]},
                                     {"giver", o.actor},
                                     {"target", o.clue_target},
                                     {"holder_is_target", h == o.clue_target},
                                     {"a", set_str(g0.common.thoughts[c].inferred)},
                                     {"b", set_str(g1.common.thoughts[c].inferred)},
                                     {"truth", id_str(t.deck[c])}}});
      }
    }
  }
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
                   {"reacter_target", r ? r->receiver_target_order : -2},
                   // The frame each walked in: its row for the other, BEFORE the clue.
                   {"giver_row", str_of(pre_state_[o.actor].stacks_known_to_both(
                                     o.actor, g->reacter))},
                   {"reacter_row", str_of(pre_state_[g->reacter].stacks_known_to_both(
                                       g->reacter, o.actor))}}});
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

  // Settled for the TEAM since (`ConvData::named_in_hole`) at EVERY seat. A name
  // written at one seat only is that seat's, however it is labelled; and a
  // player's PRIVATE settle does not count either: the other party to a view
  // cannot know of it, so a view that ignores it is not lagging.
  auto team_named = [&](const Landed& l) {
    for (int s = 0; s < np_; ++s) {
      const Game& g = sim.seat(s);
      if (l.order >= static_cast<int>(g.meta.size()) ||
          !g.meta[l.order].named_in_hole.contains(l.id)) {
        return false;
      }
    }
    return true;
  };
  auto player_names = [&](const Landed& l) { return l.named_by_player || team_named(l); };
  // ...but the player's OWN view may count its private settle.
  auto player_knows = [&](const Landed& l) {
    if (player_names(l)) return true;
    const Game& g = sim.seat(l.player);
    return l.order < static_cast<int>(g.meta.size()) && !g.meta[l.order].superposed();
  };
  auto common_names = [&](const Landed& l) { return l.named_in_common || team_named(l); };
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
          if (m != l.player) continue;
          bool knows = members.size() == 1 ? player_knows(l) : player_names(l);
          // For a PAIR, the other member must be able to attribute that knowledge
          // to the player: its own common reading named the card too (or the team
          // has since named it). A pair view is one thing both of them compute.
          if (knows && members.size() == 2 && !team_named(l)) {
            const int other = members[0] == m ? members[1] : members[0];
            if (other < static_cast<int>(l.named_at.size()) && !l.named_at[other]) {
              knows = false;
            }
          }
          if (!knows) all = false;
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

  // Views that are one thing by definition, held differently by different seats
  // (informational, class "div"): the common view at every seat, and a pair view
  // at both of its members.
  {
    std::vector<std::string> commons;
    for (int s = 0; s < np_; ++s) {
      std::string c;
      for (int k : sim.seat(s).state.common_play_stacks) c += std::to_string(k);
      commons.push_back(c);
    }
    if (std::any_of(commons.begin(), commons.end(),
                    [&](const std::string& c) { return c != commons[0]; })) {
      add(Issue{"div", "common", turn, -1, -1, json{{"views", commons}}});
      ++div_common_turns_;
    }
    bool pair_div = false;
    for (int a = 0; a < np_; ++a) {
      for (int b = a + 1; b < np_; ++b) {
        const auto& ra = sim.seat(a).state.pairwise_play_stacks;
        const auto& rb = sim.seat(b).state.pairwise_play_stacks;
        if (b < static_cast<int>(ra.size()) && a < static_cast<int>(rb.size()) &&
            ra[b] != rb[a]) {
          pair_div = true;
        }
      }
    }
    if (pair_div) ++div_pair_turns_;
    // Calls: which cards are called to play, and the common reading of each,
    // at every seat that is not the card's holder (the holder may read its own
    // card on its own stacks, §1.3).
    for (int h = 0; h < np_; ++h) {
      for (int c : t.hands[h]) {
        std::vector<std::string> views;
        for (int s2 = 0; s2 < np_; ++s2) {
          if (s2 == h) continue;
          const Game& g2 = sim.seat(s2);
          // The receiver of a pending reactive cannot know which of the reacter's
          // cards is called until the reacter acts (§1d); that is by design.
          bool blind = false;
          for (const auto& w : g2.waiting) {
            if (w.receiver == s2 && w.reacter == h) blind = true;
          }
          for (const auto& w : g2.pending_reactions) {
            if (w && w->receiver == s2 && w->reacter == h) blind = true;
          }
          if (blind) continue;
          views.push_back(g2.meta[c].status == CardStatus::CALLED_TO_PLAY
                              ? set_str(g2.common.thoughts[c].inferred)
                              : "-");
        }
        if (std::any_of(views.begin(), views.end(),
                        [&](const std::string& v) { return v != views[0]; })) {
          add(Issue{"div", "call", turn, h, c, json{{"views", views}}});
        }
      }
    }
    for (int a = 0; a < np_; ++a) {
      for (int b = a + 1; b < np_; ++b) {
        const auto& ra = sim.seat(a).state.pairwise_play_stacks;
        const auto& rb = sim.seat(b).state.pairwise_play_stacks;
        if (b >= static_cast<int>(ra.size()) || a >= static_cast<int>(rb.size())) continue;
        if (ra[b] != rb[a]) {
          add(Issue{"div", "pair", turn, a, b, json{{"view_seat", b}}});
        }
      }
    }
  }
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
  const ConvData& m = g.meta[o.order];
  // Who called it, and did the CALLER's own reading of the card hold the truth?
  json callers = json::object();
  if (m.by) {
    const Game& gb = sim.seat(*m.by);
    callers["by"] = *m.by;
    callers["by_reading"] = set_str(gb.common.thoughts[o.order].inferred);
    callers["by_had_truth"] = gb.common.thoughts[o.order].inferred.contains(*o.id);
  }
  add(Issue{"5", "strike", turn, o.actor, o.order,
            json{{"truth", id_str(*o.id)},
                 {"status", std::string(name(pre_status_[o.actor][o.order]))},
                 {"inferred", set_str(g.common.thoughts[o.order].inferred)},
                 {"urgent", m.urgent},
                 {"signal_turn", m.signal_turn ? *m.signal_turn : -1},
                 {"clued", pre_state_[o.actor].deck[o.order].clued},
                 {"caller", callers},
                 {"belief_rank", pre_state_[o.actor].play_stacks[o.id->suit_index]},
                 {"common_rank", pre_state_[o.actor].common_play_stacks.empty() ? -1 : pre_state_[o.actor].common_play_stacks[o.id->suit_index]},
                 {"true_rank", pre_truth_.stacks[o.id->suit_index]},
                 {"deck_left", static_cast<int>(pre_truth_.deck.size()) - pre_truth_.next_order},
                 {"final_round", pre_truth_.final_turn.has_value()},
                 {"clues", pre_truth_.clues},
                 {"teammates_saw_dead", saw_dead}}});
}

void Diagnostics::finish(const Sim& sim, const SimResult& r) {
  add(Issue{"stat", "divergence", r.turns, -1, -1,
            json{{"common_turns", div_common_turns_},
                 {"pair_turns", div_pair_turns_},
                 {"turns", r.turns}}});
  if (r.error) {
    add(Issue{r.error->kind, "error", r.error->turn + 1, r.error->seat, -1,
              json{{"what", r.error->what}}});
  }
  (void)sim;
}

}  // namespace hanabi::selfplay
