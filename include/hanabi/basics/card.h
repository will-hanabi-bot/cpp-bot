// Port of python-bot/src/hanabi_bot/basics/card.py.
// Original Scala: scala-bot/src/scala_bot/basics/Card.scala lines 65-230.
//
// Card (a physical card in the game) and Thought (per-perspective beliefs
// about identity) and ConvData (cross-perspective conventional state).
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "hanabi/basics/clue.h"
#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"

namespace hanabi {

struct Thought;

enum class CardStatus : std::uint8_t {
  NONE,
  CHOP_MOVED,
  CALLED_TO_PLAY,
  CALLED_TO_DISCARD,
  PERMISSION_TO_DISCARD,
  FINESSED,
  SARCASTIC,
  GENTLEMANS_DISCARD,
  F_MAYBE_BLUFFED,
  MAYBE_BLUFFED,
  BLUFFED,
};

constexpr std::string_view name(CardStatus s) {
  switch (s) {
    case CardStatus::NONE: return "none";
    case CardStatus::CHOP_MOVED: return "chop moved";
    case CardStatus::CALLED_TO_PLAY: return "called to play";
    case CardStatus::CALLED_TO_DISCARD: return "called to discard";
    case CardStatus::PERMISSION_TO_DISCARD: return "permission to discard";
    case CardStatus::FINESSED: return "finessed";
    case CardStatus::SARCASTIC: return "sarcastic";
    case CardStatus::GENTLEMANS_DISCARD: return "gentleman's discard";
    case CardStatus::F_MAYBE_BLUFFED: return "finessed, maybe bluffed";
    case CardStatus::MAYBE_BLUFFED: return "maybe bluffed";
    case CardStatus::BLUFFED: return "bluffed";
  }
  return {};
}

struct Card {
  int suit_index = -1;       // -1 if unknown to the observer
  int rank = -1;             // -1 if unknown
  int order = 0;             // position in the deck (0 = topmost)
  int turn_drawn = 0;
  bool clued = false;
  std::vector<CardClue> clues;

  std::optional<Identity> id() const {
    if (suit_index == -1 || rank == -1) return std::nullopt;
    return Identity(suit_index, rank);
  }

  // Identifiable::matches against Identity, Card, or Thought. The Thought overload
  // is defined out-of-line in card.cpp (Thought is incomplete here).
  bool matches(Identity other, bool assume = false) const;
  bool matches(const Card& other, bool assume = false) const;
  bool matches(const Thought& other, bool infer = false, bool symmetric = false,
               bool assume = false) const;

  bool operator==(const Card&) const = default;
};

struct Thought {
  int suit_index = -1;
  int rank = -1;
  int order = 0;
  IdentitySet possible;
  IdentitySet inferred;
  std::optional<IdentitySet> old_inferred;
  std::optional<IdentitySet> old_possible;
  std::optional<IdentitySet> info_lock;
  bool rewinded = false;
  bool reset = false;

  static Thought initial(int suit_index, int rank, int order, IdentitySet possible) {
    return Thought{suit_index, rank, order, possible, possible, std::nullopt,
                   std::nullopt, std::nullopt, false, false};
  }

  std::optional<Identity> id(bool infer = false, bool symmetric = false,
                              bool partial = false) const;

  // Inferences if any, otherwise possible ids.
  IdentitySet possibilities() const {
    return inferred.is_empty() ? possible : inferred;
  }

  bool matches(Identity other, bool infer = false, bool symmetric = false,
               bool assume = false) const;
  bool matches(const Card& other, bool infer = false, bool symmetric = false,
               bool assume = false) const;
  bool matches(const Thought& other, bool infer = false, bool symmetric = false,
               bool assume = false) const;

  Thought reset_inferences() const;

  bool operator==(const Thought&) const = default;
};

struct ConvData {
  int order = 0;
  bool focused = false;
  bool urgent = false;
  bool trash = false;
  CardStatus status = CardStatus::NONE;
  bool hidden = false;
  // Turns where new info was discovered about this card. Newest-first.
  std::vector<int> reasoning;
  std::optional<int> signal_turn;
  std::optional<int> by;

  bool cm() const { return status == CardStatus::CHOP_MOVED; }

  bool bluffed() const {
    return status == CardStatus::BLUFFED ||
           status == CardStatus::F_MAYBE_BLUFFED ||
           status == CardStatus::MAYBE_BLUFFED;
  }

  ConvData cleared() const;
  ConvData reason(int turn_count) const;
  ConvData signal(int turn_count) const;

  bool operator==(const ConvData&) const = default;
};

}  // namespace hanabi
