#include "hanabi/basics/game.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "hanabi/basics/fix.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/player.h"
#include "hanabi/basics/player_elim.h"
#include "hanabi/basics/sarcastic.h"
#include "hanabi/conventions/reactor/interpret_clue.h"
#include "hanabi/conventions/reactor/interpret_reaction.h"
#include "hanabi/conventions/reactor/state_eval.h"
#include "hanabi/endgame/fraction.h"
#include "hanabi/endgame/solver.h"

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

// --- Convention hooks: reactor implementation ----------------------------

void Game::interpret_clue(const Game& prev, const ClueAction& action) {
  using namespace hanabi::reactor;
  check_missed(action.giver, /*sentinel=*/99);

  if (!waiting.empty() && waiting.front().reacter == action.giver) {
    waiting.clear();
  }

  std::optional<ClueInterp> interp;

  if (next_interp) {
    if (std::holds_alternative<ClueInterp>(*next_interp)) {
      ClueInterp forced = std::get<ClueInterp>(*next_interp);
      if (forced == ClueInterp::REACTIVE) {
        int reacter = state.next_player_index(action.giver);
        interp = interpret_reactive(prev, *this, action, reacter,
                                       /*looks_stable=*/true);
      } else {
        interp = interpret_stable(prev, *this, action, /*stall=*/false);
      }
    }
  } else if (state.options.empty_clues && action.list_.empty()) {
    interp = ClueInterp::USELESS;
  } else if (prev.common.obvious_locked(prev, action.giver) || in_endgame() ||
              prev.state.clue_tokens == 8) {
    int bob = state.next_player_index(action.giver);
    int cathy = state.next_player_index(bob);
    bool target_is_cathy = action.target == cathy && cathy != action.giver;
    bool bob_unloaded = target_is_cathy &&
                          prev.common.obvious_playables(prev, bob).empty();
    if (bob_unloaded) {
      interp = interpret_reactive(prev, *this, action, /*reacter=*/bob,
                                     /*looks_stable=*/false);
    } else {
      interp = interpret_stable(prev, *this, action, /*stall=*/true);
    }
  } else {
    std::optional<int> reacter;
    for (int i = 1; i < state.num_players; ++i) {
      int pi = (action.giver + i) % state.num_players;
      auto old_play = prev.common.obvious_playables(prev, pi);
      auto new_play = common.obvious_playables(*this, pi);
      bool any_kept = false;
      for (int o : old_play) {
        if (std::find(new_play.begin(), new_play.end(), o) != new_play.end()) {
          any_kept = true;
          break;
        }
      }
      if (!any_kept) {
        reacter = pi;
        break;
      }
    }

    FixResult fr = check_fix(prev, *this, action);
    std::vector<int> fixed;
    if (std::holds_alternative<FixResultNormal>(fr)) {
      const auto& f = std::get<FixResultNormal>(fr);
      fixed.insert(fixed.end(), f.clued_resets.begin(), f.clued_resets.end());
      fixed.insert(fixed.end(), f.duplicate_reveals.begin(),
                    f.duplicate_reveals.end());
    }
    bool allowable_fix = action.target == state.next_player_index(action.giver) &&
                          !fixed.empty();

    if (!reacter) {
      interp = allowable_fix ? std::make_optional(ClueInterp::FIX) : std::nullopt;
    } else if (*reacter == action.target) {
      interp = interpret_stable(prev, *this, action, /*stall=*/false);
    } else {
      auto prev_playables =
          prev.players[action.target].obvious_playables(prev, action.target);
      bool any_in_prev = false;
      for (int o : fixed) {
        if (std::find(prev_playables.begin(), prev_playables.end(), o) !=
            prev_playables.end()) {
          any_in_prev = true;
          break;
        }
      }
      if (allowable_fix && any_in_prev) {
        interp = ClueInterp::FIX;
      } else {
        interp = interpret_reactive(prev, *this, action, *reacter,
                                       /*looks_stable=*/false);
      }
    }
  }

  if (!interp) interp = ClueInterp::MISTAKE;
  with_move(*interp);

  // Count newly-signalled plays before elim.
  std::vector<int> signalled_plays;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (prev.meta[o].status != CardStatus::CALLED_TO_PLAY &&
          meta[o].status == CardStatus::CALLED_TO_PLAY) {
        signalled_plays.push_back(o);
      }
    }
  }
  elim();
  std::vector<int> plays_after;
  for (const auto& hand : state.hands) {
    for (int o : hand) {
      if (meta[o].status == CardStatus::CALLED_TO_PLAY) plays_after.push_back(o);
    }
  }
  if (plays_after.size() < signalled_plays.size()) {
    with_move(ClueInterp::MISTAKE, /*overwrite=*/true);
  }
  if (prev.state.can_clue()) reset_zcs();
  if (!state.can_clue()) zcs_turn = state.turn_count;
  next_interp = std::nullopt;
}

