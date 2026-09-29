# Human-vs-bot diagnostics

A repository of games in which an expert human played alongside the bot and then
walked through where the bot's play fell short of what a strong human would do.
Each document holds the human's reasoning verbatim, next to what the bot did, why
the code made it do that, and what (if anything) was changed as a result.

These documents are the **motivating examples** for convention and decision-making
changes. A change to what a clue means (`src/conventions/<name>/CONVENTION.md`) or to
how the bot chooses its move (`DECISION_MAKING.md`, or `CONVENTION.md` §2 for
reactor) should be aligned with the human diagnostics here. It should cite the
document and turn it answers, both in the commit message and next to the rule it
adds or changes. A proposed change that makes the bot play *against* a diagnostic
here is a regression, even if some other measure improves.

Unlike self-play numbers, a diagnostic says what good play *is*. Self-play
(`self_play_diagnostics/`) is used only to check that a change doesn't damage play
elsewhere.

## Index

| Game | Variant | Human seat | Items | Status |
|---|---|---|---|---|
| [2013726](2013726.md) | Throw It in a Hole & Brown (4 Suits), tiiah | yagami_black | T3, T5, T17, T27, T30, T38 (sample line) | in progress |

## Template

Name the file `<database_id>.md`, and add a row to the index.

```markdown
# <database_id> — <variant>

Replay: https://hanab.live/shared-replay/<database_id>
Seats: 0 <name> (human | bot vX.Y.Z), 1 …, 2 …
Convention: <tiiah | reactor0 | reactor>
Logs: logs/<bot>-<database_id>.log

## The human's analysis (verbatim)

> …the reviewer's words, unedited…

## Items

### T<N> — <one-line summary>

- **Human:** what a strong player does here, and why (a short paraphrase; the
  verbatim text is above).
- **Bot:** the action, the DECIDE rung (`scripts/show_turn.py`), and
  `scripts/tiiah_stacks.py` output for TIIAH.
- **Cause:** the rule or its absence, with `file:line`.
- **Resolution:** the version, the rule changed, the regression test, and the
  before/after of `build/replay_log.exe … --rerun`.
- **Status:** open | fixed in vX.Y.Z | not to be fixed (ruling)
```
