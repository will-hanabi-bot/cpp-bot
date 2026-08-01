# Project notes for Claude

## Bot version

The bot's build version lives in `include/hanabi/version.h` as `kBotVersion`.
When a game starts, the bot publishes that version as a note on card order 0
so observers can confirm which build is running.

**Every deployed change must bump `kBotVersion`** (patch bump for fixes, minor
bump for behavioural changes, **major bump for a cross-version compatibility
break** — changing the default convention, or anything else that changes what
existing clues mean to partners running an older build). After making the
change, **write the new version number in the change summary** you return to
the user.

**Every version bump must be committed and pushed to `origin/master`** with a
commit message summarising the changes. Use a HEREDOC for multi-line messages.
Do not skip hooks (`--no-verify`) or bypass signing. Confirm with the user
before pushing if there's any ambiguity about whether the change is ready to
deploy.

**On every version bump, check that the repo's `.md` docs are still accurate**
(each convention's `CONVENTION.md` / `GLOSSARY.md` under
`src/conventions/<name>/`, plus `README.md`, `TODO.md`, `VERIFICATION.md`, this
file, and any other `.md` describing implementation status). Docs or code
comments that claim work is "pending", "stubbed", or "Phase N" must be updated or
marked historical once the work has shipped. VERIFICATION.md previously went
stale this way — treat any status-bearing `.md` as part of the change surface.

`TODO.md` lists convention that is legal but not implemented. When a change
closes one of its entries, delete the entry and update the `CONVENTION.md` /
`GLOSSARY.md` wording that pointed at it in the same commit.

## Convention documentation

Each convention owns its docs: `src/conventions/<name>/CONVENTION.md` is the
**ruling reference** for what a clue means and how the bot decides under that
convention, and `src/conventions/<name>/GLOSSARY.md` defines its terminology.
Both are written for a reader with no prior context, and both cite `file:line`
for every rule so that a claim can be checked against the code. Which
convention a game runs is `Game::convention`; replay tests replay under the
convention recorded in their snapshot.

**Every version bump that changes clue interpretation or decision-making must
update the `CONVENTION.md` of every convention whose behaviour changed, in the
same commit.** A change to shared engine code (the `decide.cpp` seam, `elim`,
the eval layer) updates all conventions' docs. Specifically:

- Changing an `interpret_*` branch, a target-selection pool or its ordering, a
  variant rule, or a convention's dispatcher → update the corresponding
  rule in that convention's **§1 Convention**, including its `file:line`
  citation and the replay that motivated it.
- Changing `eval_action` / `get_result` / `advance` / `eval_state` /
  `eval_game` terms, the `take_action` ladder, gates, thresholds, or the
  endgame solver's parameters → update **§2 Decision Making** (shared by all
  conventions; each convention's doc states its deltas).
- Introducing a new term, status, flag, or role → add it to the owning
  convention's `GLOSSARY.md`.
- Line numbers shift constantly. When editing a documented file, re-check the
  citations for the rules in it, not just the rule you changed.

A `CONVENTION.md` that disagrees with the code is a bug in one of them. It is
the document a human or a fresh agent is expected to read *instead of* reading
thousands of lines of `src/conventions/`, so silent drift is expensive. If a
change makes a documented rule obsolete, delete the rule — do not leave it
alongside its replacement.

## Running tests

Tests are split by convention so a scoped run is possible. **Run the test
binaries directly** — `ctest` spawns one process per test, which costs ~1 s
each and makes the full suite take ~7 minutes instead of ~23 seconds.

```bash
cmake --build build -j --target hanabi_reactor0_tests   # build what you need
build/hanabi_reactor0_tests.exe   # reactor0 only          74 tests, 6.1 s
build/hanabi_tests.exe            # convention-neutral    248 tests, 0.2 s
build/hanabi_reactor_tests.exe    # reactor + replays     120 tests, 20 s
build/hanabi_decision_tests.exe   # decision quality       12 tests
```

Pick the scope from the report's `Convention:` field:

| Working on | Run |
|---|---|
| reactor0 | `hanabi_reactor0_tests` + `hanabi_tests` (6.3 s) |
| reactor | `hanabi_reactor_tests` + `hanabi_tests` (20 s) |
| shared engine (`src/basics/`, eval, elim) | all three |

`ctest` remains available when you want label composition or CI-style output:

```bash
ctest --test-dir build --output-on-failure -L '^(core|reactor0)$'
ctest --test-dir build --output-on-failure -LE decision_making   # full correctness
ctest --test-dir build --output-on-failure -L  decision_making   # quality
```

Note `-L` takes a **regex**: unanchored `-L reactor` also matches `reactor0`
(160 tests, not 122). Always anchor as `^reactor$`.

