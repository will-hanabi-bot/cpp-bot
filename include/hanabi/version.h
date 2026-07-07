#pragma once

namespace hanabi {

// Bumped manually with every deployed change. The bot publishes this on
// card order 0 as soon as a game starts, so observers can verify which
// build a will-bot is running. Don't forget to mention the new version
// in the change summary, per CLAUDE.md.
inline constexpr const char* kBotVersion = "v1.7.0";

}  // namespace hanabi
