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

## Debugging a bug report

When a bug report arrives with `(game_id, turn, expected vs actual)`:

1. **Look for an existing log first.** Per-game structured logs live at
   `logs/{bot_name}-{game_id}.log`. Use:
   ```
   scripts/find_game.sh <game_id>
   ```
   If a log exists, **do NOT re-simulate from turn 1**. The log already
   captures the state and the decision branch the bot took.

2. **Inspect the turn in the log.**
   ```
   scripts/show_turn.py logs/<bot>-<game_id>.log <turn>
   ```
   The STATE record shows the exact game state (stacks, hands w/ empathy
   decoded, meta, waiting connections). The DECIDE trace shows which
   convention branches fired and with what inputs. The TIMING line shows
   where the turn spent its time.

3. **Re-run with current code.**
   ```
   build/replay_log logs/<bot>-<game_id>.log --turn <N> --rerun
   ```
   Reconstructs the Game from the snapshot and calls take_action() with
   the build you have. If the action still mismatches the bug report,
   the bug is in current main and you can iterate by editing → rebuilding
   → re-running replay_log (1-second loop).

   If the rerun action now MATCHES the expected action, the prior log was
   from a build that already had the bug fixed — diff `kBotVersion`s.

4. **Generate a regression test only after you understand the branch.**
   ```
   scripts/bug_to_test.sh logs/<bot>-<game_id>.log <turn>
   ```
   Emits `tests/test_endgame/test_replay_<game_id>.cpp`, builds, and runs
   it. Manual replay-test authoring (typing out the deck + action sequence
   in the test_endgame/replay_helpers.h style) is the **last** resort, not
   the first — only fall back when no log exists.

Per-game logs are also useful for "what did the bot spend its time on"
investigations. `scripts/log_summary.py logs/<bot>-<game_id>.log` prints
the per-turn action + the per-game TIMING aggregate.
