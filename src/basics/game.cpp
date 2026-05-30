#include "hanabi/basics/game.h"

#include <algorithm>
#include <stdexcept>

namespace hanabi {
namespace {

// Insert an action into action_list at the given turn (port of _add_action).
// No-op if the action is already at that turn.
void add_action(std::vector<std::vector<Action>>& action_list, Action action, int turn) {
  if (turn < static_cast<int>(action_list.size())) {
    // De-dupe by equality. Python compares dataclasses by value; std::variant gives us ==.
    for (const auto& a : action_list[turn]) {
      if (a == action) return;
    }
    action_list[turn].push_back(std::move(action));
    return;
  }
  if (turn == static_cast<int>(action_list.size())) {
    action_list.push_back({std::move(action)});
    return;
  }
  throw std::out_of_range("Attempted to add action to a turn beyond action_list");
}

}  // namespace

// --- Factory ---------------------------------------------------------------

Game Game::create(int table_id, State state) {
  Game g;
  g.table_id = table_id;
  auto [players, common] = gen_players(state);
  g.players = std::move(players);
  g.common = std::move(common);
  g.last_actions.assign(state.num_players, std::nullopt);
  g.base = Base{state, {}, g.players, g.common};
  g.state = std::move(state);
  return g;
}

// --- Convention hooks: default no-ops --------------------------------------
// Phase 4 fills these in with reactor logic.

void Game::interpret_clue(const Game&, const ClueAction&) {}
void Game::interpret_discard(const Game&, const DiscardAction&) {}
void Game::interpret_play(const Game&, const PlayAction&) {}
void Game::update_turn(const TurnAction&) {}
void Game::refresh_after_play(const Game&, const PlayAction&) {}
void Game::clean_hypo() {}

std::vector<int> Game::filter_playables(const Player&, int,
                                          const std::vector<int>& orders, bool) const {
  return orders;
}

bool Game::valid_arr(Identity, int) const { return true; }

double Game::eval_action(const Action&) const { return 0.0; }

// --- Generic state updates -------------------------------------------------

void Game::with_state(const std::function<void(State&)>& f) { f(state); }

void Game::with_meta(int order, const std::function<void(ConvData&)>& f) {
  f(meta[order]);
}

void Game::with_card(int order, const std::function<void(Card&)>& f) {
  f(state.deck[order]);
}

void Game::with_thought(int order,
                          const std::function<Thought(const Thought&)>& f) {
  common = common.with_thought(order, f);
}

void Game::with_id(int order, Identity id) {
  state.deck[order].suit_index = id.suit_index;
  state.deck[order].rank = id.rank;
  if (static_cast<int>(deck_ids.size()) <= order) deck_ids.resize(order + 1);
  deck_ids[order] = id;
}

// --- Status predicates -----------------------------------------------------

bool Game::is_touched(int order) const {
  CardStatus status = meta[order].status;
  return state.deck[order].clued || status == CardStatus::CALLED_TO_PLAY ||
         status == CardStatus::GENTLEMANS_DISCARD || status == CardStatus::FINESSED;
}

bool Game::is_blind_playing(int order) const {
  if (state.deck[order].clued) return false;
  const ConvData& m = meta[order];
  return m.status == CardStatus::CALLED_TO_PLAY ||
         m.status == CardStatus::FINESSED || m.bluffed();
}

bool Game::is_saved(int order) const {
  return is_touched(order) || meta[order].cm();
}

bool Game::order_matches(int order, Identity id, bool infer) const {
  if (auto card_id = state.deck[order].id()) return *card_id == id;
  if (order < static_cast<int>(deck_ids.size()) && deck_ids[order]) {
    return *deck_ids[order] == id;
  }
  const Player& me_p = players[state.our_player_index];
  auto t_id = me_p.thoughts[order].id(infer);
  return t_id.has_value() && *t_id == id;
}

bool Game::known_as(int order, std::string_view needle,
                     std::optional<int> special_rank) const {
  IdentitySet possible = common.thoughts[order].possible;
  return possible.forall([&](Identity i) {
    const std::string& suit_name = state.variant->suits[i.suit_index].name;
    if (suit_name.find(needle) != std::string::npos) return true;
    return special_rank.has_value() && i.rank == *special_rank;
  });
}

// --- Action handlers -------------------------------------------------------

void Game::on_clue(const ClueAction& action) {
  const Variant& v = *state.variant;
  IdentitySet new_possible = IdentitySet::create(
      [&](Identity i) { return v.id_touched(i, action.clue.kind, action.clue.value); },
      static_cast<int>(v.suits.size() * 5));
  std::vector<bool> in_touched(state.cards_total, false);
  for (int o : action.list_) {
    if (o < static_cast<int>(in_touched.size())) in_touched[o] = true;
  }

  for (int order : state.hands[action.target]) {
    Thought before = common.thoughts[order];
    if (order < static_cast<int>(in_touched.size()) && in_touched[order]) {
      // Touched: mark clued, intersect inferences.
      with_card(order, [&](Card& c) {
        c.clued = true;
        c.clues.emplace_back(action.clue.kind, action.clue.value, action.giver,
                              state.turn_count);
      });
      with_thought(order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.intersect(new_possible);
        out.possible = t.possible.intersect(new_possible);
        if (t.info_lock) {
          IdentitySet new_lock = t.info_lock->intersect(new_possible);
          out.info_lock = new_lock.is_empty() ? std::optional<IdentitySet>{} : new_lock;
        }
        return out;
      });

      const Thought& after = common.thoughts[order];
      if (after.possible.length() == 1) {
        with_id(order, after.possible.head());
      }
      if (after.inferred.length() < before.inferred.length()) {
        int turn = state.turn_count;
        with_meta(order, [&](ConvData& m) { m = m.reason(turn); });
      }
    } else {
      // Untouched: difference out new_possible.
      with_thought(order, [&](const Thought& t) {
        Thought out = t;
        out.inferred = t.inferred.difference(new_possible);
        out.possible = t.possible.difference(new_possible);
        if (t.info_lock) {
          IdentitySet new_lock = t.info_lock->difference(new_possible);
          out.info_lock = new_lock.is_empty() ? std::optional<IdentitySet>{} : new_lock;
        }
        return out;
      });
    }
  }