void Game::interpret_discard(const Game& prev, const DiscardAction& action) {
  using namespace hanabi::reactor;
  check_missed(action.player_index_v, action.order);

  bool failed = action.failed;
  std::optional<Identity> id;
  if (action.suit_index != -1 && action.rank != -1) {
    id = Identity(action.suit_index, action.rank);
  }

  if (failed) {
    // Bombed - clear all conv info.
    for (const auto& hand : state.hands) {
      for (int o : hand) {
        with_thought(o, [](const Thought& t) {
          Thought out = t;
          out.inferred = t.possible;
          out.old_inferred = std::nullopt;
          out.info_lock = std::nullopt;
          return out;
        });
        with_meta(o, [](ConvData& m) { m = m.cleared(); });
      }
    }
    waiting.clear();
  }

  bool useful_dc = !failed && prev.state.deck[action.order].clued && id.has_value() &&
                    state.is_useful(*id) &&
                    prev.meta[action.order].status != CardStatus::CALLED_TO_DISCARD &&
                    !(prev.common.thinks_locked(prev, action.player_index_v) &&
                      prev.state.clue_tokens == 0);

  if (!waiting.empty()) {
    react_discard(prev, *this, action.player_index_v, action.order, waiting.front());
  } else if (useful_dc && id) {
    DiscardResult dc_result = interpret_useful_dc(*this, action);
    if (std::holds_alternative<DiscardResultNone>(dc_result)) {
      with_move(DiscardInterp::NONE);
    } else if (std::holds_alternative<DiscardResultMistake>(dc_result)) {
      with_move(DiscardInterp::MISTAKE);
    } else if (std::holds_alternative<DiscardResultGentlemansDiscard>(dc_result)) {
      const auto& gd = std::get<DiscardResultGentlemansDiscard>(dc_result);
      State hypo_state = state;
      for (size_t k = 0; k < gd.orders.size(); ++k) {
        int o = gd.orders[k];
        bool hidden = k + 1 != gd.orders.size();
        IdentitySet inferred = hidden ? hypo_state.playable_set : IdentitySet::single(*id);
        auto me_id = me().thoughts[o].id();
        if (me_id) hypo_state = hypo_state.with_play(*me_id);
        IdentitySet inf_copy = inferred;
        with_thought(o, [inf_copy](const Thought& t) {
          Thought out = t;
          out.inferred = inf_copy;
          return out;
        });
        with_meta(o, [hidden](ConvData& m) {
          m.status = CardStatus::GENTLEMANS_DISCARD;
          m.hidden = hidden;
        });
      }
      with_move(DiscardInterp::GENTLEMANS_DISCARD);
    } else if (std::holds_alternative<DiscardResultSarcastic>(dc_result)) {
      const auto& s = std::get<DiscardResultSarcastic>(dc_result);
      SarcasticLink new_link{s.orders, *id};
      common.links.insert(common.links.begin(), Link{std::move(new_link)});
      with_move(DiscardInterp::SARCASTIC);
    }
  } else {
    with_move(DiscardInterp::NONE);
  }

  elim();
  if (prev.state.can_clue()) reset_zcs();
}

void Game::interpret_play(const Game& prev, const PlayAction& action) {
  using namespace hanabi::reactor;
  // reinterp_play (rewind support) isn't ported; skip it for now.
  check_missed(action.player_index_v, action.order);
  if (!waiting.empty()) {
    react_play(prev, *this, action.player_index_v, action.order, waiting.front());
  }
  with_move(PlayInterp::NONE, /*overwrite=*/true);
  elim();
  if (prev.state.can_clue()) reset_zcs();
}

