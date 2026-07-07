#include "hanabi/logging/state_snapshot.h"

#include <stdexcept>
#include <string>
#include <variant>

#include "hanabi/basics/action.h"
#include "hanabi/basics/card.h"
#include "hanabi/basics/clue.h"
#include "hanabi/basics/game.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/basics/options.h"
#include "hanabi/basics/state.h"
#include "hanabi/basics/variant.h"
#include "hanabi/logging/game_logger.h"

namespace hanabi::logging {

namespace {

using json = nlohmann::json;

const char* card_status_token(CardStatus s) {
  switch (s) {
    case CardStatus::NONE: return "NONE";
    case CardStatus::CHOP_MOVED: return "CHOP_MOVED";
    case CardStatus::CALLED_TO_PLAY: return "CALLED_TO_PLAY";
    case CardStatus::CALLED_TO_DISCARD: return "CALLED_TO_DISCARD";
    case CardStatus::PERMISSION_TO_DISCARD: return "PERMISSION_TO_DISCARD";
    case CardStatus::FINESSED: return "FINESSED";
    case CardStatus::SARCASTIC: return "SARCASTIC";
    case CardStatus::GENTLEMANS_DISCARD: return "GENTLEMANS_DISCARD";
    case CardStatus::F_MAYBE_BLUFFED: return "F_MAYBE_BLUFFED";
    case CardStatus::MAYBE_BLUFFED: return "MAYBE_BLUFFED";
    case CardStatus::BLUFFED: return "BLUFFED";
  }
  return "?";
}

json options_to_json(const TableOptions& o) {
  return json{
      {"num_players", o.num_players},
      {"variant_name", o.variant_name},
      {"starting_player", o.starting_player},
      {"deck_plays", o.deck_plays},
      {"detrimental_characters", o.detrimental_characters},
      {"empty_clues", o.empty_clues},
      {"one_extra_card", o.one_extra_card},
      {"one_less_card", o.one_less_card},
      {"speedrun", o.speedrun},
  };
}

TableOptions options_from_json(const json& j) {
  TableOptions o;
  o.num_players = j.value("num_players", 2);
  o.variant_name = j.value("variant_name", "");
  o.starting_player = j.value("starting_player", 0);
  o.deck_plays = j.value("deck_plays", false);
  o.detrimental_characters = j.value("detrimental_characters", false);
  o.empty_clues = j.value("empty_clues", false);
  o.one_extra_card = j.value("one_extra_card", false);
  o.one_less_card = j.value("one_less_card", false);
  o.speedrun = j.value("speedrun", false);
  return o;
}

json interp_to_json(const Interp& interp) {
  return std::visit(
      [](const auto& v) -> json {
        using T = std::decay_t<decltype(v)>;
        json j;
        if constexpr (std::is_same_v<T, ClueInterp>) {
          j["k"] = "clue";
          j["v"] = std::string(name(v));
        } else if constexpr (std::is_same_v<T, PlayInterp>) {
          j["k"] = "play";
          j["v"] = std::string(name(v));
        } else if constexpr (std::is_same_v<T, DiscardInterp>) {
          j["k"] = "discard";
          j["v"] = std::string(name(v));
        }
        return j;
      },
      interp);
}

ClueInterp parse_clue_interp(const std::string& s) {
  if (s == "Mistake") return ClueInterp::MISTAKE;
  if (s == "Reactive") return ClueInterp::REACTIVE;
  if (s == "Play") return ClueInterp::PLAY;
  if (s == "Save") return ClueInterp::SAVE;
  if (s == "Discard") return ClueInterp::DISCARD;
  if (s == "Lock") return ClueInterp::LOCK;
  if (s == "Reveal") return ClueInterp::REVEAL;
  if (s == "Fix") return ClueInterp::FIX;
  if (s == "Stall") return ClueInterp::STALL;
  if (s == "Distribution") return ClueInterp::DISTRIBUTION;
  if (s == "Useless") return ClueInterp::USELESS;
  throw std::invalid_argument("Unknown ClueInterp: " + s);
}

PlayInterp parse_play_interp(const std::string& s) {
  if (s == "None") return PlayInterp::NONE;
  if (s == "Mistake") return PlayInterp::MISTAKE;
  if (s == "OrderCM") return PlayInterp::ORDER_CM;
  throw std::invalid_argument("Unknown PlayInterp: " + s);
}

DiscardInterp parse_discard_interp(const std::string& s) {
  if (s == "None") return DiscardInterp::NONE;
  if (s == "Mistake") return DiscardInterp::MISTAKE;
  if (s == "Sarcastic") return DiscardInterp::SARCASTIC;
  if (s == "GentlemansDiscard") return DiscardInterp::GENTLEMANS_DISCARD;
  if (s == "Emergency") return DiscardInterp::EMERGENCY;
  if (s == "Positional") return DiscardInterp::POSITIONAL;
  if (s == "Sacrifice") return DiscardInterp::SACRIFICE;
  throw std::invalid_argument("Unknown DiscardInterp: " + s);
}

Interp parse_interp(const json& j) {
  std::string k = j.at("k").get<std::string>();
  std::string v = j.at("v").get<std::string>();
  if (k == "clue") return parse_clue_interp(v);
  if (k == "play") return parse_play_interp(v);
  if (k == "discard") return parse_discard_interp(v);
  throw std::invalid_argument("Unknown Interp kind: " + k);
}

}  // namespace

// --- Internal action JSON format ------------------------------------------

json action_to_internal_json(const Action& a) {
  return std::visit(
      [](const auto& v) -> json {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, StatusAction>) {
          return json{{"t", "status"},
                      {"clues", v.clues},
                      {"score", v.score},
                      {"max", v.max_score}};
        } else if constexpr (std::is_same_v<T, TurnAction>) {
          return json{{"t", "turn"}, {"num", v.num}, {"cpi", v.current_player_index}};
        } else if constexpr (std::is_same_v<T, ClueAction>) {
          return json{{"t", "clue"},
                      {"giver", v.giver},
                      {"target", v.target},
                      {"list", v.list_},
                      {"kind", v.clue.kind == ClueKind::COLOUR ? "C" : "R"},
                      {"value", v.clue.value}};
        } else if constexpr (std::is_same_v<T, DrawAction>) {
          return json{{"t", "draw"},
                      {"p", v.player_index_v},
                      {"order", v.order},
                      {"suit", v.suit_index},
                      {"rank", v.rank}};
        } else if constexpr (std::is_same_v<T, PlayAction>) {
          return json{{"t", "play"},
                      {"p", v.player_index_v},
                      {"order", v.order},
                      {"suit", v.suit_index},
                      {"rank", v.rank}};
        } else if constexpr (std::is_same_v<T, DiscardAction>) {
          return json{{"t", "discard"},
                      {"p", v.player_index_v},
                      {"order", v.order},
                      {"suit", v.suit_index},
                      {"rank", v.rank},
                      {"failed", v.failed}};
        } else if constexpr (std::is_same_v<T, StrikeAction>) {
          return json{{"t", "strike"},
                      {"num", v.num},
                      {"turn", v.turn},
                      {"order", v.order}};
        } else if constexpr (std::is_same_v<T, GameOverAction>) {
          return json{{"t", "gameover"},
                      {"end", v.end_condition},
                      {"p", v.player_index_v}};
        } else if constexpr (std::is_same_v<T, InterpAction>) {
          json j{{"t", "interp"}};
          j["interp"] = interp_to_json(v.interp);
          return j;
        }
      },
      a);
}

