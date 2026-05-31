// Port of python-bot/src/hanabi_bot/basics/fix.py.
// Original Scala: scala-bot/src/scala_bot/basics/fix.scala.
//
// Fix-clue detection: was the given clue a "fix" (resetting a previously-clued
// card or revealing a duplicate). Also distribution_clue and rainbow_mismatch.
//
#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

namespace hanabi {

class Game;
struct Player;
struct ClueAction;

struct FixResultNormal {
  std::vector<int> clued_resets;
  std::vector<int> duplicate_reveals;
  bool operator==(const FixResultNormal&) const = default;
};

struct FixResultNoNewInfo {
  bool operator==(const FixResultNoNewInfo&) const = default;
};

struct FixResultNone {
  bool operator==(const FixResultNone&) const = default;
};

using FixResult = std::variant<FixResultNormal, FixResultNoNewInfo, FixResultNone>;

FixResult check_fix(const Game& prev, const Game& game, const ClueAction& action);

std::optional<IdentitySet> distribution_clue(const Game& prev, const Game& game,
                                                const ClueAction& action, int focus);

bool rainbow_mismatch(const Game& game, const ClueAction& action, Identity id,
                       int prompt);

// If id is given, returns a non-empty list iff it can be made playable by
// `target`'s turn. Otherwise returns the orders that would be playable in
// target's hand by their turn. Port of fix.scala lines 55-78.
std::vector<int> connectable_simple(const Game& game, const Player& player,
                                       int start, int target,
                                       std::optional<Identity> id = std::nullopt);

}  // namespace hanabi
