// Which convention a Game interprets clues under. There is one enum value
// per directory in src/conventions/ that implements a full convention
// (reactor, reactor0, tiiah); the variants/ helpers are shared.
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
  // Throw It in a Hole. Forked from reactor0 and selected by the VARIANT rather
  // than by `/setall`, since only these variants can be played under it.
  TIIAH,
};

// Does this convention use reactor0's BELIEF machinery -- the no-widening
// escalation ladder, the missed-call policy, the no-reset-on-strike rule, call
// invariants, and the reaction-resolution paths?
//
// TIIAH forks from reactor0 and inherits all of it, so the shared engine sites
// must ask this rather than `== Convention::REACTOR0`. Written as a positive
// predicate deliberately: nearly every one of those sites used to be a NEGATIVE
// test against reactor0, which would silently drop a third convention onto
// reactor's side of the fork.
bool is_reactor0_family(Convention c);

// Does this convention use reactor0's DECISION layer -- `analyse_clues`,
// `choose_clue`, `choose_action`, and the endgame's reactor0-only preferences?
//
// False for TIIAH, and that is the point: those routines price clues by
// reactor0's meanings, so letting a TIIAH game into them is exactly the
// "played with the wrong rules" bug the convention exists to fix. Flipping this
// to true for TIIAH is the switch a later version throws once TIIAH has a
// decision layer of its own.
bool uses_reactor0_decisions(Convention c);

// Stable wire/log name: "reactor" / "reactor0".
std::string_view convention_name(Convention c);

// Parse a user- or config-supplied name. Accepts the log names plus the
// legacy config spelling ("Reactor1") and simple case variants. Unknown
// strings return nullopt — callers decide whether that is silent (chat
// commands sharing a namespace with other bot families) or a warning
// (config).
std::optional<Convention> parse_convention(std::string_view s);

}  // namespace hanabi