Action action_from_internal_json(const json& j) {
  std::string t = j.at("t").get<std::string>();
  if (t == "status") {
    return StatusAction{j.at("clues").get<int>(), j.at("score").get<int>(),
                         j.at("max").get<int>()};
  }
  if (t == "turn") {
    return TurnAction{j.at("num").get<int>(), j.at("cpi").get<int>()};
  }
  if (t == "clue") {
    ClueKind kind =
        j.at("kind").get<std::string>() == "C" ? ClueKind::COLOUR : ClueKind::RANK;
    std::vector<int> list_ = j.at("list").get<std::vector<int>>();
    return ClueAction(j.at("giver").get<int>(), j.at("target").get<int>(),
                       std::move(list_), BaseClue(kind, j.at("value").get<int>()));
  }
  if (t == "draw") {
    return DrawAction{j.at("p").get<int>(), j.at("order").get<int>(),
                       j.at("suit").get<int>(), j.at("rank").get<int>()};
  }
  if (t == "play") {
    return PlayAction{j.at("p").get<int>(), j.at("order").get<int>(),
                       j.at("suit").get<int>(), j.at("rank").get<int>()};
  }
  if (t == "discard") {
    return DiscardAction{j.at("p").get<int>(), j.at("order").get<int>(),
                          j.at("suit").get<int>(), j.at("rank").get<int>(),
                          j.value("failed", false)};
  }
  if (t == "strike") {
    return StrikeAction{j.at("num").get<int>(), j.at("turn").get<int>(),
                         j.at("order").get<int>()};
  }
  if (t == "gameover") {
    return GameOverAction{j.at("end").get<int>(), j.at("p").get<int>()};
  }
  if (t == "interp") {
    InterpAction ia{parse_interp(j.at("interp"))};
    return ia;
  }
  throw std::invalid_argument("Unknown action type: " + t);
}

