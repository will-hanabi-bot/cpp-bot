// Which convention a Game interprets clues under. There is one enum value
// per directory in src/conventions/ that implements a full convention
// (reactor, reactor0); the variants/ helpers are shared.
//
// The Game field defaults to REACTOR so that historical snapshots (which
// predate the field) and existing tests replay under the convention they
// were played with. The live default for NEW games is BotClient's
// convention_mode_, seeded from BotConfig::convention.
#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace hanabi {

enum class Convention : std::uint8_t {
  REACTOR,
  REACTOR0,
};

// Stable wire/log name: "reactor" / "reactor0".
std::string_view convention_name(Convention c);

// Parse a user- or config-supplied name. Accepts the log names plus the
// legacy config spelling ("Reactor1") and simple case variants. Unknown
// strings return nullopt — callers decide whether that is silent (chat
// commands sharing a namespace with other bot families) or a warning
// (config).
std::optional<Convention> parse_convention(std::string_view s);

}  // namespace hanabi
