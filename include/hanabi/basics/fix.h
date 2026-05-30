// Port of python-bot/src/hanabi_bot/basics/fix.py.
// Original Scala: scala-bot/src/scala_bot/basics/fix.scala.
//
// Fix-clue detection: was the given clue a "fix" (resetting a previously-clued
// card or revealing a duplicate). Also distribution_clue and rainbow_mismatch.
//
// DEFERRED: connectable_simple (uses game.simulate, Stage 6 territory).
#pragma once

#include <optional>
#include <variant>
#include <vector>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

namespace hanabi {

class Game;
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

}  // namespace hanabi
