# Project notes for Claude

## Bot version

The bot's build version lives in `include/hanabi/version.h` as `kBotVersion`.
When a game starts, the bot publishes that version as a note on card order 0
so observers can confirm which build is running.

**Every deployed change must bump `kBotVersion`** (patch bump for fixes, minor
bump for behavioural changes). After making the change, **write the new version
number in the change summary** you return to the user.

**Every version bump must be committed and pushed to `origin/master`** with a
commit message summarising the changes. Use a HEREDOC for multi-line messages.
Do not skip hooks (`--no-verify`) or bypass signing. Confirm with the user
before pushing if there's any ambiguity about whether the change is ready to
deploy.

## Test changes

The `tests/` tree captures behavioural expectations for the bot. Before
modifying or deleting **any existing test** (anything under `tests/`),
enumerate the proposed changes (file, test name, what assertion or fixture
moves) and **wait for approval**. Adding new tests is fine without approval.
Mechanical, behaviour-neutral edits (renames, comment-only changes) should
still be listed before being applied.