  with_state([](State& s) {
    --s.clue_tokens;
    if (s.endgame_turns) --(*s.endgame_turns);
  });
}

namespace {

void drop_from_hand(State& s, int player_index, int order) {
  auto& hand = s.hands[player_index];
  hand.erase(std::remove(hand.begin(), hand.end(), order), hand.end());
}

}  // namespace

void Game::on_discard(const DiscardAction& action) {
  with_state([&](State& s) {
    drop_from_hand(s, action.player_index_v, action.order);
    if (s.endgame_turns) --(*s.endgame_turns);
    if (action.failed) {
      ++s.strikes;
    } else {
      s = s.regain_clue();
    }
  });

  if (action.suit_index != -1 && action.rank != -1) {
    Identity id{action.suit_index, action.rank};
    with_state([&](State& s) { s = s.with_discard(id, action.order); });
    with_id(action.order, id);
    with_thought(action.order, [&](const Thought& t) {
      Thought out = t;
      out.suit_index = id.suit_index;
      out.rank = id.rank;
      out.old_inferred = t.inferred;
      out.old_possible = t.possible;
      out.inferred = IdentitySet::single(id);
      out.possible = IdentitySet::single(id);
      return out;
    });
  }
}

void Game::on_play(const PlayAction& action) {
  with_state([&](State& s) {
    drop_from_hand(s, action.player_index_v, action.order);
    if (s.endgame_turns) --(*s.endgame_turns);
  });

  // Deck-plays edge case: the very last card was played, not drawn.
  const bool deck_plays_edge = state.options.deck_plays &&
                               action.order == state.cards_total - 1 &&
                               static_cast<int>(state.deck.size()) == action.order;
  if (deck_plays_edge) {
    with_state([&](State& s) {
      Card new_card;
      new_card.suit_index = action.suit_index;
      new_card.rank = action.rank;
      new_card.order = action.order;
      new_card.turn_drawn = s.turn_count;
      s.deck.push_back(std::move(new_card));
      s.holders.push_back(action.player_index_v);
      ++s.next_card_order;
      --s.cards_left;
      s.endgame_turns = s.num_players;
    });
    const int order = action.order;
    for (size_t i = 0; i < players.size(); ++i) {
      Player& p = players[i];
      Thought t = Thought::initial(
          static_cast<int>(i) != action.player_index_v ? action.suit_index : -1,
          static_cast<int>(i) != action.player_index_v ? action.rank : -1, order,
          p.all_possible);
      p.thoughts.push_back(std::move(t));
      p.dirty.insert(order);
    }
    common.thoughts.push_back(Thought::initial(-1, -1, order, common.all_possible));
    common.dirty.insert(order);
    ConvData cd;
    cd.order = order;
    meta.push_back(std::move(cd));
  }

  if (action.suit_index != -1 && action.rank != -1) {
    Identity id{action.suit_index, action.rank};
    with_state([&](State& s) { s = s.with_play(id); });
    with_id(action.order, id);
    with_thought(action.order, [&](const Thought& t) {
      Thought out = t;
      out.suit_index = id.suit_index;
      out.rank = id.rank;
      out.old_inferred = t.inferred;
      out.old_possible = t.possible;
      out.inferred = IdentitySet::single(id);
      out.possible = IdentitySet::single(id);
      return out;
    });
  }
}

