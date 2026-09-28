#include "sim.h"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <random>
#include <utility>

#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/conventions/reactor0/efficiency.h"
#include "hanabi/instrumentation/timer.h"
#include "hanabi/logging/state_snapshot.h"
#include "hanabi/version.h"

namespace hanabi::selfplay {

using nlohmann::json;

// --- Truth -----------------------------------------------------------------

int TrueState::next_rank(const Variant& v, int suit) const {
  const int played = stacks[suit];
  if (played >= 5) return 0;
  return v.suits[suit].suit_type.reversed ? 5 - played : played + 1;
}

bool TrueState::playable(const Variant& v, Identity id) const {
  return next_rank(v, id.suit_index) == id.rank;
}

int TrueState::max_score(const Variant& v) const {
  // Per suit, how far the stack can still get: walk the ranks from the next one
  // and stop at the first identity every copy of which is gone.
  int total = 0;
  for (int k = 0; k < static_cast<int>(v.suits.size()); ++k) {
    int reach = stacks[k];
    const bool rev = v.suits[k].suit_type.reversed;
    for (int step = stacks[k]; step < 5; ++step) {
      const Identity id{k, rev ? 5 - step : step + 1};
      int gone = 0;
      for (std::size_t o = 0; o < deck.size(); ++o) {
        if (deck[o] == id && discarded[o]) ++gone;
      }
      if (gone >= v.card_count(id)) break;
      reach = step + 1;
    }
    total += reach;
  }
  return total;
}

int hand_size_for(int num_players) {
  if (num_players <= 3) return 5;
  if (num_players <= 5) return 4;
  return 3;
}

std::vector<Identity> build_deck(const Variant& variant, std::uint64_t seed) {
  std::vector<Identity> deck;
  for (Identity id : variant.all_ids()) {
    for (int c = 0; c < variant.card_count(id); ++c) deck.push_back(id);
  }
  std::mt19937_64 rng(seed);
  // Fisher-Yates with our own index draw: `std::shuffle` is implementation
  // defined, and a seed has to mean the same deck on every toolchain.
  for (std::size_t i = deck.size(); i > 1; --i) {
    const std::size_t j = static_cast<std::size_t>(rng() % i);
    std::swap(deck[i - 1], deck[j]);
  }
  return deck;
}

// --- Wire ------------------------------------------------------------------

namespace {

json draw_json(int player, int order, std::optional<Identity> id) {
  return json{{"type", "draw"},
              {"playerIndex", player},
              {"order", order},
              {"suitIndex", id ? id->suit_index : -1},
              {"rank", id ? id->rank : -1}};
}

}  // namespace

std::vector<std::vector<json>> deal_messages(const std::vector<Identity>& deck,
                                             int num_players, int hand_size) {
  std::vector<std::vector<json>> out(num_players);
  int order = 0;
  for (int p = 0; p < num_players; ++p) {
    for (int i = 0; i < hand_size; ++i, ++order) {
      for (int seat = 0; seat < num_players; ++seat) {
        out[seat].push_back(draw_json(
            p, order, seat == p ? std::nullopt : std::optional<Identity>(deck[order])));
      }
    }
  }
  return out;
}

std::vector<std::vector<json>> wire_messages(const Variant& variant, int num_players,
                                             const WireEvent& ev) {
  const bool hole = variant.throw_it_in_a_hole;
  const Outcome& o = ev.outcome;
  std::vector<std::vector<json>> out(num_players);
  for (int seat = 0; seat < num_players; ++seat) {
    auto& m = out[seat];
    switch (o.kind) {
      case Outcome::Kind::CLUE:
        m.push_back(json{{"type", "clue"},
                         {"clue",
                          {{"type", o.clue_kind == ClueKind::COLOUR ? 0 : 1},
                           {"value", o.clue_value}}},
                         {"giver", o.actor},
                         {"list", o.touched},
                         {"target", o.clue_target},
                         {"turn", o.turn}});
        break;
      case Outcome::Kind::PLAY_LANDED:
      case Outcome::Kind::PLAY_MISSED: {
        const bool landed = o.kind == Outcome::Kind::PLAY_LANDED;
        if (hole) {
          // Nobody, the player included, is told what it was or whether it
          // landed.
          m.push_back(json{{"type", "play"},
                           {"playerIndex", o.actor},
                           {"order", o.order},
                           {"suitIndex", -1},
                           {"rank", -1}});
        } else if (landed) {
          m.push_back(json{{"type", "play"},
                           {"playerIndex", o.actor},
                           {"order", o.order},
                           {"suitIndex", o.id->suit_index},
                           {"rank", o.id->rank}});
        } else {
          m.push_back(json{{"type", "strike"},
                           {"num", ev.strikes_after},
                           {"order", o.order},
                           {"turn", o.turn}});
          m.push_back(json{{"type", "discard"},
                           {"playerIndex", o.actor},
                           {"order", o.order},
                           {"suitIndex", o.id->suit_index},
                           {"rank", o.id->rank},
                           {"failed", true}});
        }
        break;
      }
      case Outcome::Kind::DISCARD:
        m.push_back(json{{"type", "discard"},
                         {"playerIndex", o.actor},
                         {"order", o.order},
                         {"suitIndex", o.id->suit_index},
                         {"rank", o.id->rank},
                         {"failed", false}});
        break;
    }
    if (o.drawn_order) {
      m.push_back(draw_json(o.actor, *o.drawn_order,
                            seat == o.actor ? std::nullopt : ev.drawn_id));
    }
    m.push_back(json{{"type", "status"},
                     {"clues", ev.clues_after},
                     {"score", hole ? 0 : ev.score_after},
                     {"maxScore", hole ? 25 : ev.max_score_after}});
    if (ev.game_over) {
      m.push_back(json{{"type", "gameOver"},
                       {"endCondition", static_cast<int>(*ev.game_over)},
                       {"playerIndex", -1}});
    } else {
      m.push_back(json{{"type", "turn"},
                       {"num", ev.next_turn_num},
                       {"currentPlayerIndex", ev.next_player}});
    }
  }
  return out;
}

// --- Sim -------------------------------------------------------------------

Sim::Sim(SimConfig cfg) : cfg_(std::move(cfg)) {
  variant_ = &get_variant(cfg_.variant);
  static const char* kNames[] = {"sim-alice", "sim-bob",  "sim-cathy",
                                 "sim-donald", "sim-emily", "sim-frank"};
  for (int p = 0; p < cfg_.num_players; ++p) names_.push_back(kNames[p]);

  truth_.deck = build_deck(*variant_, cfg_.seed);
  truth_.holder.assign(truth_.deck.size(), -1);
  truth_.landed_turn.assign(truth_.deck.size(), -1);
  truth_.discarded.assign(truth_.deck.size(), false);
  truth_.stacks.assign(variant_->suits.size(), 0);
  truth_.hands.assign(cfg_.num_players, {});

  const int np = cfg_.num_players;
  for (int p = 0; p < np; ++p) {
    TableOptions opts;
    opts.num_players = np;
    opts.variant_name = cfg_.variant;
    State s = State::create(names_, p, *variant_, opts);
    auto game = std::make_unique<Game>(Game::create(cfg_.game_id, std::move(s)));
    // As `BotClient::on_init` configures a live table.
    game->in_progress = true;
    game->catchup = false;
    game->convention =
        resolve_table_convention(variant_->throw_it_in_a_hole, np, cfg_.convention_mode);
    game->all_plays = false;
    game->allow_reactive_locks =
        hanabi::reactor0::default_allow_reactive_locks(*variant_, np);
    game->endgame_timeout = cfg_.endgame_timeout;
    games_.push_back(std::move(game));

    if (!cfg_.log_dir.empty()) {
      // The logger appends, so a rerun of the same seed must start clean.
      const std::string path =
          logging::GameLogger::log_path(names_[p], cfg_.game_id, cfg_.log_dir);
      std::error_code ec;
      std::filesystem::remove(path, ec);
      auto logger =
          std::make_shared<logging::GameLogger>(names_[p], cfg_.game_id, cfg_.log_dir);
      logger->emit_lifecycle(
          "game_init",
          json{{"variant", cfg_.variant},
               {"num_players", np},
               {"our_player_index", p},
               {"names", names_},
               {"is_replay", false},
               {"bot_version", kBotVersion},
               {"all_plays", false},
               {"convention", std::string(convention_name(games_[p]->convention))},
               {"rlocks", games_[p]->allow_reactive_locks},
               {"self_play", true},
               {"seed", cfg_.seed}});
      log_paths_.push_back(logger->path());
      loggers_.push_back(std::move(logger));
    } else {
      log_paths_.emplace_back();
      loggers_.push_back(nullptr);
    }
  }
}

Sim::~Sim() = default;

void Sim::feed(int p, const json& wire) {
  auto act = action_from_json(wire);
  if (!act) return;
  Game& g = *games_[p];
  act = orient_action_for_engine(*act, *g.state.variant);
  if (loggers_[p]) {
    loggers_[p]->emit_lifecycle(
        "inbound_action",
        json{{"turn", g.state.turn_count},
             {"action", logging::action_to_internal_json(*act)}});
  }
  g.handle_action(*act);
}

void Sim::feed_all(const std::vector<std::vector<json>>& per_seat) {
  for (int p = 0; p < static_cast<int>(per_seat.size()); ++p) {
    for (const json& w : per_seat[p]) feed(p, w);
  }
}

std::optional<Outcome> Sim::resolve(int actor, const PerformAction& perform) {
  TrueState& t = truth_;
  const Variant& v = *variant_;
  Outcome o;
  o.actor = actor;
  o.turn = t.turn;
  auto fail = [&](const std::string& what) {
    error_ = SimError{"harness_error", t.turn, actor, what};
    return std::nullopt;
  };

  if (auto* c = std::get_if<PerformColour>(&perform)) {
    o.kind = Outcome::Kind::CLUE;
    o.clue_target = c->target;
    o.clue_kind = ClueKind::COLOUR;
    o.clue_value = c->value;
  } else if (auto* r = std::get_if<PerformRank>(&perform)) {
    o.kind = Outcome::Kind::CLUE;
    o.clue_target = r->target;
    o.clue_kind = ClueKind::RANK;
    o.clue_value = r->value;
  }
  if (o.kind == Outcome::Kind::CLUE && (std::holds_alternative<PerformColour>(perform) ||
                                        std::holds_alternative<PerformRank>(perform))) {
    if (t.clues <= 0) return fail("clue with no clue tokens");
    if (o.clue_target == actor || o.clue_target < 0 ||
        o.clue_target >= cfg_.num_players) {
      return fail("clue to an invalid seat " + std::to_string(o.clue_target));
    }
    for (int ord : t.hands[o.clue_target]) {
      if (v.id_touched(t.deck[ord], o.clue_kind, o.clue_value)) o.touched.push_back(ord);
    }
    const bool touch_nothing = o.clue_kind == ClueKind::COLOUR
                                   ? v.colour_clues_touch_nothing
                                   : v.rank_clues_touch_nothing;
    if (o.touched.empty() && !touch_nothing) return fail("clue touches no card");
    --t.clues;
    return o;
  }

  int order = -1;
  bool pressed_play = false;
  if (auto* p = std::get_if<PerformPlay>(&perform)) {
    order = p->target;
    pressed_play = true;
  } else if (auto* d = std::get_if<PerformDiscard>(&perform)) {
    order = d->target;
  } else {
    return fail("unknown action");
  }
  auto& hand = t.hands[actor];
  auto it = std::find(hand.begin(), hand.end(), order);
  if (it == hand.end()) return fail("order " + std::to_string(order) + " not in hand");
  if (!pressed_play && t.clues >= 8) return fail("discard at 8 clue tokens");
  hand.erase(it);

  const Identity id = t.deck[order];
  o.order = order;
  o.id = id;
  o.pressed_play = pressed_play;
  // An inverted suit swaps the buttons: Play throws it away (a pitch), Discard
  // attempts to play it (a chuck).
  const bool attempts_play = pressed_play != v.suits[id.suit_index].suit_type.inverted;
  if (attempts_play) {
    if (t.playable(v, id)) {
      o.kind = Outcome::Kind::PLAY_LANDED;
      ++t.stacks[id.suit_index];
      ++t.score;
      t.landed_turn[order] = t.turn;
      const bool completes = t.stacks[id.suit_index] == 5;
      if (completes && !v.throw_it_in_a_hole && t.clues < 8) ++t.clues;
    } else {
      o.kind = Outcome::Kind::PLAY_MISSED;
      ++t.strikes;
      t.discarded[order] = true;
    }
  } else {
    o.kind = Outcome::Kind::DISCARD;
    t.discarded[order] = true;
    if (t.clues < 8) ++t.clues;
  }
  return o;
}

SimResult Sim::run(const SimHooks& hooks) {
  TrueState& t = truth_;
  const Variant& v = *variant_;
  const int np = cfg_.num_players;
  const int hs = hand_size_for(np);

  // The deal.
  const auto deal = deal_messages(t.deck, np, hs);
  for (int p = 0; p < np; ++p) {
    for (int i = 0; i < hs; ++i) {
      const int ord = p * hs + i;
      t.hands[p].insert(t.hands[p].begin(), ord);  // newest card is slot 1
      t.holder[ord] = p;
    }
  }
  t.next_order = np * hs;
  try {
    feed_all(deal);
  } catch (const std::exception& e) {
    error_ = SimError{"crash", 0, -1, std::string("deal: ") + e.what()};
  }

  const int max_possible = 5 * static_cast<int>(v.suits.size());
  while (!error_ && !t.ended) {
    if (t.turn >= cfg_.max_turns) {
      error_ = SimError{"harness_error", t.turn, t.current, "turn limit reached"};
      break;
    }
    if (hooks.before_action) hooks.before_action(*this);

    const int actor = t.current;
    Game& g = *games_[actor];
    PerformAction perform;
    {
      auto* glog = loggers_[actor].get();
      logging::CurrentLoggerGuard guard(glog);
      if (glog) {
        glog->mark_turn_start();
        logging::emit_state_snapshot(*glog, g, g.state.turn_count);
        glog->emit_lifecycle("decide_start", json{{"turn", g.state.turn_count}});
      }
      try {
        instr::ScopedTimer st("take_action");
        perform = g.take_action();
      } catch (const std::exception& e) {
        error_ = SimError{"crash", t.turn, actor, std::string("take_action: ") + e.what()};
        if (glog) {
          glog->emit_lifecycle("take_action_error",
                               json{{"turn", g.state.turn_count}, {"error", e.what()}});
        }
        break;
      }
      if (glog) {
        glog->emit_lifecycle("outbound_action",
                             json{{"turn", g.state.turn_count},
                                  {"action", to_json(perform, cfg_.game_id)}});
      }
    }

    auto outcome = resolve(actor, perform);
    if (!outcome) break;

    // The draw.
    std::optional<Identity> drawn_id;
    if (outcome->kind != Outcome::Kind::CLUE &&
        t.next_order < static_cast<int>(t.deck.size())) {
      const int ord = t.next_order++;
      t.hands[actor].insert(t.hands[actor].begin(), ord);
      t.holder[ord] = actor;
      outcome->drawn_order = ord;
      drawn_id = t.deck[ord];
      if (t.next_order == static_cast<int>(t.deck.size())) {
        // The last card: every seat, this one included, gets one more turn.
        t.final_turn = t.turn + np + 1;
      }
    }

    ++t.turn;
    t.current = (t.current + 1) % np;
    std::optional<EndCondition> over;
    if (t.strikes >= 3) over = EndCondition::STRIKEOUT;
    else if (t.score >= max_possible) over = EndCondition::NORMAL;
    else if (t.final_turn && t.turn >= *t.final_turn) over = EndCondition::NORMAL;

    WireEvent ev;
    ev.outcome = *outcome;
    ev.drawn_id = drawn_id;
    ev.strikes_after = t.strikes;
    ev.clues_after = t.clues;
    ev.score_after = t.score;
    ev.max_score_after = t.max_score(v);
    ev.next_turn_num = t.turn;
    ev.next_player = t.current;
    ev.game_over = over;
    try {
      feed_all(wire_messages(v, np, ev));
    } catch (const std::exception& e) {
      error_ = SimError{"crash", t.turn - 1, -1, std::string("handle_action: ") + e.what()};
      break;
    }
    if (over) t.ended = over;
    if (hooks.after_action) hooks.after_action(*this, *outcome);
  }

  SimResult r;
  r.seed = cfg_.seed;
  r.game_id = cfg_.game_id;
  r.score = t.score;
  r.max_score = max_possible;
  r.strikes = t.strikes;
  r.turns = t.turn;
  r.end = t.ended.value_or(EndCondition::TERMINATED);
  r.error = error_;
  for (int p = 0; p < np; ++p) {
    if (!loggers_[p]) continue;
    loggers_[p]->emit_lifecycle(
        "game_over", json{{"end_condition", static_cast<int>(r.end)},
                          {"true_score", r.score},
                          {"true_strikes", r.strikes}});
    auto snap = loggers_[p]->aggregator().snapshot();
    loggers_[p]->emit(json{{"ch", "TIMING"},
                           {"scope", "per_game"},
                           {"scopes", instr::Aggregator::to_json(snap)}});
  }
  return r;
}

}  // namespace hanabi::selfplay