void Game::update_turn(const TurnAction& action) {
  int cpi = action.current_player_index;
  if (cpi == -1) return;

  std::optional<int> next_queued_playable;
  int best_signal = 99;
  for (int o : state.hands[cpi]) {
    if (meta[o].status == CardStatus::CALLED_TO_PLAY &&
        !common.thoughts[o].id(/*infer=*/true)) {
      int s = meta[o].signal_turn.value_or(99);
      if (!next_queued_playable || s < best_signal) {
        next_queued_playable = o;
        best_signal = s;
      }
    }
  }

  if (!waiting.empty() &&
      waiting.front().reacter == state.last_player_index(cpi)) {
    waiting.clear();
  }

  if (next_queued_playable) {
    int order = *next_queued_playable;
    IdentitySet new_inferred =
        common.thoughts[order].inferred.intersect(state.playable_set);
    if (new_inferred.is_empty()) {
      with_thought(order, [](const Thought& t) { return t.reset_inferences(); });
      with_meta(order, [](ConvData& m) {
        m.status = CardStatus::NONE;
        m.by = std::nullopt;
        m.trash = true;
      });
    } else {
      IdentitySet ni = new_inferred;
      with_thought(order, [ni](const Thought& t) {
        Thought out = t;
        out.inferred = ni;
        return out;
      });
    }
  }
  elim();
}

void Game::refresh_after_play(const Game&, const PlayAction&) {}
void Game::clean_hypo() {}

std::vector<int> Game::filter_playables(const Player&, int,
                                          const std::vector<int>& orders, bool) const {
  return orders;
}

bool Game::valid_arr(Identity id, int order) const {
  // Reactor: respect info_lock when assigning identities to unknown cards.
  // Port of conventions/reactor/reactor.py: Reactor.valid_arr.
  const auto& lock = me().thoughts[order].info_lock;
  return !lock || lock->contains(id);
}

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

void Game::with_move(const Interp& interp, bool overwrite) {
  int player_actions = 0;
  for (const auto& turn : state.action_list) {
    for (const auto& a : turn) {
      if (hanabi::is_player_action(a)) ++player_actions;
    }
  }
  if (!overwrite) {
    if (static_cast<int>(move_history.size()) == player_actions) {
      throw std::runtime_error("trying to add move to full move_history");
    }
    move_history.push_back(interp);
    return;
  }
  if (static_cast<int>(move_history.size()) < player_actions) {
    move_history.push_back(interp);
    return;
  }
  move_history.back() = interp;
}