void Game::on_draw(const DrawAction& action) {
  const int order = action.order;

  if (static_cast<int>(state.hands[action.player_index_v].size()) ==
          kHandSize[state.num_players] &&
      !(state.options.deck_plays && order == state.cards_total - 1)) {
    throw std::runtime_error(state.names[action.player_index_v] +
                             " already has a full hand!");
  }

  std::optional<Identity> id;
  if (action.suit_index != -1 && action.rank != -1) {
    id = Identity(action.suit_index, action.rank);
  }

  if (id && order < static_cast<int>(deck_ids.size()) && deck_ids[order] &&
      *deck_ids[order] != *id) {
    throw std::runtime_error("draw identity does not match expected deck_ids entry");
  }

  if (static_cast<int>(deck_ids.size()) == order) {
    deck_ids.push_back(id);
  } else if (static_cast<int>(deck_ids.size()) > order) {
    if (!deck_ids[order] && id) deck_ids[order] = id;
  } else {
    throw std::runtime_error("deck_ids ran behind drawn card");
  }

  if (!state.options.deck_plays) {
    if (static_cast<int>(state.deck.size()) != order ||
        static_cast<int>(state.deck.size()) != state.next_card_order) {
      throw std::runtime_error("deck size out of sync with drawn order");
    }
  }

  with_state([&](State& s) {
    auto& hand = s.hands[action.player_index_v];
    hand.insert(hand.begin(), order);  // prepend
    Card new_card;
    new_card.suit_index = action.suit_index;
    new_card.rank = action.rank;
    new_card.order = order;
    new_card.turn_drawn = s.turn_count;
    s.deck.push_back(std::move(new_card));
    s.holders.push_back(action.player_index_v);
    s.next_card_order = order + 1;
    --s.cards_left;
    if (s.cards_left == 0 && !s.endgame_turns) s.endgame_turns = s.num_players;
  });

  if (order == static_cast<int>(meta.size())) {
    for (size_t i = 0; i < players.size(); ++i) {
      Player& p = players[i];
      int s_idx = static_cast<int>(i) != action.player_index_v ? action.suit_index : -1;
      int r = static_cast<int>(i) != action.player_index_v ? action.rank : -1;
      p.thoughts.push_back(Thought::initial(s_idx, r, order, p.all_possible));
      p.dirty.insert(order);
    }
    common.thoughts.push_back(Thought::initial(-1, -1, order, common.all_possible));
    common.dirty.insert(order);
    ConvData cd;
    cd.order = order;
    meta.push_back(std::move(cd));
  }
}

