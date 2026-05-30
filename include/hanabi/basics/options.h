// Port of python-bot/src/hanabi_bot/basics/options.py.
// Original Scala: scala-bot/src/scala_bot/command.scala lines 56-66.
#pragma once

#include <string>

#include <nlohmann/json_fwd.hpp>

namespace hanabi {

struct TableOptions {
  int num_players = 2;
  std::string variant_name;
  int starting_player = 0;
  bool deck_plays = false;
  bool detrimental_characters = false;
  bool empty_clues = false;
  bool one_extra_card = false;
  bool one_less_card = false;
  bool speedrun = false;

  static TableOptions from_json(const nlohmann::json& obj);

  bool operator==(const TableOptions&) const = default;
};

}  // namespace hanabi
