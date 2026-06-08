# Project notes for Claude

## Bot version

The bot's build version lives in `include/hanabi/version.h` as `kBotVersion`.
When a game starts, the bot publishes that version as a note on card order 0
so observers can confirm which build is running.

**Every deployed change must bump `kBotVersion`** (patch bump for fixes, minor
bump for behavioural changes). After making the change, **write the new version
number in the change summary** you return to the user.