Two rules:

- **The scoped run is a fast inner loop, not the gate.** Run the full
  correctness suite before committing — reactor0 leans on shared machinery
  (`target_play`, `ref_discard`, `elim`, `chop`, snapshots), so a change that
  looks convention-local can break the other convention's corpus.
- On Windows a running test binary is locked, so a rebuild cannot relink it
  while it is executing. Let the run finish first (see README §5.7).

## Test changes

The `tests/` tree captures behavioural expectations for the bot. Before
modifying or deleting **any existing test** (anything under `tests/`),
enumerate the proposed changes (file, test name, what assertion or fixture
moves) and **wait for approval**. Adding new tests is fine without approval.
Mechanical, behaviour-neutral edits (renames, comment-only changes) should
still be listed before being applied.

## Debugging a bug report

When a bug report arrives with `(game_id, turn, expected vs actual)`:

0. **Establish which convention it is about.** This picks the tests you run,
   the `CONVENTION.md` you update, and the folder a regression test goes in.
   In order of authority:
   - the log's `game_init` record or any STATE record's `replay.convention`
     field — this is what *actually ran*, so it wins;
   - the report's `Convention:` field;
   - otherwise assume `reactor0` (the live default). Note a 4+ player game
     always runs reactor regardless of the selected convention.

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
   the convention's `test_misc` folder with no slug), builds, and runs it. It reads the
   convention from the log and builds/runs the matching target, so a reactor0
   log exercises `hanabi_reactor0_tests`. Manual replay-test authoring (typing
   out the deck + action sequence in the tests/replay_helpers.h style)
   is the **last** resort, not the first — only fall back when no log exists.

Per-game logs are also useful for "what did the bot spend its time on"
investigations. `scripts/log_summary.py logs/<bot>-<game_id>.log` prints
the per-turn action + the per-game TIMING aggregate.

## Replay-test standards

- **Convention field.** Bug reports carry a `Convention` field
  (`reactor0` | `reactor`) alongside `Category`; resolve it per step 0 of the
  debugging workflow. It decides which CMake target the new test is wired
  into, and therefore which scoped run covers it.
- **Category folders.** Bug reports carry a `Category` field naming the
  folder the regression test belongs to. Categories are convention-qualified:
  the test goes in `tests/test_reactor/<category>/` (e.g.
  `tests/test_reactor/test_bad_reactive_target/`) or
  `tests/test_reactor0/<category>/`, defaulting to that convention's
  `test_misc/` when the report names no category. Create the folder if it
  doesn't exist and add the new `.cpp` to the **matching target's** source
  list in `CMakeLists.txt` — `hanabi_reactor0_tests` for reactor0,
  `hanabi_reactor_tests` for reactor, `hanabi_tests` only for a
  convention-neutral engine test. **The folder decides the target**: anything
  under `tests/test_reactor/` belongs to `hanabi_reactor_tests`. No other
  wiring is needed.
  **One exception**: `test_decision_making/` under either convention
  (`tests/test_reactor/test_decision_making/`, and
  `tests/test_reactor0/test_decision_making/` once its first test is written)
  belongs to `hanabi_decision_tests`, which carries the ctest label
  `decision_making` so quality failures can be excluded from a correctness
  run. These tests are convention-specific — the existing ones are all built
  on reactor — but they share one target because they share that label.
  A reactor0 replay test must also carry `"convention": "reactor0"` in its
  snapshot, or `apply_snapshot` will replay it under reactor (the
  missing-key default, which keeps historical logs correct).
- **Descriptive filenames.** Replay tests are named
  `test_replay_<game_id>_<short_slug>.cpp`, where the slug is a snake_case
  3–6 word description of the issue, e.g.
  `test_replay_1234567_focus_clued_trash_over_unclued_trash.cpp`.
- **Export JSONs.** hanab.live export JSONs live in the central store
  `tests/replays/<game_id>.json` regardless of category. Nothing reads them at
  runtime — decks are hardcoded in each `.cpp` — they are a human reference
  corpus.
- **Shared fixtures.** `tests/replay_helpers.h` (included as
  `#include "replay_helpers.h"`) provides `OrigAction` / `ReplayContext` /
  `apply_orig_action`. `replay_log --emit-test` generates that include, so
  moving the header means updating `apps/replay_log.cpp` in the same commit.
- **Suite names track the folder.** `tests/test_reactor/test_misc/` uses
  `MiscReplay<id>`, `test_reactor/test_endgame/` uses `EndgameReplay<id>`, and
  the category folders use their category (`BadReaction1915981`,
  `StackedPlays1916815`). Keep them in step when adding or moving a test.
