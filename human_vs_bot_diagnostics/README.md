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
| [2013726](2013726.md) | Throw It in a Hole & Brown (4 Suits), tiiah | yagami_black | T3, T5, T17, T27, T30, T38 (sample line) | T30 fixed (v18.3.0), T27 (v18.4.0), T38 (v18.5.0), T17 (v18.7.0); T27 reacter reading (v18.9.0); open: T27 shared-view booking; T3 and T5 need no change |
| [2013963](2013963.md) | Throw It in a Hole & Pink (5 Suits), tiiah | yagami_black | T10 (a reverse-reactive finesse on a settled call) | fixed (v18.10.0) |
| [2014076](2014076.md) | Throw It in a Hole & Brown (6 Suits), tiiah | yagami_light | T14 (deferred collapse on a re-touched clued card), T18 (occupied Alice saves Bob's playable chop) | T14 fixed (v18.12.0), T18 (v18.13.0) |
| [2014538](2014538.md) | Throw It in a Hole & Brown (6 Suits), tiiah | yagami_black | T21 (clue economy at 2 clues with a player locked), T23 (a colour stable play over a rank stall, role inversion), T24 (a stable play before locking a stuck Bob) | T21 recorded only (ruling); T23 fixed (v18.15.0), T24 (v18.16.0) |
| [2014561](2014561.md) | Throw It in a Hole & Brown (6 Suits), tiiah | yagami_black | T49 (tiebreak stable plays by the receiver's inferences), T50 (a four-step stable play hierarchy), T56 (a named call past the own-dupe veto), T66 (endgame: single out Bob's, then Cathy's, good card) | T49 no change (ruling); T66 verified; T50 fixed (v18.17.0), T56 (v18.18.0) |
| [2018365](2018365.md) | Throw It in a Hole & Null (6 Suits), tiiah | yagami_black | T7 (no lock over a chop worse than one away), T10 (no forced clue at 8 tokens while Alice can play) | T7 fixed (v20.2.0), T10 (v20.2.0) |
| [2018541](2018541.md) | Throw It in a Hole & Rainbow (5 Suits), tiiah | yagami_black | T26 (a reacter card playable in a hole world: the Actionable Superposition Collapse Rule) | fixed (v20.6.0) |
| [2018759](2018759.md) | Throw It in a Hole & Rainbow (5 Suits), tiiah | yagami_black | T34 (a spent reaction stays urgent and blocks a save of Bob's playable chop) | fixed (v20.7.0) |
| [2025310](2025310.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah | yagami_black | T3 (Alice plays her known card; Bob sees to Cathy's playable chop) | fixed (v23.11.0) |
| [2025422](2025422.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah | yagami_black | T28-T33 (a call whose team reading lands elsewhere stays called as any playable) | fixed (v23.12.0) |
| [2025452](2025452.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah | yagami_black | T38 (black's playable r1; T18-T20 a bucket impossible for the card reads any playable over the worlds) | fixed (v23.12.0) |
| [2025488](2025488.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah | yagami_black | T50 (the called play in a thin endgame), T55 (o42 reads {g5,y5}), T60 (the positional double discard) | T50 fixed (v23.13.0), T55 open (live/replay divergence), T60 fixed (v23.13.0) |
| [9000002](9000002.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T45 (a reactive both seats can name) | fixed (v23.14.0) |
| [9000009](9000009.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T10 (general: unlock a locked Bob with a play clue), T29 (a reactive both can name) | fixed (v23.14.0) |
| [9000022](9000022.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T19 (special-suit preference), T39 (play clue over trash reveal), T58 (reacter plays the y5) | fixed (v23.14.0) |
| [9000046](9000046.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T57 (special-suit preference) | fixed (v23.14.0) |
| [9000085](9000085.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T52 (urgent endgame reactive) | fixed (v23.14.0) |
| [9000004](9000004.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T11 (illegal stable Red calls an r5 as an r2) | fixed (v23.15.0) |
| [9000118](9000118.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T7-T8 (the giver does not model the reacter's ASCR) | fixed (v23.15.0) |
| [9000126](9000126.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T14-T16 (reverse reactive ignored), T28 (a 4 to play the g1 ignored) | fixed (v23.15.0) |
| [9000174](9000174.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T14 (known r2 after reverse reactives ignored) | fixed (v23.15.0) |
| [2026727](2026727.md) | Throw It in a Hole & Dark Prism (6 Suits), tiiah | yagami_black | T9 (the ordinary reactive's target under stack simulation of the receiver's known plays) | fixed (v23.29.0) |
| [9000080](9000080.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T44–T51 (an endgame reactive has the reacter misname its b2 as the y4) | fixed (v23.17.0) |
| [9000007 (v23.22)](9000007_23_22.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T48 (High Score Phase: a play clue leading into other hands) | fixed (v23.26.0) |
| [9000069 (v23.22)](9000069_23_22.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T56 (a late bucket break made safe by the giver's own named card) | fixed (v23.25.0) |
| [9000081 (v23.22)](9000081_23_22.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T51 (1 to sim-bob, both know their cards; at least Blue, a lead-in) | the 1 fixed (v23.23.0, pinned v23.25.0); Blue a HIGH lead-in (v23.26.0) |
| [9000096 (v23.22)](9000096_23_22.md) | Throw It in a Hole & Dark Null (6 Suits), tiiah (self-play) | — | T59 (forced endgame: discard so the seat after next gets two turns) | fixed (v23.27.0) |
| [9000016 (v23.29)](9000016_23_29.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T2 (no clue that gets a second copy of a called card played) | fixed (v23.29.0) |
| [2026832](2026832.md) | Throw It in a Hole & Dark Prism (6 Suits), tiiah | yagami_black | T7–T11 (a play call replaces a discard call, at the receiver too) | fixed (v23.30.0) |
| [9000159 (v23.30, scream experiment)](9000159_23_30.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T36 (a play clue on a critical, playable chop over a lock), T38 (a locked Bob throws the card he sees again) | fixed (v23.33.0) |
| [2027469](2027469.md) | Throw It in a Hole & Gray Pink (6 Suits), tiiah | yagami_black | T57 (no stall that plays nothing while Bob holds a critical card and no play) | fixed (v23.35.0) |
| [9000115 (v23.33, no-idle-stall candidate)](9000115_23_33.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T22 (a save of a good card on chop is no idle stall) | fixed (v23.35.0) |
| [9000091 (v23.33, no-idle-stall candidate)](9000091_23_33.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T53 (in the High Score Phase, a play clue that newly touches a playable card is at least MEDIUM) | open (Patch 1 A/B neutral, held) |
| [9000170 (v23.33, no-idle-stall candidate)](9000170_23_33.md) | Throw It in a Hole & Black (6 Suits), tiiah (self-play) | — | T58 (one card left, the seat after next needs two turns: a forced fill-in stall) | fixed (v23.35.0) |
| [2026856](2026856.md) | Throw It in a Hole & Dark Prism (6 Suits), tiiah | yagami_black | T47 (a play known by touch comes before a LOW clue) | fixed (v23.31.0) |
| [2026882](2026882.md) | Throw It in a Hole & Dark Prism (6 Suits), tiiah | yagami_black | T25 (save Cathy's critical chop while Bob is loaded) | fixed (v23.32.0) |
| [2027454](2027454.md) | Throw It in a Hole & Gray Pink (6 Suits), tiiah | yagami_black | T40 (a reactive whose bucket names a card the reacter sees twice), T41 (our blind play named by a card we see is gone), T46 (stack views past a critical card in sight) | T40 needs no change; T41 and T46 fixed (v23.34.0), with the stack invariant |
| [2027736](2027736.md) | Throw It in a Hole & Gray Pink (6 Suits), tiiah | yagami_black | T47-T48 (our own reaction card read by the team's set), T50 (the wrong call reset, a desync), T51 (the reverse reactive 2 to will-bot69) | fixed (v23.36.0) |
| [2027811](2027811.md) | Throw It in a Hole & Gray Pink (6 Suits), tiiah | yagami_black | T39-T40 (a known bucket violation the team's named y5 makes safe) | fixed (v23.37.0) |
| [2027922](2027922.md) | Throw It in a Hole & Dark Brown (6 Suits), tiiah | yagami_black | T22 (no VERY HIGH for a dark card the pending reaction already gets), T27 (a player beginning a turn with a token clears every other player's zero-clue marker) | fixed (v23.38.0) |
| [2028737](2028737.md) | Throw It in a Hole & Dark Orange (6 Suits), tiiah | yagami_black | T46 (an endgame play clue that leads into the other hands) | fixed (v23.39.0) |
| [2028742](2028742.md) | Throw It in a Hole & Dark Orange (6 Suits), tiiah | yagami_black | T13 (with Bob loaded, a stable play clue to Cathy saves her critical chop) | fixed (v23.40.0) |

## Template

Name the file `<database_id>.md`, and add a row to the index. A self-play game is
named after the build that played it as well, `<game_id>_<major>_<minor>.md` like its
exported JSON (from v23.25.0): every build numbers its games `9000000 + seed`, so the
id alone repeats across versions.

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
