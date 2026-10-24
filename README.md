# Loup-Garou SimWorld

A C++17 console simulation by **Ali Elyazori (900244034)** and
**Youcef Rabehi (900244346)**, following the supplied design, milestones, gaps,
and CSCE 1101 course topics.

## Build and run

From the repository root on this Windows machine:

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_m10.ps1 -Interactive
~~~

This builds without warnings, runs all nine suites and two application
playthroughs, creates a source-only archive, and opens the console. Omit
-Interactive for unattended verification. Add -MemoryChecks for Visual Studio
AddressSanitizer and debug-heap checks.

The compiler is g++ from PATH or the installed Qt MinGW compiler. The
execution-policy option applies only to that PowerShell process. Generated
files stay in ignored build/; the application is build/simworld.exe.

For standard g++:

~~~sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp Capabilities.cpp Console.cpp main.cpp -o build/simworld
./build/simworld
./build/simworld --demo
~~~

Only root production .cpp files form the application. Compile each test
separately: tests and historical examples have their own main functions.

## Console

The application starts with eight sample players at Night. One turn is one
phase; one complete round is Night followed by Day.

| Choice | Action |
| --- | --- |
| 0 | Quit |
| 1 | List all players |
| 2 | View pending events without consuming them |
| 3 | Queue a manual event |
| 4 / 5 | Run one phase / a batch of 1-1,000 phases |
| 6 | Create any role with defaults or custom values |
| 7 / 8 / 9 | Delete / duplicate / modify a player |
| 10 | Undo the most recent player-management edit |
| 11 / 12 | Ask a player to inspect / use a potion |
| 13 / 14 | Show one player's details / simulation statistics |

Integer and name prompts re-prompt on invalid values. EOF exits without applying
an incomplete command. Details are an administrator view that reveals roles;
Seer inspection is a separate gameplay action that queues an event.

## Rules

- Six roles: Villager, Seer, Witch, Hunter, Werewolf, AlphaWolf. Each has distinct
  phase behavior and its own independently copied dynamic array.
- Automatic Night actions run Witch/Seer first, wolves next, then Villager/Hunter.
  Events resolve FIFO. Manual events precede automatic ones and are never sorted.
- Wolves share one eligible village victim. Targeting uses learned facts/suspicion
  where available, then the lowest eligible ID.
- Witch has one heal and one poison. She auto-heals herself once; poison is an
  explicit user action. Healing protects a living target and never resurrects.
  Protection blocks attacks, poison, and Hunter shots, then expires after Night.
- Day uses the first valid ballot per source. AlphaWolf weight is two; others
  one. Highest-total ties and no valid votes cause no elimination.
- Hunter death appends a final shot. Every reaction resolves before victory:
  no wolves means village victory, wolf parity or greater means wolf victory,
  and no survivors means a draw. Empty worlds cannot start.
- Results stop turns. Successful management edits or undo reopen the same
  simulation and retain statistics. Administrative edits have no death reactions
  and add no kill records.
- Undo restores player state, ID, and position. Starting a turn clears history.
  Turns/events/statistics are not undoable; allocated IDs are never recycled.
  There is no save/load or randomness.

## Design and requirements

~~~text
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf
Seer also implements Inspector; Witch also implements PotionUser.
~~~

Player owns shared identity/flags. Teams share side identity and wolf priority.
Roles own their histories and behavior. World owns one vector of Player pointers,
the FIFO events, and undo commands. Normal turns use virtual methods; only the
two user-selected optional capabilities use checked dynamic_cast.

| Feature | Implementation |
| --- | --- |
| F1: diverse entities | Player / Teams / six Roles |
| F2: creation and constructors | World::createPlayer; role rule-of-three methods |
| F3: independent duplication | Role clone methods; World::duplicatePlayer |
| F4: polymorphic turns | World::runTurn/runTurns; virtual act/nightOrder/voteWeight |
| F5: special capabilities | Inspector / PotionUser; World capability request methods |
| F6: FIFO events | Queue, Event hierarchy, World event methods |
| F7: multi-level undo | Command hierarchy, Stack, World::undo |
| F8: own data structures | Dynamic-array Stack and circular-array Queue |
| F9: complete console | Console::run and line-based input helpers |
| F10: memory safety | Virtual destruction, deep copies, IDs, explicit ownership |
| F11: own feature | Victory/draw checks, cumulative votes, named elimination history |

The complete design report, assumptions, feature map, team details, compilation
instructions, and accurate AI disclosure are also in main.cpp's header comment.
Class comments explain responsibility and hierarchy placement. The F1-F11 labels
were verified against the local SimWorld assignment handout.

The runtime uses standard C++17: classes, inheritance, virtual functions, pointers,
new[]/delete[], basic templates, loops, exceptions, string, and vector. It uses no
forbidden stack/queue/list containers, smart pointers, lambdas, move semantics,
concurrency, or external runtime libraries. Template definitions stay in headers.

## Verification and source archive

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_m10.ps1 -MemoryChecks
powershell -NoProfile -ExecutionPolicy Bypass -File .\verify_memory.ps1
~~~

The second command runs memory checks separately. It finds Visual Studio tools,
then builds every suite with AddressSanitizer and separately with the debug heap.

Linux verification requires g++ and Valgrind:

~~~sh
sh verify_linux.sh
~~~

It runs each suite with AddressSanitizer/UndefinedBehaviorSanitizer and leak
detection, then Valgrind on separate uninstrumented binaries.
See [the final verification report](docs/M10.md) for actual results and contracts.

The final build creates build/Team_900244034_900244346.zip, containing the 19 root
production .h/.cpp files. It excludes test/example entry points, executables,
dependencies, and build folders. It does not export this conversation; keep a
conversation export separately if needed for the assignment hand-in.

## Milestone walkthroughs

M1-M9 describe historical checkpoints; M10 documents the final state. The earlier
build_mN.ps1 scripts remain available and use current behavior.

| Milestone | Walkthrough |
| --- | --- |
| M1: hierarchy and deep copies | [M1](docs/M1.md) |
| M2: custom containers | [M2](docs/M2.md) |
| M3: World and phases | [M3](docs/M3.md) |
| M4: events | [M4](docs/M4.md) |
| M5: player management | [M5](docs/M5.md) |
| M6: undo | [M6](docs/M6.md) |
| M7: capabilities | [M7](docs/M7.md) |
| M8: console and input | [M8](docs/M8.md) |
| M9: outcomes and statistics | [M9](docs/M9.md) |
| M10: final verification | [M10](docs/M10.md) |
