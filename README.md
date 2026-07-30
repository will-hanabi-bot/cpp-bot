# cpp-bot — a Hanabi bot playing the Reactor convention

A C++20 bot that plays [Hanabi](https://hanab.live) on hanab.live using the
**Reactor** convention. It logs in over HTTPS, holds a WebSocket session, joins
or creates tables, and plays full games — including an exact-win-probability
endgame solver.

It is a port of an earlier Python bot (itself preceded by a Scala one). The
port was motivated by the endgame solver: exact `Fraction` arithmetic in
Python dominated the search's wall clock, and moving to C++ made the solver
roughly 50× faster per primitive operation. See
[VERIFICATION.md](VERIFICATION.md) for the historical benchmarks.

**Documentation map**

| File | What it is for |
|---|---|
| **README.md** (this file) | Overview, build, and run instructions |
| [CONVENTION.md](CONVENTION.md) | **The ruling reference** for what a clue means and how the bot decides |
| [GLOSSARY.md](GLOSSARY.md) | Every domain term, defined, with code references |
| [TODO.md](TODO.md) | Convention that is legal but not yet implemented |
| [CLAUDE.md](CLAUDE.md) | Working agreement for agents: version bumps, test policy, the bug-report workflow |
| [VERIFICATION.md](VERIFICATION.md) | Historical port-era verification notes and benchmarks |

---

## 1. The game, briefly

Hanabi is a cooperative, imperfect-information card game. Each player sees
everyone's hand **except their own**.

- **Deck**: 5 suits × ranks 1–5. Each suit has three 1s, two each of 2/3/4,
  and one 5 — so **every 5 is irreplaceable**, and a card becomes *critical*
  once it is the last useful copy.
- **Goal**: build one ascending stack per suit, 1→5. Final score is the sum of
  the stack heights, maximum 25.
- **On your turn**, do exactly one of:
  - **Play** a card. If it is the next card for its suit, the stack advances;
    otherwise it is a **strike**. Three strikes end the game at 0.
  - **Discard** a card, returning one **clue token** (8 maximum).
  - **Give a clue**, spending a token. A clue names one player and either one
    colour or one rank, and must point out **all** cards in that hand matching
    it, and at least one.
- Playing a 5 also returns a clue token.
- When the deck empties, every player gets one final turn.

The whole difficulty is that a clue's literal content ("these two cards are
red") is far less than what the team needs. Conventions assign *extra* agreed
meaning to a clue based on which cards it touched and the situation — that
shared meaning is what [CONVENTION.md](CONVENTION.md) specifies.

## 2. The Reactor convention in one page

[CONVENTION.md](CONVENTION.md) is authoritative; this is orientation.

Two orientation facts first:

- **Slot 1 is the leftmost, newest card** — draws prepend to the hand
  (`src/basics/game.cpp:378`).
- **Alice / Bob / Cathy** are positional: Alice is the player to move, Bob is
  next, Cathy after that.

Every clue is read as one of two families.

**Stable clues** are self-contained, read from the clue's own shape:

- A **colour** clue is a *referential play*: it points at the card one slot to
  the **left** (newer) of a newly-touched card.
- A **rank** clue is a *referential discard*: it points at the first unclued
  slot to the **right** (older) of the focus. If it instead touches the
  receiver's **lock slot** (their oldest unclued card), it's a **lock** —
  nothing in that hand is safe to discard.

**Reactive clues** are the convention's distinctive mechanism. A reactive clue
is given to a **receiver**, but it is decoded by a third player — the
**reacter**, normally the giver's next player. Neither the slot to act on nor
who acts is stated directly. Instead:

> **react_slot + target_slot ≡ focus_slot  (mod hand_size)**

The reacter picks a slot of their own; that choice, combined with the clue's
focus slot, tells the receiver which of *their* slots to act on. The clue's
*kind* says what the two actions are:

- **colour clue** → reacter **discards**, receiver **plays** ("odd plays")
- **rank clue** → reacter **plays**, receiver **plays** ("even plays")

So one clue can produce two plays, and the information is carried as much by
*which slot the reacter chooses to act on* as by the clue itself.

The bot decides between the two families per clue
(`src/basics/decide.cpp:31-220`), and can rewind and re-read a clue as
reactive if the next player's response contradicts the stable reading.

Cards are kept alive by the two stable readings rather than by a dedicated save
clue: a referential discard implicitly protects everything it does not name, and
a lock protects a whole hand. The reactive family has its own **finesse** — the
reacter plays a card that connects with a one-away-from-playable card in the
receiver's hand — see [CONVENTION.md §1a.5](CONVENTION.md).

## 3. Repository layout

| Path | Contents |
|---|---|
| `src/basics/` | Game state, empathy/card knowledge, elimination, the clue dispatcher (`decide.cpp`) and action selection |
| `src/conventions/reactor/` | The convention proper: `interpret_clue`, `interpret_reactive`, `interpret_reaction`, `state_eval` |
| `src/conventions/variants/` | Variant-specific convention rules: rainbow/pink tables, brownish, inverted (Orange), reversed |
| `src/endgame/` | Exact win-probability solver, forced-endgame rules, winnability helpers |
| `src/net/` | hanab.live client: HTTPS login, WebSocket transport, protocol codec, command dispatcher, notes |
| `src/logging/` | Per-game JSONL logs, state snapshots, decision traces |
| `src/instrumentation/` | Scoped timers feeding the per-turn `TIMING` records |
| `include/hanabi/` | Public headers, mirroring `src/` |
| `apps/` | Executable entry points |
| `tests/` | GoogleTest suites, including 53 replay regressions |
| `scripts/` | Developer tools for log inspection and test generation |
| `data/` | `variants.json` + `suits.json`, hanab.live's variant definitions, loaded at runtime |

Largest and most load-bearing files: `src/basics/decide.cpp` (1 055 lines),
`src/endgame/solver.cpp` (903), `src/conventions/reactor/interpret_reactive.cpp`
(874), `src/net/commands.cpp` (846),
`src/conventions/reactor/interpret_clue.cpp` (818).

## 4. Dependencies

| Dependency | Version | How it is obtained |
|---|---|---|
| C++20 compiler | GCC 13+ / Clang 15+ | system |
| CMake | ≥ 3.20 | system |
| Ninja | any | system (the build scripts assume this generator) |
| Boost | ≥ 1.90 (headers only — Asio + Beast) | system |
| OpenSSL | 3.x | system |
| nlohmann/json | v3.11.3 | `FetchContent` at configure time |
| spdlog | v1.15.3 | `FetchContent` |
| GoogleTest | v1.14.0 | `FetchContent` |
| cpp-httplib | v0.18.7 | `FetchContent` |

The four `FetchContent` dependencies (`cmake/deps.cmake`) are cloned from
GitHub during `cmake` configure, so **the first configure needs network
access**. Later builds do not.

C++20 is a hard requirement, not a preference: `IdentitySet` uses
`std::popcount` and `std::countr_zero` from `<bit>`
(`include/hanabi/basics/identity_set.h`).

---

## 5. Building on Windows (MSYS2 / UCRT64)

This is the path the project is currently developed on, verified end to end on
Windows 11 with GCC 16.1.0, CMake 4.4.0, Boost 1.91, and OpenSSL 3.6.3.

MSYS2 is used rather than MSVC or vcpkg because `pacman` ships **prebuilt**
Boost and OpenSSL packages (vcpkg would build OpenSSL from source), and because
GCC accepts the project's existing `-Wall -Wextra -Wpedantic` flags unchanged.

### 5.1 Install MSYS2

```powershell
winget install --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
```

### 5.2 Update MSYS2 — **this takes two passes**

Open `C:\msys64\ucrt64.exe` and run:

```bash
pacman -Syuu --noconfirm
```

The first pass upgrades `msys2-runtime` and then **closes every MSYS2
process, including the terminal you are in**. That is expected. Reopen the
UCRT64 shell and run the same command again to finish the upgrade.

### 5.3 Install the toolchain and libraries

```bash
pacman -S --needed --noconfirm \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-boost \
  mingw-w64-ucrt-x86_64-openssl \
  git
```

All are prebuilt binary packages; nothing compiles from source here.

> **Use the UCRT64 environment, not the MSYS or MINGW64 one.** The
> `mingw-w64-ucrt-x86_64-*` packages are only on the `PATH` under UCRT64.
> Launch `C:\msys64\ucrt64.exe`, or if you are driving the shell from
> PowerShell, set `MSYSTEM` first:
>
> ```powershell
> $env:MSYSTEM='UCRT64'; $env:CHERE_INVOKING='1'
> & C:\msys64\usr\bin\bash.exe -lc "<command>"
> ```

### 5.4 Configure, build, test

```bash
cd /c/path/to/cpp-bot
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure -LE decision_making
```

Expect roughly two minutes to configure (dominated by the four dependency
clones) and a few minutes to build.

`RelWithDebInfo` is the default build type if you do not pass one
(`CMakeLists.txt:9-11`). Note that debug info makes the artifacts large —
`libhanabi_core.a` is around 140 MB and `replay_log.exe` around 70 MB. That is
normal, not a symptom of a problem.

### 5.5 Running the binaries

The executables link against the UCRT64 runtime DLLs, so
`C:\msys64\ucrt64\bin` must be on `PATH`. Inside the UCRT64 shell this is
automatic. From PowerShell or Git Bash it is not — you will get a bare
"command not found"-style failure with no further explanation:

```bash
PATH="/c/msys64/ucrt64/bin:$PATH" ./build/hanabi_bot.exe index=0
```

### 5.6 Building from Git Bash instead of the MSYS2 shell

You do not need the MSYS2 shell. The UCRT64 environment is essentially just a
`PATH`, and the toolchain binaries in `C:\msys64\ucrt64\bin` are **native
Windows PE executables linked against the UCRT** — none of them depends on
`msys-2.0.dll` — so they run fine from Git Bash, PowerShell, or cmd.

One directory is all you need, prepended so it wins over anything else:

```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure -LE decision_making
```

Verified from Git Bash (`MSYSTEM=MINGW64`): configure, full build, `ctest`, and
the `scripts/*.sh` helpers all work, and the same `PATH` entry covers the
runtime DLLs from §5.5.

To make it permanent for Git Bash, append the `export` line to `~/.bashrc`. To
make it permanent for every shell including PowerShell, add
`C:\msys64\ucrt64\bin` to your user `PATH` via *System Properties → Environment
Variables* (or `setx PATH "C:\msys64\ucrt64\bin;%PATH%"`, once, from a normal
prompt). That directory holds only ~126 binaries and ships no `bash`/`sh`, so
it is unlikely to disturb anything; the only common names it shadows are
`cmake`, `curl`, and `openssl`.

> **Do not add `C:\msys64\usr\bin`.** That is the MSYS (Cygwin-like)
> environment, and it ships its own `msys-2.0.dll` — as does Git Bash. Having
> both on one `PATH` is the classic way to get MSYS processes crashing on a
> DLL-version mismatch. Git Bash already provides the coreutils the build
> scripts need, so `ucrt64\bin` alone is sufficient.

Two things that are *not* problems here: the Ninja generator sidesteps CMake's
"`sh.exe` was found in your PATH" complaint, which only affects the *MinGW
Makefiles* generator; and `cmake` resolves the relative `-B build` path fine
from a Git Bash working directory.

### 5.7 Troubleshooting

Every item here is an error actually encountered bringing this build up on
Windows.

| Symptom | Cause and fix |
|---|---|
| `'::setenv' has not been declared` in `src/settings.cpp` | mingw-w64/UCRT has no POSIX `setenv`. Fixed in-tree by `hanabi::platform::setenv_if_unset` (`include/hanabi/platform/compat.h`). |
| `'::localtime_r' has not been declared` in `src/logging/game_logger.cpp` or `apps/hanabi_bot.cpp` | Same — no POSIX `localtime_r`. Fixed by `hanabi::platform::localtime_local`, which wraps `localtime_s` on Windows (note the arguments are in the opposite order). |
| Dozens of `undefined reference to '__imp_select'`, `'__imp_WSARecv'`, `'__imp_WSASend'`, `'__imp_connect'`, `'__imp_shutdown'`, `getaddrinfo`, … when linking `hanabi_bot.exe` / `hanabi_tests.exe` | On Linux and macOS the socket API is in libc, so `CMakeLists.txt` never needed platform link libraries. On Windows it lives in Winsock. Fixed by linking `ws2_32 mswsock crypt32` under `if(WIN32)` (`CMakeLists.txt`). |
| Binary starts and immediately fails with no message | UCRT64 runtime DLLs not on `PATH` — see §5.5. |
| `scripts/find_game.sh` reports "no per-game log found" even though the log exists | Was calling BSD `stat -f "%m %N"`; MSYS2 ships GNU coreutils, which needs `-c "%Y %n"`. The script now probes for the right flavour at runtime. |
| `pacman -Syuu` appears to hang or kills your terminal | Expected on the first pass — see §5.2. Reopen the shell and repeat. |
| `collect2.exe: error: ld returned 1 exit status` when linking a test binary, with no preceding compile error | Windows locks a running executable, so a rebuild cannot relink `hanabi_tests.exe` while a `ctest` run is in progress. Wait for the test run to finish, then rebuild. |

| MSYS or Git Bash processes crash with a DLL-version error after a `PATH` change | `C:\msys64\usr\bin` was added to `PATH` alongside Git Bash, giving two rival `msys-2.0.dll`s. Remove it — see §5.6, you only need `ucrt64\bin`. |

The bash scripts (`build.sh`, `scripts/*.sh`) require a POSIX shell; run them
from the MSYS2 shell or Git Bash, not from PowerShell.

---

## 6. Building on macOS

```bash
brew install cmake ninja boost openssl@3
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure -LE decision_making
```

`./build.sh` is a convenience wrapper that rebuilds just the `hanabi_bot`
target and recovers automatically from a truncated `.ninja_deps` left by an
interrupted build.

If CMake cannot find OpenSSL — Homebrew keeps it keg-only — point it at the
right prefix:

```bash
cmake -G Ninja -B build -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
```

The Windows portability shims are all behind `#if defined(_WIN32)`
(`include/hanabi/platform/compat.h`) and the Winsock link libraries behind
`if(WIN32)`, so the macOS build is unaffected by them.

---

## 7. Executables

All land in `build/`.

| Target | What it does |
|---|---|
| `hanabi_bot` | The live bot. Logs into hanab.live, opens a WebSocket, joins/creates tables, plays. |
| `hanabi_tests` | Correctness suite — 338 tests, including 52 replay regressions reconstructed from real games. A failure here is a bug. |
| `hanabi_decision_tests` | Decision-**quality** suite, under the ctest label `decision_making`. A failure means the bot made a suboptimal but not necessarily incorrect choice, and needs manual review (see `CLAUDE.md`). |
| `replay_log` | Reconstructs a `Game` from a per-game JSONL log and re-runs `take_action` with the current build, or emits a regression-test scaffold. |
| `bench_endgame` | Microbenchmark of `IdentitySet` and `Fraction` primitives. |

Run the two test suites separately:

```bash
ctest --test-dir build --output-on-failure -LE decision_making   # correctness
ctest --test-dir build --output-on-failure -L  decision_making   # quality
```

## 8. Running the bot

### Credentials

The bot reads credentials from the environment, falling back to a `.env` file
in the working directory (`src/settings.cpp`). Keys are suffixed with the bot
index, so several bots can share one file:

```
HANABI_USERNAME0=...
HANABI_PASSWORD0=...
HANABI_USERNAME1=...
HANABI_PASSWORD1=...
HANABI_LEAVE_PREGAME_IF_ONLY_BOTS=true
HANABI_LEAVE_REPLAY_IF_ONLY_BOTS=true
```

`HANABI_HOST` overrides the server. Environment variables take precedence over
`.env`; `.env` never overwrites something already set.

> ⚠ **The `.env` file in this repository is committed to git and contains
> plaintext passwords.** If those accounts matter, rotate the credentials and
> untrack the file.

### Invocation

Arguments are `key=value` pairs (matching the earlier Scala and Python bots):

```bash
./build/hanabi_bot index=0
./build/hanabi_bot index=1 bot_to_join=create table=bots max_players=5
./build/hanabi_bot index=0 host=localhost:8080
```

| Key | Default | Meaning |
|---|---|---|
| `index` | `0` | Which `HANABI_USERNAME<n>` / `HANABI_PASSWORD<n>` pair to use |
| `username`, `password` | — | Used only if the env vars are absent |
| `host` | `hanab.live` | Server. `localhost` / `127.*` / `0.0.0.0` automatically switch to plain HTTP/WS |
| `bot_to_join` | — | Absent = idle; `create` = make a table; otherwise the username to join |
| `table` | `bots` | Table name when creating |
| `max_players` | `5` | Start the game once this many players are seated |
| `convention` | `Reactor1` | Convention name reported to the server |
| `disconnect_on_game_end` | `false` | Exit after one game |

Ctrl+C shuts the socket down cleanly. Connection failures retry with
exponential backoff, up to 5 attempts.

### Chat commands

The bot answers these on hanab.live (`src/net/commands.cpp:140-195`). Some are
available anywhere; the rest only in a private message.

| Command | Where | Effect |
|---|---|---|
| `/settings` | anywhere | Print the variant's reactive slot tables |
| `/allplays [on\|off]` | anywhere | Promote colour reactives to play+play. With no argument, reports the current setting |
| `/getversion` | anywhere | Report the running `kBotVersion` |
| `/leaveall` | anywhere | Leave every table |
| `/join [user]` | PM only | Join a table |
| `/create` | PM only | Create a table |
| `/start` | PM only | Start the game |
| `/setvariant <name>` | PM only | Set the variant. `_` becomes a space, `+` becomes ` & ` |
| `/terminate [table_id]` | PM only | Terminate a game |

At the start of every game the bot writes its version as a note on card
order 0, so observers can confirm which build is running.

## 9. Debugging a game

Each game writes a structured JSONL log to
`logs/{bot_name}-{game_id}.log`. When the game ends the server's `databaseID`
message renames it to the hanab.live database id, so a
`https://hanab.live/shared-replay/<id>` URL maps directly onto a log file.

The workflow — **do not re-simulate from turn 1**; the log already has the
state and the branch the bot took:

```bash
scripts/find_game.sh <game_id>                       # locate the log
scripts/show_turn.py logs/<bot>-<game_id>.log <turn> # state + decision trace + timing
build/replay_log logs/<bot>-<game_id>.log --turn N --rerun   # re-run with the current build
scripts/bug_to_test.sh logs/<bot>-<game_id>.log <turn> [category] [slug]  # emit a regression test
```

`scripts/log_summary.py` prints per-turn actions plus the per-game timing
aggregate — useful for "where did the bot spend its time" questions.

Full details, including the replay-test naming and category conventions, are in
[CLAUDE.md](CLAUDE.md).

## 10. Contributing

- The build version lives in `include/hanabi/version.h` as `kBotVersion` and
  **must be bumped on every deployed change**.
- **Modifying or deleting any existing test requires prior approval**; adding
  tests does not. New test files must be added to the source list in
  `CMakeLists.txt` by hand — there is no globbing.
- On every version bump, check that `CONVENTION.md`, `GLOSSARY.md`, `TODO.md`,
  and this file still match the code.

`CLAUDE.md` has the full working agreement.
