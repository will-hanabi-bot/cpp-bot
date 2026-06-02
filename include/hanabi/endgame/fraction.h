// Custom Fraction<int64_t> for the endgame solver.
//
// Lazy GCD reduction: only normalize on construction-from-raw and on
// comparison. Avoids the per-operation GCD overhead of Python's Fraction
// while preserving exact arithmetic.
//
// Per the plan's "fidelity vs. perf" decision: Hanabi's max_score / cards_left
// stay well within int64 range across recursion depth ~10, so int64
// numerator+denominator is sufficient.
#pragma once

#include <cstdint>
#include <numeric>
#include <stdexcept>

namespace hanabi::endgame {

struct Fraction {
  std::int64_t num;
  std::int64_t den;

  constexpr Fraction() : num(0), den(1) {}
  constexpr Fraction(std::int64_t n) : num(n), den(1) {}
  Fraction(std::int64_t n, std::int64_t d) {
    if (d == 0) throw std::invalid_argument("Fraction denominator must be non-zero");
    if (d < 0) {
      n = -n;
      d = -d;
    }
    std::int64_t g = std::gcd(n < 0 ? -n : n, d);
    num = n / g;
    den = d / g;
  }

  static constexpr Fraction zero() { return Fraction(0); }
  static constexpr Fraction one() { return Fraction(1); }

  // All arithmetic ops normalize via the validating ctor so the invariant
  // den > 0 (and gcd(|num|,den) = 1) holds end-to-end. The earlier "lazy
  // reduce" scheme caused int64 overflow during deep accumulation chains
  // (arrangement.prob * many factors, then sum across arrangements) — the
  // overflowed denominator could wrap to 0 and trip the assert below.
  Fraction operator+(Fraction o) const {
    return Fraction(num * o.den + o.num * den, den * o.den);
  }
  Fraction operator-(Fraction o) const {
    return Fraction(num * o.den - o.num * den, den * o.den);
  }
  Fraction operator*(Fraction o) const {
    return Fraction(num * o.num, den * o.den);
  }
  Fraction operator/(Fraction o) const {
    if (o.num == 0) throw std::invalid_argument("Fraction division by zero");
    return Fraction(num * o.den, den * o.num);
  }
  Fraction& operator+=(Fraction o) { return *this = *this + o; }
  Fraction& operator-=(Fraction o) { return *this = *this - o; }
  Fraction& operator*=(Fraction o) { return *this = *this * o; }
  Fraction& operator/=(Fraction o) { return *this = *this / o; }
  constexpr Fraction operator-() const { return Fraction::raw(-num, den); }

  // Cross-multiply comparisons. den > 0 always.
  constexpr bool operator==(Fraction o) const { return num * o.den == o.num * den; }
  constexpr bool operator!=(Fraction o) const { return !(*this == o); }
  constexpr bool operator<(Fraction o) const { return num * o.den < o.num * den; }
  constexpr bool operator<=(Fraction o) const { return num * o.den <= o.num * den; }
  constexpr bool operator>(Fraction o) const { return num * o.den > o.num * den; }
  constexpr bool operator>=(Fraction o) const { return num * o.den >= o.num * den; }

  constexpr double to_double() const {
    return static_cast<double>(num) / static_cast<double>(den);
  }

 private:
  static constexpr Fraction raw(std::int64_t n, std::int64_t d) {
    Fraction f;
    f.num = n;
    f.den = d;
    return f;
  }
};

}  // namespace hanabi::endgame