// --- Dispatcher ------------------------------------------------------------

void Game::handle_action(const Action& action) {
  if (static_cast<int>(state.action_list.size()) < state.turn_count) {
    throw std::runtime_error("turn_count exceeds action_list length");
  }
  add_action(state.action_list, action, state.turn_count);

  // Snapshot prev for the convention hooks (which take the pre-handler state).
  Game prev = *this;

  std::visit(
      [&](const auto& a) {
        using T = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<T, ClueAction>) {
          on_clue(a);
          interpret_clue(prev, a);
          last_actions[a.giver] = action;
        } else if constexpr (std::is_same_v<T, DiscardAction>) {
          on_discard(a);
          interpret_discard(prev, a);
          last_actions[a.player_index_v] = action;
        } else if constexpr (std::is_same_v<T, PlayAction>) {
          on_play(a);
          interpret_play(prev, a);
          last_actions[a.player_index_v] = action;
        } else if constexpr (std::is_same_v<T, DrawAction>) {
          on_draw(a);
          const int hand_size = kHandSize[state.num_players];
          if (state.turn_count == 0) {
            bool all_full = true;
            for (const auto& h : state.hands) {
              if (static_cast<int>(h.size()) != hand_size) {
                all_full = false;
                break;
              }
            }
            if (all_full) state.turn_count = 1;
          }
          elim();
        } else if constexpr (std::is_same_v<T, GameOverAction>) {
          in_progress = false;
        } else if constexpr (std::is_same_v<T, TurnAction>) {
          state.current_player_index = a.current_player_index;
          state.turn_count = a.num + 1;
          if (a.current_player_index != -1) update_turn(a);
        } else if constexpr (std::is_same_v<T, InterpAction>) {
          next_interp = a.interp;
        }
        // StatusAction / StrikeAction: action_list recording only.
      },
      action);
}

// --- Elim stub (real impl arrives with player_elim in Phase 2b) -----------

void Game::elim(std::optional<int>) {
  // Stub: just clear dirty so dependent helpers don't spin.
  for (auto& p : players) p.dirty.clear();
  common.dirty.clear();
}

// --- Simulation ------------------------------------------------------------

Game Game::simulate_action(const Action& action, std::optional<Identity> draw) const {
  Game g = *this;
  g.catchup = true;

  const int player_index = hanabi::player_index(action);

  if (!g.state.action_list.empty() && !g.state.action_list.back().empty()) {
    const Action& last = g.state.action_list.back().back();
    if (!std::holds_alternative<TurnAction>(last)) {
      g.handle_action(TurnAction{g.state.turn_count, player_index});
    }
  }
  g.handle_action(action);
  if (hanabi::requires_draw(action) && g.state.cards_left > 0) {
    int order = g.state.next_card_order;
    std::optional<Identity> next_id;
    if (order < static_cast<int>(g.deck_ids.size()) && g.deck_ids[order]) {
      next_id = g.deck_ids[order];
    } else if (draw) {
      next_id = draw;
    }
    if (next_id) {
      g.handle_action(DrawAction{player_index, order, next_id->suit_index, next_id->rank});
    } else {
      g.handle_action(DrawAction{player_index, order, -1, -1});
    }
  }
  g.handle_action(TurnAction{g.state.turn_count, player_index});
  g.catchup = false;
  return g;
}

Game Game::simulate_clue(const ClueAction& action, bool free) const {
  Game g = *this;
  g.catchup = true;
  add_action(g.state.action_list, action, g.state.turn_count);
  if (free) ++g.state.clue_tokens;

  Game prev = g;
  g.on_clue(action);
  g.interpret_clue(prev, action);
  g.last_actions[action.giver] = action;
  ++g.state.turn_count;
  g.catchup = false;
  return g;
}

}  // namespace hanabi
