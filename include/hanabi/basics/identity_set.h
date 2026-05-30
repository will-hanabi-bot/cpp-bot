// Port of python-bot/src/hanabi_bot/basics/identity.py (IdentitySet portion).
// Original Scala: scala-bot/src/scala_bot/basics/IdentitySet.scala.
//
// Bit-packed set of Identity values. Wraps a uint64_t; bit k corresponds to
// Identity.from_ord(k). Operator overloads return IdentitySet so the type is
// preserved across &/|/^/-.
#pragma once

#include <bit>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "hanabi/basics/identity.h"

namespace hanabi {

class IdentitySet {
 public:
  using Bits = std::uint64_t;

  constexpr IdentitySet() : bits_(0) {}
  constexpr explicit IdentitySet(Bits b) : bits_(b) {}

  static constexpr IdentitySet empty() { return IdentitySet(0); }
  static constexpr IdentitySet single(Identity id) {
    return IdentitySet(Bits{1} << id.to_ord());
  }

  template <typename Range>
  static constexpr IdentitySet from_iter(const Range& r) {
    Bits b = 0;
    for (Identity id : r) b |= Bits{1} << id.to_ord();
    return IdentitySet(b);
  }

  static constexpr IdentitySet from_iter(std::initializer_list<Identity> ids) {
    Bits b = 0;
    for (Identity id : ids) b |= Bits{1} << id.to_ord();
    return IdentitySet(b);
  }

  // Build a set of every Identity (up to max_ids ordinals) satisfying cond.
  template <typename Cond>
  static IdentitySet create(Cond&& cond, int max_ids = kMaxOrd) {
    Bits b = 0;
    for (int i = 0; i < max_ids; ++i) {
      if (cond(Identity::from_ord(i))) b |= Bits{1} << i;
    }
    return IdentitySet(b);
  }

  constexpr Bits bits() const { return bits_; }

  // Cardinality / emptiness.
  constexpr int length() const { return std::popcount(bits_); }
  constexpr bool is_empty() const { return bits_ == 0; }
  constexpr bool non_empty() const { return bits_ != 0; }

  // Element access.
  Identity head() const {
    if (bits_ == 0) throw std::out_of_range("head of empty IdentitySet");
    return Identity::from_ord(std::countr_zero(bits_));
  }

  constexpr bool is_exactly(Identity id) const {
    return bits_ == (Bits{1} << id.to_ord());
  }

  constexpr bool contains(Identity id) const {
    return (bits_ & (Bits{1} << id.to_ord())) != 0;
  }

  // Iteration: yields identities in ascending ordinal order.
  class iterator {
   public:
    using value_type = Identity;
    using difference_type = std::ptrdiff_t;
    using reference = Identity;
    using pointer = void;
    using iterator_category = std::input_iterator_tag;

    iterator() : remaining_(0) {}
    explicit iterator(Bits remaining) : remaining_(remaining) {}

    Identity operator*() const {
      return Identity::from_ord(std::countr_zero(remaining_));
    }
    iterator& operator++() {
      remaining_ &= remaining_ - 1;
      return *this;
    }
    iterator operator++(int) {
      iterator copy = *this;
      ++*this;
      return copy;
    }
    bool operator==(const iterator& o) const { return remaining_ == o.remaining_; }
    bool operator!=(const iterator& o) const { return remaining_ != o.remaining_; }

   private:
    Bits remaining_;
  };

  iterator begin() const { return iterator(bits_); }
  iterator end() const { return iterator(0); }

  std::vector<Identity> to_list() const {
    std::vector<Identity> out;
    out.reserve(length());
    for (Identity id : *this) out.push_back(id);
    return out;
  }

  // Bitwise operators (preserve subclass).
  constexpr IdentitySet operator&(IdentitySet o) const { return IdentitySet(bits_ & o.bits_); }
  constexpr IdentitySet operator|(IdentitySet o) const { return IdentitySet(bits_ | o.bits_); }
  constexpr IdentitySet operator^(IdentitySet o) const { return IdentitySet(bits_ ^ o.bits_); }
  constexpr IdentitySet operator~() const { return IdentitySet(~bits_); }
  constexpr IdentitySet operator-(IdentitySet o) const { return IdentitySet(bits_ & ~o.bits_); }

  IdentitySet& operator&=(IdentitySet o) { bits_ &= o.bits_; return *this; }
  IdentitySet& operator|=(IdentitySet o) { bits_ |= o.bits_; return *this; }
  IdentitySet& operator^=(IdentitySet o) { bits_ ^= o.bits_; return *this; }
  IdentitySet& operator-=(IdentitySet o) { bits_ &= ~o.bits_; return *this; }

  constexpr bool operator==(const IdentitySet&) const = default;

  // High-level set ops (Scala-style names; non-mutating).
  constexpr IdentitySet add(Identity id) const {
    return IdentitySet(bits_ | (Bits{1} << id.to_ord()));
  }
  constexpr IdentitySet remove(Identity id) const {
    return IdentitySet(bits_ & ~(Bits{1} << id.to_ord()));
  }
  constexpr IdentitySet union_with(IdentitySet o) const { return *this | o; }
  constexpr IdentitySet union_with(Identity id) const { return add(id); }
  template <typename Range>
  IdentitySet union_with(const Range& r) const { return *this | from_iter(r); }

  constexpr IdentitySet intersect(IdentitySet o) const { return *this & o; }
  template <typename Range>
  IdentitySet intersect(const Range& r) const { return *this & from_iter(r); }

  constexpr IdentitySet difference(IdentitySet o) const { return *this - o; }
  constexpr IdentitySet difference(Identity id) const { return remove(id); }
  template <typename Range>
  IdentitySet difference(const Range& r) const { return *this - from_iter(r); }

  // Functional predicates / transforms.
  template <typename Cond>
  IdentitySet filter(Cond&& cond) const {
    Bits b = bits_;
    Bits scan = bits_;
    while (scan != 0) {
      int lsb = std::countr_zero(scan);
      scan &= scan - 1;
      if (!cond(Identity::from_ord(lsb))) b &= ~(Bits{1} << lsb);
    }
    return IdentitySet(b);
  }

  template <typename Cond>
  bool forall(Cond&& cond) const {
    for (Identity id : *this) {
      if (!cond(id)) return false;
    }
    return true;
  }

  template <typename Cond>
  bool exists(Cond&& cond) const {
    for (Identity id : *this) {
      if (cond(id)) return true;
    }
    return false;
  }

  template <typename Cond>
  std::optional<Identity> find(Cond&& cond) const {
    for (Identity id : *this) {
      if (cond(id)) return id;
    }
    return std::nullopt;
  }

  template <typename Cond>
  int count(Cond&& cond) const {
    int n = 0;
    for (Identity id : *this) {
      if (cond(id)) ++n;
    }
    return n;
  }

  constexpr IdentitySet when_empty(IdentitySet alternative) const {
    return is_empty() ? alternative : *this;
  }

  std::string to_string() const;

 private:
  Bits bits_;
};

}  // namespace hanabi
