// Port of python-bot/src/hanabi_bot/basics/sarcastic.py.
// Original Scala: scala-bot/src/scala_bot/basics/sarcastic.scala.
//
// Sarcastic-discard interpretation: when a useful (non-trash) card is
// discarded, decide whether the convention reads it as Sarcastic / GD /
// Baton / None / Mistake.
#pragma once

#include <variant>
#include <vector>

#include "hanabi/basics/identity.h"

namespace hanabi {

class Game;
struct DiscardAction;

struct DiscardResultNone {
  bool operator==(const DiscardResultNone&) const = default;
};

struct DiscardResultMistake {
  bool operator==(const DiscardResultMistake&) const = default;
};

struct DiscardResultSarcastic {
  std::vector<int> orders;
  bool operator==(const DiscardResultSarcastic&) const = default;
};

struct DiscardResultGentlemansDiscard {
  std::vector<int> orders;
  bool operator==(const DiscardResultGentlemansDiscard&) const = default;
};

struct DiscardResultBaton {
  int order;
  bool operator==(const DiscardResultBaton&) const = default;
};

using DiscardResult = std::variant<DiscardResultNone, DiscardResultMistake,
                                     DiscardResultSarcastic,
                                     DiscardResultGentlemansDiscard,
                                     DiscardResultBaton>;

// Whether `order` could plausibly receive a sarcastic transfer of `id`.
bool valid_transfer(const Game& game, Identity id, int order);

// Interpret a useful (non-trash) discard as Sarcastic, GD, Baton, None, or Mistake.
DiscardResult interpret_useful_dc(const Game& game, const DiscardAction& action);

}  // namespace hanabi