// --- Snapshot writer ------------------------------------------------------

namespace {

json build_replay_section(const Game& game) {
  const State& s = game.state;
  json replay;
  replay["variant"] = s.variant ? s.variant->name : "";
  replay["options"] = options_to_json(s.options);
  replay["num_players"] = s.num_players;
  replay["names"] = s.names;
  replay["our_player_index"] = s.our_player_index;
  replay["all_plays"] = game.all_plays;
  replay["zcs_turn"] = game.zcs_turn;

  // Ground-truth deck: for every order that has a known identity in
  // deck_ids (i.e. the bot has observed it as a draw — either via the
  // server revealing the suit/rank of a non-our-hand card, or via a play/
  // discard reveal). Cards still in our hand may be unknown (deck_ids[o] =
  // nullopt). Snapshot consumers (replay_log) should treat nullopt entries
  // as "unknown to replayer too" — for our hand this is fine because the
  // bot's reasoning works from inference; for unseen-deck entries it's a
  // limitation of what was knowable at snapshot time.
  json deck = json::array();
  for (const auto& opt_id : game.deck_ids) {
    if (opt_id) {
      deck.push_back(json::array({opt_id->suit_index, opt_id->rank}));
    } else {
      deck.push_back(nullptr);
    }
  }
  replay["deck"] = std::move(deck);

  // Action history: flattened across turns. Each handle_action call appended
  // exactly one entry to state.action_list at the turn it was processed,
  // and replay just re-applies them in order.
  json actions = json::array();
  for (const auto& per_turn : s.action_list) {
    for (const auto& a : per_turn) {
      actions.push_back(action_to_internal_json(a));
    }
  }
  replay["actions"] = std::move(actions);
  return replay;
}

json build_debug_section(const Game& game) {
  const State& s = game.state;
  json dbg;
  dbg["play_stacks"] = s.play_stacks;
  dbg["max_ranks"] = s.max_ranks;
  dbg["clue_tokens"] = s.clue_tokens;
  dbg["strikes"] = s.strikes;
  dbg["cards_left"] = s.cards_left;
  dbg["turn_count"] = s.turn_count;
  dbg["current_player_index"] = s.current_player_index;
  dbg["in_progress"] = game.in_progress;
  if (s.endgame_turns) dbg["endgame_turns"] = *s.endgame_turns;
  else dbg["endgame_turns"] = nullptr;

  // Discards in pile order. Each [suit][rank-1] holds orders newest-first;
  // we emit them grouped by identity for compactness.
  json discards = json::array();
  for (int suit = 0; suit < static_cast<int>(s.discard_stacks.size()); ++suit) {
    for (int rank = 1; rank <= 5; ++rank) {
      const auto& orders = s.discard_stacks[suit][rank - 1];
      for (int order : orders) {
        discards.push_back(json{{"suit", suit}, {"rank", rank}, {"order", order}});
      }
    }
  }
  dbg["discards"] = std::move(discards);

  // Hands: per-player slot order, per-card empathy + meta. Empathy is the
  // common-perspective inferred/possible bitmask — the "everyone-knows"
  // view a debugger usually wants. For our own hand the per-player and
  // common views typically coincide; the snapshot shows common to stay
  // observer-symmetric.
  json hands = json::array();
  for (int p = 0; p < s.num_players; ++p) {
    json player_obj;
    player_obj["player"] = p;
    if (p < static_cast<int>(s.names.size())) player_obj["name"] = s.names[p];
    json cards = json::array();
    const auto& hand = s.hands[p];
    for (int slot = 0; slot < static_cast<int>(hand.size()); ++slot) {
      int order = hand[slot];
      const Thought& th = game.common.thoughts[order];
      const Card& card = s.deck[order];
      const ConvData& m = game.meta[order];
      json c;
      c["slot"] = slot + 1;  // 1-indexed for display
      c["order"] = order;
      c["clued"] = card.clued;
      c["status"] = card_status_token(m.status);
      c["focused"] = m.focused;
      c["urgent"] = m.urgent;
      c["trash"] = m.trash;
      c["possible"] = th.possible.bits();
      c["inferred"] = th.inferred.bits();
      if (th.info_lock) c["info_lock"] = th.info_lock->bits();
      else c["info_lock"] = nullptr;
      // Ground-truth identity if known (for our hand: only if revealed).
      if (auto id = card.id()) {
        c["id"] = json::array({id->suit_index, id->rank});
      } else {
        c["id"] = nullptr;
      }
      cards.push_back(std::move(c));
    }
    player_obj["cards"] = std::move(cards);
    hands.push_back(std::move(player_obj));
  }
  dbg["hands"] = std::move(hands);

  // Waiting reactive connections.
  json waiting = json::array();
  for (const auto& wc : game.waiting) {
    waiting.push_back(json{{"giver", wc.giver},
                              {"reacter", wc.reacter},
                              {"receiver", wc.receiver},
                              {"receiver_hand", wc.receiver_hand},
                              {"clue_kind", wc.clue.kind == ClueKind::COLOUR ? "C" : "R"},
                              {"clue_value", wc.clue.value},
                              {"focus_slot", wc.focus_slot},
                              {"inverted", wc.inverted},
                              {"turn", wc.turn},
                              {"all_plays", wc.all_plays},
                              {"react_order", wc.react_order}});
  }
  dbg["waiting"] = std::move(waiting);

  // Move history.
  json moves = json::array();
  for (const auto& interp : game.move_history) {
    moves.push_back(interp_to_json(interp));
  }
  dbg["move_history"] = std::move(moves);

  return dbg;
}

}  // namespace

