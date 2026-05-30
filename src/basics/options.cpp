#include "hanabi/basics/options.h"

#include <nlohmann/json.hpp>

namespace hanabi {

TableOptions TableOptions::from_json(const nlohmann::json& obj) {
  TableOptions o;
  o.num_players = obj.at("numPlayers").get<int>();
  o.variant_name = obj.at("variantName").get<std::string>();
  o.starting_player = obj.value("startingPlayer", 0);
  o.deck_plays = obj.value("deckPlays", false);
  o.detrimental_characters = obj.value("detrimentalCharacters", false);
  o.empty_clues = obj.value("emptyClues", false);
  o.one_extra_card = obj.value("oneExtraCard", false);
  o.one_less_card = obj.value("oneLessCard", false);
  o.speedrun = obj.value("speedrun", false);
  return o;
}

}  // namespace hanabi