void Game::check_missed(int player_index, int action_order) {
  std::optional<int> urgent_order;
  for (int o : state.hands[player_index]) {
    if (meta[o].urgent && o != action_order) {
      urgent_order = o;
      break;
    }
  }
  if (!urgent_order) return;
  int uo = *urgent_order;
  with_thought(uo, [](const Thought& t) {
    if (!t.old_inferred) {
      throw std::runtime_error("check_missed: no old_inferred on urgent card");
    }
    Thought out = t;
    out.inferred = *t.old_inferred;
    out.old_inferred = std::nullopt;
    out.info_lock = std::nullopt;
    return out;
  });
  int turn = state.turn_count;
  with_meta(uo, [turn](ConvData& m) { m = m.cleared().reason(turn); });
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

// --- Empathy elim (port of game.py:613-710) -------------------------------

void Game::elim(std::optional<int> except_) {
  // Step 1: pre-elim cleanup.
  for (const auto& hand : state.hands) {
    for (int order : hand) {
      const Thought& thought = common.thoughts[order];
      if (thought.inferred.is_empty() && !thought.reset) {
        with_thought(order, [](const Thought& t) { return t.reset_inferences(); });
        with_meta(order, [](ConvData& m) {
          m.status = CardStatus::NONE;
          m.by = std::nullopt;
        });
      }
      const Thought& updated = common.thoughts[order];
      if (updated.info_lock && updated.info_lock->is_empty()) {
        with_thought(order, [](const Thought& t) {
          Thought out = t;
          out.info_lock = std::nullopt;
          return out;
        });
      }
    }
  }

  // Step 2: card_elim + (optional) good_touch_elim on common.
  auto [resets, new_common] = card_elim(std::move(common), state);
  if (good_touch) {
    auto [gt_resets, gt_common] = good_touch_elim(std::move(new_common), *this, except_);
    new_common = std::move(gt_common);
    for (int o : gt_resets) resets.insert(o);
  }
  common = std::move(new_common);
  for (int order : resets) {
    if (meta[order].status == CardStatus::CALLED_TO_PLAY) {
      with_meta(order, [](ConvData& m) {
        m.status = CardStatus::NONE;
        m.by = std::nullopt;
      });
    }
  }

  // Step 3: refresh_links + refresh_play_links + update_hypo_stacks on common.
  auto [sarcastics, post_links] = refresh_links(std::move(common), *this);
  post_links = refresh_play_links(std::move(post_links), *this);
  common = std::move(post_links);
  common = common.update_hypo_stacks(*this);
  for (int order : sarcastics) {
    with_meta(order, [](ConvData& m) { m.status = CardStatus::SARCASTIC; });
  }

  // Step 4: sync each per-player perspective from common, re-run elim per-player.
  for (size_t pi = 0; pi < players.size(); ++pi) {
    Player p = players[pi];
    for (int o : common.dirty) {
      Thought& t = p.thoughts[o];
      const Thought& c_t = common.thoughts[o];
      IdentitySet new_inferred =
          c_t.inferred.intersect(t.possible).when_empty(t.possible);
      std::optional<IdentitySet> new_info_lock;
      if (c_t.info_lock) {
        IdentitySet ids = c_t.info_lock->intersect(t.possible);
        if (!ids.is_empty()) new_info_lock = ids;
      }
      t.possible = c_t.possible;
      t.inferred = new_inferred;
      t.info_lock = new_info_lock;
      t.reset = c_t.reset;
    }
    p.links = common.links;
    p.play_links = common.play_links;
    p.dirty = common.dirty;

    auto [_, after_card] = card_elim(std::move(p), state);
    p = std::move(after_card);
    if (good_touch) {
      auto [_unused, after_gt] = good_touch_elim(std::move(p), *this, except_);
      p = std::move(after_gt);
    }
    auto [_s, after_refresh] = refresh_links(std::move(p), *this);
    p = std::move(after_refresh);
    p = refresh_play_links(std::move(p), *this);
    p = p.update_hypo_stacks(*this);
    p.dirty.clear();
    players[pi] = std::move(p);
  }

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

// --- Reactor helpers (faithful port of reactor.scala chop/hasPtd) --------

// Reactor's `in_endgame` is one turn earlier than the Game default. Since we
// don't subclass, we check whether the game has reactor data and adjust.
bool Game::in_endgame() const {
  // Reactor signals presence via waiting/zcs_turn fields existing on Game; the
  // conditional below matches Reactor's override of `in_endgame`.
  return state.pace() < state.num_players - 1;
}

std::optional<int> Game::chop(int player_index) const {
  // First pass: explicit CalledToDiscard.
  for (int o : state.hands[player_index]) {
    if (meta[o].status == CardStatus::CALLED_TO_DISCARD) return o;
  }
  // Second pass: newest unclued + status NONE, gated on zcs_turn.
  for (int o : state.hands[player_index]) {
    bool zcs_ok = zcs_turn == -1 || zcs_turn >= state.deck[o].turn_drawn;
    if (zcs_ok && !state.deck[o].clued && meta[o].status == CardStatus::NONE) {
      return o;
    }
  }
  return std::nullopt;
}

bool Game::has_ptd() const {
  int player_index = state.current_player_index;
  int zelda = state.last_player_index(player_index);
  int bob = state.next_player_index(player_index);
  auto bob_chop = chop(bob);

  std::optional<Identity> bob_chop_id;
  if (bob_chop) bob_chop_id = state.deck[*bob_chop].id();

  auto known_dupe = [&]() -> bool {
    if (!bob_chop_id) return false;
    for (int o : state.hands[bob]) {
      if (o == *bob_chop) continue;
      if (players[zelda].thoughts[o].matches(*bob_chop_id) &&
          me().thoughts[o].matches(*bob_chop_id)) {
        return true;
      }
    }
    return false;
  };

  auto unknown_play = [&]() -> bool {
    if (!last_actions[zelda]) return false;
    const Action& last = *last_actions[zelda];
    if (!std::holds_alternative<PlayAction>(last)) return false;
    if (!bob_chop_id) return false;
    const auto& pa = std::get<PlayAction>(last);
    Identity played_id{pa.suit_index, pa.rank};
    if (played_id != *bob_chop_id) return false;
    const auto& old_inf = common.thoughts[pa.order].old_inferred;
    return !old_inf || *old_inf != IdentitySet::single(played_id);
  };

  if (common.obvious_loaded(*this, bob)) return true;
  if (bob_chop_id && state.is_critical(*bob_chop_id)) return false;
  if (bob_chop_id && state.is_basic_trash(*bob_chop_id)) return !unknown_play();
  if (known_dupe()) return true;
  return !(bob_chop_id &&
            (state.is_playable(*bob_chop_id) || bob_chop_id->rank == 2));
}

// find_all_clues: enumerate clue candidates, filtering out mistakes and
// useless duplicates. Mirrors reactor.py find_all_clues — we evaluate each
// candidate by simulating it, drop MISTAKE interpretations, and rank
// surviving useful clues by get_result score. At most one representative
// "useless" clue is included so the solver still has a stall option.
std::vector<PerformAction> Game::find_all_clues(int giver) const {
  std::vector<PerformAction> out;
  bool added_useless_clue = false;
  std::vector<std::pair<Clue, double>> scored;

  for (int target = 0; target < state.num_players; ++target) {
    if (target == giver) continue;
    for (const Clue& clue : state.all_valid_clues(target)) {
      auto list_orders =
          state.clue_touched(state.hands[target], clue.kind, clue.value);

      // Only touches previously-clued trash — useless.
      bool all_trash_touch = !list_orders.empty();
      for (int o : list_orders) {
        if (!state.deck[o].clued) { all_trash_touch = false; break; }
        auto id = state.deck[o].id();
        if (!id || !state.is_basic_trash(*id)) { all_trash_touch = false; break; }
      }
      if (all_trash_touch) {
        if (added_useless_clue) continue;
        added_useless_clue = true;
        scored.emplace_back(clue, 0.0);
        continue;
      }

      ClueAction action{giver, clue.target, list_orders, clue.base()};
      Game hypo = simulate_clue(action);
      auto last = hypo.last_move();
      if (last && std::holds_alternative<ClueInterp>(*last) &&
          std::get<ClueInterp>(*last) == ClueInterp::MISTAKE) {
        continue;
      }

      double clue_result = hanabi::reactor::get_result(*this, hypo, action);
      bool reactive_move =
          last && std::holds_alternative<ClueInterp>(*last) &&
          std::get<ClueInterp>(*last) == ClueInterp::REACTIVE;
      bool hypo_score_up = hypo.common.hypo_score() > common.hypo_score();
      bool shrunk_possible = false;
      for (int o : state.hands[clue.target]) {
        auto did = state.deck[o].id();
        bool useful_card = !did || state.is_useful(*did);
        bool clued_in_hypo = hypo.state.deck[o].clued;
        bool shrunk = hypo.common.thoughts[o].possible.length() <
                      common.thoughts[o].possible.length();
        if (useful_card && clued_in_hypo && shrunk) {
          shrunk_possible = true;
          break;
        }
      }
      bool useful = clue_result > -1.0 &&
                    (reactive_move || hypo_score_up || shrunk_possible);

      if (useful) {
        scored.emplace_back(clue, clue_result);
      } else if (!added_useless_clue) {
        added_useless_clue = true;
        scored.emplace_back(clue, 0.0);
      }
      // else: skip; redundant useless clue
    }
  }

  std::stable_sort(scored.begin(), scored.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
  for (const auto& [clue, _] : scored) {
    if (clue.kind == ClueKind::COLOUR) {
      out.push_back(PerformColour{clue.target, clue.value});
    } else {
      out.push_back(PerformRank{clue.target, clue.value});
    }
  }
  return out;
}

// --- take_action (the main decision function) ---------------------------

namespace {

double clue_eval_value(const Game& game, const Clue& clue) {
  const State& state = game.state;
  ClueAction act{state.our_player_index, clue.target,
                  state.clue_touched(state.hands[clue.target], clue.kind, clue.value),
                  clue.base()};
  return hanabi::reactor::eval_action(game, Action{act});
}

bool contains_v(const std::vector<int>& v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

PerformAction Game::take_action() const {
  using namespace hanabi::reactor;
  const State& s = state;
  const Player& m = me();
  int next_player_index = s.next_player_index(s.our_player_index);

  // --- Handle urgent (signalled to play / signalled to discard) cards. ---
  std::optional<int> urgent_order;
  for (int o : s.our_hand()) {
    if (meta[o].urgent) {
      urgent_order = o;
      break;
    }
  }
  std::optional<PerformAction> urgent_action;
  if (urgent_order) {
    bool urgent_bob_save = s.can_clue() && !waiting.empty() &&
                            waiting.front().reacter == s.our_player_index &&
                            waiting.front().receiver != next_player_index &&
                            !common.obvious_loaded(*this, next_player_index);
    if (urgent_bob_save) {
      Game tmp = *this;
      tmp.zcs_turn = -1;
      auto bob_chop = tmp.chop(next_player_index);
      if (bob_chop) {
        auto bob_chop_id = s.deck[*bob_chop].id();
        if (bob_chop_id && s.is_critical(*bob_chop_id)) {
          auto clues = s.can_clue() ? s.all_valid_clues(next_player_index)
                                     : std::vector<Clue>{};
          if (!clues.empty()) {
            auto best = *std::max_element(
                clues.begin(), clues.end(),
                [&](const Clue& a, const Clue& b) {
                  return clue_eval_value(*this, a) < clue_eval_value(*this, b);
                });
            urgent_action = best.kind == ClueKind::COLOUR
                                ? PerformAction{PerformColour{best.target, best.value}}
                                : PerformAction{PerformRank{best.target, best.value}};
          }
        }
      }
    }
    if (!urgent_action) {
      CardStatus status = meta[*urgent_order].status;
      const Thought& thought = m.thoughts[*urgent_order];
      if (status == CardStatus::CALLED_TO_PLAY &&
          !thought.possible.forall([&](Identity i) { return s.is_basic_trash(i); })) {
        urgent_action = PerformPlay{*urgent_order};
      } else if (status == CardStatus::CALLED_TO_DISCARD &&
                  !thought.possible.forall([&](Identity i) { return s.is_critical(i); })) {
        urgent_action = PerformDiscard{*urgent_order};
      }
    }
  }

  // --- Endgame solver fork ---
  if (s.rem_score() <= static_cast<int>(s.variant->suits.size()) + 1) {
    hanabi::endgame::EndgameSolver solver(/*mc=*/true, /*timeout=*/30.0);
    auto result = solver.solve(*this);
    if (result.ok() && result.winrate >= hanabi::endgame::Fraction(1, 100)) {
      return result.action;
    }
    // Solver returned no winning action or winrate < 1%; fall through to heuristic.
  }

  if (urgent_action) return *urgent_action;

  // --- Find playable orders ---
  auto common_p = common.obvious_playables(*this, s.our_player_index);
  auto known_p = m.obvious_playables(*this, s.our_player_index);

  std::vector<int> possible_connectors;
  if (!common_p.empty() && !waiting.empty() &&
      waiting.front().receiver == s.our_player_index) {
    int reacter = waiting.front().reacter;
    for (int p : common_p) {
      for (Identity i : m.thoughts[p].inferred) {
        auto nxt = i.next();
        if (!nxt) continue;
        bool match = false;
        for (int o : s.hands[reacter]) {
          if (m.thoughts[o].matches(*nxt)) {
            match = true;
            break;
          }
        }
        if (match) {
          possible_connectors.push_back(p);
          break;
        }
      }
    }
  }

  std::vector<int> playable_orders;
  if (!possible_connectors.empty()) {
    int target = possible_connectors.front();
    int best_signal = meta[target].signal_turn.value_or(99);
    for (int o : possible_connectors) {
      int s_v = meta[o].signal_turn.value_or(99);
      if (s_v < best_signal) {
        best_signal = s_v;
        target = o;
      }
    }
    playable_orders = {target};
  } else if (!known_p.empty()) {
    for (int order : known_p) {
      if (meta[order].status == CardStatus::CALLED_TO_PLAY) {
        playable_orders.push_back(order);
        continue;
      }
      bool same_focused_dup = false;
      for (int o : s.our_hand()) {
        if (o != order && m.thoughts[o].possible == m.thoughts[order].possible &&
            meta[o].focused) {
          same_focused_dup = true;
          break;
        }
      }
      if (!same_focused_dup) playable_orders.push_back(order);
    }
  } else {
    playable_orders = m.thinks_playables(*this, s.our_player_index);
  }

  bool can_clue_now = s.can_clue() &&
                       (waiting.empty() || waiting.front().receiver != s.our_player_index);

  std::vector<std::pair<PerformAction, Action>> all_clues;
  if (can_clue_now) {
    for (int target = 0; target < s.num_players; ++target) {
      if (target == s.our_player_index) continue;
      for (const Clue& clue : s.all_valid_clues(target)) {
        PerformAction perform = clue.kind == ClueKind::COLOUR
                                     ? PerformAction{PerformColour{clue.target, clue.value}}
                                     : PerformAction{PerformRank{clue.target, clue.value}};
        ClueAction act{s.our_player_index, clue.target,
                        s.clue_touched(s.hands[target], clue.kind, clue.value),
                        clue.base()};
        all_clues.emplace_back(perform, Action{act});
      }
    }
  }

  std::vector<std::pair<PerformAction, Action>> all_plays;
  for (int o : playable_orders) {
    auto inferred = m.thoughts[o].id(/*infer=*/true);
    Action act = inferred
                      ? Action{PlayAction{s.our_player_index, o, inferred->suit_index, inferred->rank}}
                      : Action{PlayAction{s.our_player_index, o, -1, -1}};
    all_plays.emplace_back(PerformPlay{o}, std::move(act));
  }

  // Forced-play detection.
  bool potential_forced_play = false;
  if (!all_plays.empty() && !waiting.empty() &&
      waiting.front().reacter == next_player_index) {
    for (int o : playable_orders) {
      for (Identity id : m.thoughts[o].inferred) {
        auto nxt = id.next();
        if (!nxt) continue;
        for (int o2 : s.hands[next_player_index]) {
          auto deck_id = s.deck[o2].id();
          if (deck_id && *deck_id == *nxt) {
            potential_forced_play = true;
            break;
          }
        }
        if (potential_forced_play) break;
      }
      if (potential_forced_play) break;
    }
  }

  bool cant_discard = s.clue_tokens == 8 ||
                       (s.pace() == 0 && (!all_clues.empty() || !all_plays.empty())) ||
                       potential_forced_play;

  std::vector<std::pair<PerformAction, Action>> all_discards;
  if (!cant_discard) {
    auto trash = m.thinks_trash(*this, s.our_player_index);
    std::vector<int> expected;
    if (!trash.empty()) {
      expected = trash;
    } else if (!m.obvious_locked(*this, s.our_player_index) && all_plays.empty() &&
                has_ptd()) {
      auto chop_o = chop(s.our_player_index);
      if (chop_o) expected = {*chop_o};
    }
    std::vector<int> discard_orders;
    if (!waiting.empty() && waiting.front().receiver == s.our_player_index) {
      discard_orders = expected;
    } else {
      auto discardable = m.discardable(*this, s.our_player_index);
      std::vector<int> seen;
      for (int o : expected) {
        if (!contains_v(seen, o)) {
          discard_orders.push_back(o);
          seen.push_back(o);
        }
      }
      for (int o : discardable) {
        if (!contains_v(seen, o)) {
          discard_orders.push_back(o);
          seen.push_back(o);
        }
      }
    }
    for (int o : discard_orders) {
      auto inferred = m.thoughts[o].id(/*infer=*/true);
      Action act = inferred
                        ? Action{DiscardAction{s.our_player_index, o, inferred->suit_index, inferred->rank, false}}
                        : Action{DiscardAction{s.our_player_index, o, -1, -1, false}};
      all_discards.emplace_back(PerformDiscard{o}, std::move(act));
    }
  }

  std::vector<std::pair<PerformAction, Action>> all_actions;
  for (auto& a : all_clues) all_actions.push_back(std::move(a));
  for (auto& a : all_plays) all_actions.push_back(std::move(a));
  for (auto& a : all_discards) all_actions.push_back(std::move(a));

  if (all_actions.empty()) {
    if (s.clue_tokens == 8) return PerformPlay{s.our_hand().front()};
    return PerformDiscard{m.locked_discard(s, s.our_player_index)};
  }

  auto best = std::max_element(
      all_actions.begin(), all_actions.end(),
      [&](const auto& a, const auto& b) {
        return hanabi::reactor::eval_action(*this, a.second) <
               hanabi::reactor::eval_action(*this, b.second);
      });
  return best->first;
}

std::vector<PerformAction> Game::find_all_discards(int player_index) const {
  auto trash = common.thinks_trash(*this, player_index);
  int target;
  if (!trash.empty()) {
    target = trash.front();
  } else if (auto c = chop(player_index)) {
    target = *c;
  } else {
    target = players[player_index].locked_discard(state, player_index);
  }
  return {PerformDiscard{target}};
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
