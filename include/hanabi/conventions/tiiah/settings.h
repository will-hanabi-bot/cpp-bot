// What `/settings` prints at a Throw It in a Hole table.
//
// Its own line rather than reactor0's: the even/odd split reactor0 reports is
// the one thing this convention does not have (every reactive is even). The text
// is the user's, verbatim (v24.0.0), one line for five or six non-inverted suits
// (the epoch shift) and one for three or four; the first is 299 characters, the
// chat limit (`net::kMaxChatLength`).
#pragma once

#include <string>

namespace hanabi {
struct Variant;
}

namespace hanabi::tiiah {

std::string format_settings(const Variant& variant);

}  // namespace hanabi::tiiah