json build_state_snapshot(const Game& game, int turn) {
  json rec;
  rec["ch"] = "STATE";
  rec["turn"] = turn;
  rec["current_player_index"] = game.state.current_player_index;
  rec["replay"] = build_replay_section(game);
  rec["debug"] = build_debug_section(game);
  return rec;
}

void emit_state_snapshot(GameLogger& logger, const Game& game, int turn) {
  logger.emit(build_state_snapshot(game, turn));
}

// --- Snapshot reader / replayer ------------------------------------------

Game apply_snapshot(const json& record) {
  const json& replay = record.at("replay");
  std::string variant_name = replay.at("variant").get<std::string>();
  const Variant& variant = get_variant(variant_name);
  TableOptions opts = options_from_json(replay.at("options"));
  std::vector<std::string> names = replay.at("names").get<std::vector<std::string>>();
  int our_player_index = replay.at("our_player_index").get<int>();
  opts.num_players = static_cast<int>(names.size());
  opts.variant_name = variant_name;

  State state = State::create(std::move(names), our_player_index, variant,
                                std::move(opts));
  Game game = Game::create(0, std::move(state));
  game.catchup = true;
  game.all_plays = replay.value("all_plays", false);

  for (const auto& a_json : replay.at("actions")) {
    Action a = action_from_internal_json(a_json);
    game.handle_action(a);
  }

  game.catchup = false;
  if (replay.contains("zcs_turn")) game.zcs_turn = replay.at("zcs_turn").get<int>();
  return game;
}

}  // namespace hanabi::logging
