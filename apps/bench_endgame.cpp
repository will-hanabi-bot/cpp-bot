// Microbenchmark for endgame solver primitives.
//
// Each op uses `i` (or a varying value) so the compiler can't hoist it out
// of the loop. Reports nanoseconds per op.
#include <chrono>
#include <cstdio>

#include "hanabi/basics/identity.h"
#include "hanabi/basics/identity_set.h"
#include "hanabi/endgame/fraction.h"

namespace {

using clock_t_ = std::chrono::steady_clock;

template <typename F>
double time_ns(int iterations, F&& f) {
  auto t0 = clock_t_::now();
  for (int i = 0; i < iterations; ++i) f(i);
  auto t1 = clock_t_::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / iterations;
}

}  // namespace

int main() {
  using namespace hanabi;
  using namespace hanabi::endgame;

  volatile std::uint64_t sink = 0;

  // --- IdentitySet ---
  // Build varying sets so each op has unique operands.
  IdentitySet base = IdentitySet::from_iter({Identity(0, 1), Identity(0, 2), Identity(1, 3),
                                                Identity(2, 4), Identity(3, 5)});

  double ns_and = time_ns(10'000'000, [&](int i) {
    IdentitySet x(base.bits() ^ (1ull << (i % 29)));
    sink = (base & x).bits();
  });
  double ns_or = time_ns(10'000'000, [&](int i) {
    IdentitySet x(base.bits() ^ (1ull << (i % 29)));
    sink = (base | x).bits();
  });
  double ns_iter = time_ns(1'000'000, [&](int i) {
    IdentitySet x(base.bits() ^ (1ull << (i % 29)));
    int n = 0;
    for (Identity id : x) {
      sink += static_cast<std::uint64_t>(id.to_ord());
      ++n;
    }
    sink ^= n;
  });

  // --- Fraction ---
  double ns_fadd = time_ns(10'000'000, [&](int i) {
    Fraction f1(i + 1, (i & 31) + 2);
    Fraction f2(i + 3, (i & 15) + 5);
    sink = static_cast<std::uint64_t>((f1 + f2).num);
  });
  double ns_fmul = time_ns(10'000'000, [&](int i) {
    Fraction f1(i + 1, (i & 31) + 2);
    Fraction f2(i + 3, (i & 15) + 5);
    sink = static_cast<std::uint64_t>((f1 * f2).num);
  });
  double ns_fcmp = time_ns(10'000'000, [&](int i) {
    Fraction f1(i + 1, (i & 31) + 2);
    Fraction f2(i + 3, (i & 15) + 5);
    sink = static_cast<std::uint64_t>(f1 < f2);
  });

  std::printf("IdentitySet & :  %7.2f ns/op\n", ns_and);
  std::printf("IdentitySet | :  %7.2f ns/op\n", ns_or);
  std::printf("IdentitySet iter (~5 ids): %7.2f ns/op\n", ns_iter);
  std::printf("Fraction + (with construction):  %7.2f ns/op\n", ns_fadd);
  std::printf("Fraction * (with construction):  %7.2f ns/op\n", ns_fmul);
  std::printf("Fraction <:                       %7.2f ns/op\n", ns_fcmp);
  std::printf("(sink: %llu)\n", static_cast<unsigned long long>(sink));
  return 0;
}
