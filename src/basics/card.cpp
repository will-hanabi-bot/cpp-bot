#include "hanabi/basics/card.h"

namespace hanabi {

// --- Card::matches overloads ----------------------------------------------

bool Card::matches(Identity other, bool assume) const {
  auto a = id();
  if (!a) return assume;
  return *a == other;
}

bool Card::matches(const Card& other, bool assume) const {
  auto a = id();
  if (!a) return assume;
  auto b = other.id();
  return b.has_value() && *a == *b;
}

bool Card::matches(const Thought& other, bool infer, bool symmetric, bool assume) const {
  auto a = id();
  if (!a) return assume;
  auto b = other.id(infer, symmetric);
  return b.has_value() && *a == *b;
}

// --- Thought::id ----------------------------------------------------------

std::optional<Identity> Thought::id(bool infer, bool symmetric, bool partial) const {
  if (possible.length() == 1) return possible.head();
  if (!symmetric && suit_index != -1) return Identity(suit_index, rank);
  if (infer && inferred.length() == 1) return inferred.head();
  if (partial) {
    Identity head = possible.head();
    if (possible.forall([&](Identity i) { return i.suit_index == head.suit_index; })) {
      return Identity(head.suit_index, -1);
    }
    if (possible.forall([&](Identity i) { return i.rank == head.rank; })) {
      return Identity(-1, head.rank);
    }
    return std::nullopt;
  }
  return std::nullopt;
}

// --- Thought::matches -----------------------------------------------------

bool Thought::matches(Identity other, bool infer, bool symmetric, bool assume) const {
  auto a = id(infer, symmetric);
  if (!a) return assume;
  return *a == other;
}

bool Thought::matches(const Card& other, bool infer, bool symmetric, bool assume) const {
  auto a = id(infer, symmetric);
  if (!a) return assume;
  auto b = other.id();
  return b.has_value() && *a == *b;
}

bool Thought::matches(const Thought& other, bool infer, bool symmetric, bool assume) const {
  auto a = id(infer, symmetric);
  if (!a) return assume;
  auto b = other.id(infer, symmetric);
  return b.has_value() && *a == *b;
}

Thought Thought::reset_inferences() const {
  if (reset) return *this;

  std::optional<IdentitySet> new_lock;
  if (info_lock) {
    IdentitySet ids = info_lock->intersect(possible);
    if (!ids.is_empty()) new_lock = ids;
  }
  IdentitySet new_inferred = new_lock ? possible.intersect(*new_lock) : possible;

  Thought out = *this;
  out.reset = true;
  out.inferred = new_inferred;
  out.info_lock = new_lock;
  return out;
}

// --- ConvData -------------------------------------------------------------

ConvData ConvData::cleared() const {
  ConvData out = *this;
  out.focused = false;
  out.urgent = false;
  out.trash = false;
  out.status = (status == CardStatus::CHOP_MOVED) ? CardStatus::CHOP_MOVED : CardStatus::NONE;
  out.signal_turn = std::nullopt;
  out.by = std::nullopt;
  return out;
}

ConvData ConvData::reason(int turn_count) const {
  // Python stores reasoning newest-first; the dedupe checks `reasoning[-1] == turn_count`,
  // which corresponds to the *oldest* entry (since Python's `tuple[-1]` indexes the last
  // tuple position, but the construction `(turn_count, *self.reasoning)` keeps newest at [0]).
  // Faithful port: dedupe on the last (oldest) entry.
  if (!reasoning.empty() && reasoning.back() == turn_count) return *this;
  ConvData out = *this;
  out.reasoning.insert(out.reasoning.begin(), turn_count);
  return out;
}

ConvData ConvData::signal(int turn_count) const {
  if (signal_turn) return *this;
  ConvData out = *this;
  out.signal_turn = turn_count;
  return out;
}

}  // namespace hanabi
