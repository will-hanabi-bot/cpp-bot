#include "hanabi/basics/action.h"

#include <stdexcept>

#include "hanabi/basics/variant.h"

namespace hanabi {

// --- Inbound from_json -------------------------------------------------

StatusAction StatusAction::from_json(const nlohmann::json& obj) {
  return {obj.at("clues").get<int>(), obj.at("score").get<int>(),
          obj.at("maxScore").get<int>()};
}

TurnAction TurnAction::from_json(const nlohmann::json& obj) {
  return {obj.at("num").get<int>(), obj.at("currentPlayerIndex").get<int>()};
}

ClueAction ClueAction::from_json(const nlohmann::json& obj) {
  const auto& clue_obj = obj.at("clue");
  ClueKind kind = clue_obj.at("type").get<int>() == 0 ? ClueKind::COLOUR : ClueKind::RANK;
  std::vector<int> list_;
  for (const auto& v : obj.at("list")) list_.push_back(v.get<int>());
  return ClueAction(obj.at("giver").get<int>(),
                    obj.at("target").get<int>(),
                    std::move(list_),
                    BaseClue(kind, clue_obj.at("value").get<int>()));
}

DrawAction DrawAction::from_json(const nlohmann::json& obj) {
  return {obj.at("playerIndex").get<int>(), obj.at("order").get<int>(),
          obj.at("suitIndex").get<int>(), obj.at("rank").get<int>()};
}

PlayAction PlayAction::from_json(const nlohmann::json& obj) {
  return {obj.at("playerIndex").get<int>(), obj.at("order").get<int>(),
          obj.at("suitIndex").get<int>(), obj.at("rank").get<int>()};
}

DiscardAction DiscardAction::from_json(const nlohmann::json& obj) {
  return {obj.at("playerIndex").get<int>(), obj.at("order").get<int>(),
          obj.at("suitIndex").get<int>(), obj.at("rank").get<int>(),
          obj.at("failed").get<bool>()};
}

StrikeAction StrikeAction::from_json(const nlohmann::json& obj) {
  return {obj.at("num").get<int>(), obj.at("turn").get<int>(),
          obj.at("order").get<int>()};
}

GameOverAction GameOverAction::from_json(const nlohmann::json& obj) {
  return {obj.at("endCondition").get<int>(), obj.at("playerIndex").get<int>()};
}

Action orient_action_for_engine(Action act, const Variant& variant) {
  if (auto* p = std::get_if<PlayAction>(&act)) {
    if (p->suit_index >= 0 &&
        p->suit_index < static_cast<int>(variant.suits.size()) &&
        variant.suits[p->suit_index].suit_type.inverted) {
      return DiscardAction{p->player_index_v, p->order, p->suit_index,
                            p->rank, /*failed=*/false};
    }
  } else if (auto* d = std::get_if<DiscardAction>(&act)) {
    if (d->suit_index >= 0 &&
        d->suit_index < static_cast<int>(variant.suits.size()) &&
        variant.suits[d->suit_index].suit_type.inverted && !d->failed) {
      return PlayAction{d->player_index_v, d->order, d->suit_index, d->rank};
    }
  }
  return act;
}

std::optional<Action> action_from_json(const nlohmann::json& obj) {
  const auto type_it = obj.find("type");
  if (type_it == obj.end()) return std::nullopt;
  const std::string type = type_it->get<std::string>();
  if (type == "clue") return ClueAction::from_json(obj);
  if (type == "discard") return DiscardAction::from_json(obj);
  if (type == "play") return PlayAction::from_json(obj);
  if (type == "draw") return DrawAction::from_json(obj);
  if (type == "status") return StatusAction::from_json(obj);
  if (type == "turn") return TurnAction::from_json(obj);
  if (type == "strike") return StrikeAction::from_json(obj);
  if (type == "gameOver") return GameOverAction::from_json(obj);
  return std::nullopt;
}

// --- Outbound to_json --------------------------------------------------

nlohmann::json PerformPlay::to_json(int table_id) const {
  return {{"tableID", table_id}, {"type", 0}, {"target", target}};
}
nlohmann::json PerformDiscard::to_json(int table_id) const {
  return {{"tableID", table_id}, {"type", 1}, {"target", target}};
}
nlohmann::json PerformColour::to_json(int table_id) const {
  return {{"tableID", table_id}, {"type", 2}, {"target", target}, {"value", value}};
}
nlohmann::json PerformRank::to_json(int table_id) const {
  return {{"tableID", table_id}, {"type", 3}, {"target", target}, {"value", value}};
}
nlohmann::json PerformTerminate::to_json(int table_id) const {
  return {{"tableID", table_id}, {"type", 4}, {"target", target}, {"value", value}};
}

PerformAction perform_action_from_json(const nlohmann::json& obj) {
  int action_type = obj.at("type").get<int>();
  int target = obj.at("target").get<int>();
  int value = obj.value("value", 0);
  switch (action_type) {
    case 0: return PerformPlay{target};
    case 1: return PerformDiscard{target};
    case 2: return PerformColour{target, value};
    case 3: return PerformRank{target, value};
    case 4: return PerformTerminate{target, value};
    default:
      throw std::invalid_argument("Unknown PerformAction type");
  }
}

}  // namespace hanabi
