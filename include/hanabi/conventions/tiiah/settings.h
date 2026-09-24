// What `/settings` prints at a Throw It in a Hole table.
//
// Its own line rather than reactor0's: the even/odd split reactor0 reports is
// the one thing this convention does not have (every reactive is even), and the
// two things a reader most needs here — the buckets and the reverse-reactive
// dispatch — do not appear in reactor0's line at all.
#pragma once

#include <string>

namespace hanabi {
struct Variant;
}

namespace hanabi::tiiah {

std::string format_settings(const Variant& variant);

}  // namespace hanabi::tiiah
