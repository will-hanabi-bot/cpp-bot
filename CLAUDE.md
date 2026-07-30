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

**On every version bump, check that the repo's `.md` docs are still accurate**
(`CONVENTION.md`, `GLOSSARY.md`, `README.md`, `TODO.md`, `VERIFICATION.md`, this
file, and any other `.md` describing implementation status). Docs or code
comments that claim work is "pending", "stubbed", or "Phase N" must be updated or
marked historical once the work has shipped. VERIFICATION.md previously went
stale this way — treat any status-bearing `.md` as part of the change surface.

`TODO.md` lists convention that is legal but not implemented. When a change
closes one of its entries, delete the entry and update the `CONVENTION.md` /
`GLOSSARY.md` wording that pointed at it in the same commit.

## Convention documentation

`CONVENTION.md` is the **ruling reference** for what a clue means and how the
bot decides. `GLOSSARY.md` defines the project's terminology. Both are written
for a reader with no prior context, and both cite `file:line` for every rule so
that a claim can be checked against the code.

**Every version bump that changes clue interpretation or decision-making must
update `CONVENTION.md` in the same commit.** Specifically:

- Changing an `interpret_*` branch, a target-selection pool or its ordering, a
  variant rule, or the stable/reactive dispatcher → update the corresponding
  rule in **§1 Convention**, including its `file:line` citation and the replay
  that motivated it.
- Changing `eval_action` / `get_result` / `advance` / `eval_state` /
  `eval_game` terms, the `take_action` ladder, gates, thresholds, or the
  endgame solver's parameters → update **§2 Decision Making**.
- Introducing a new term, status, flag, or role → add it to `GLOSSARY.md`.
- Line numbers shift constantly. When editing a documented file, re-check the
  citations for the rules in it, not just the rule you changed.

A `CONVENTION.md` that disagrees with the code is a bug in one of them. It is
the document a human or a fresh agent is expected to read *instead of* reading
2 900 lines of `src/conventions/`, so silent drift is expensive. If a change
makes a documented rule obsolete, delete the rule — do not leave it alongside
its replacement.

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
   `logs/{bot_name}-{id}.log`. During play the id is the live **table id**;
   when the game concludes the server's `finishOngoingGame` message reveals
   the **database id** — the one replay links use — and the log is rewritten
   under it, with both ids stamped on every record (`game_id` = table id,
   `database_id` = replay id). So a `https://hanab.live/shared-replay/<id>`
   URL maps straight to a log. Use:
   ```
   scripts/find_game.sh <id>
   ```
   which accepts either id, and falls back to searching log contents and
   the `logs/bot-*.log` transcripts for logs written before v1.12.0. Those
   older logs can be renamed in bulk with
   `scripts/backfill_database_ids.sh --apply`.
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
   scripts/bug_to_test.sh logs/<bot>-<game_id>.log <turn> [category] [slug]
   ```
   Emits `tests/<category>/test_replay_<game_id>_<slug>.cpp` (defaults to
   `tests/test_endgame/` with no slug), builds, and runs it. Manual
   replay-test authoring (typing out the deck + action sequence in the
   test_endgame/replay_helpers.h style) is the **last** resort, not the
   first — only fall back when no log exists.

Per-game logs are also useful for "what did the bot spend its time on"
investigations. `scripts/log_summary.py logs/<bot>-<game_id>.log` prints
the per-turn action + the per-game TIMING aggregate.

## Replay-test standards

- **Category folders.** Bug reports carry a `Category` field naming the
  folder the regression test belongs to. The test goes in
  `tests/<category>/` (e.g. `tests/test_bad_reactive_target/`). Create the
  folder if it doesn't exist and add the new `.cpp` to the `hanabi_tests`
  source list in `CMakeLists.txt` — no other wiring is needed.
- **Descriptive filenames.** Replay tests are named
  `test_replay_<game_id>_<short_slug>.cpp`, where the slug is a snake_case
  3–6 word description of the issue, e.g.
  `test_replay_1234567_focus_clued_trash_over_unclued_trash.cpp`.
- **Export JSONs.** hanab.live export JSONs live in the central store
  `tests/test_endgame/replays/<game_id>.json` regardless of category.
- **Existing tests.** Uncategorized tests already in `tests/test_endgame/`
  stay there until explicitly recategorized (moving/renaming them is an
  existing-test change and needs approval, per "Test changes" above).
