# Loup-Garou SimWorld

A turn-based C++ console simulation of a werewolf village, planned for CSCE 1101.

## Status

M1 through M3 are implemented: the player hierarchy, custom Stack and Queue, and
a World that owns players and runs Night/Day turns through virtual actions.
The user approved starting M3 on 2026-10-24. M3 is ready for review; M4 has not started.

Each role now prints a distinct action for each phase. Events, actual attacks,
inspection results, healing, and votes arrive in M4; Hunter's death hook remains
empty until then. This is not yet a complete playable game.

## Build and try M3

On this Windows machine, run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_m3.ps1
```

This builds and runs the World tests and the current `main.cpp` demonstration.
It uses g++ from PATH or the installed Qt MinGW compiler. The demonstration shows
night priorities, day order, one/multiple turns, dead-player skipping, removal,
and unique IDs. No interactive input is required yet.

For a standard g++ environment, first create `build/`, then run:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror Player.cpp Teams.cpp Roles.cpp World.cpp main.cpp -o build/simworld
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp tests/m3_tests.cpp -o build/m3_tests
./build/m3_tests
./build/simworld
```

Read [the M3 walkthrough](docs/M3.md) for ownership, ID generation, turn order,
phase bookkeeping, and the distinction between dead and removed players.

## Build and try M2

On this Windows machine, run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_m2.ps1
```

This builds and runs the standalone container tests and demonstration. The script
uses g++ from PATH or the installed Qt MinGW compiler. It does not change the
machine's execution policy.

For a standard g++ environment, first create `build/`, then run:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. tests/m2_tests.cpp -o build/m2_tests
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. examples/m2_demo.cpp -o build/m2_demo
./build/m2_tests
./build/m2_demo
```

Read [the M2 walkthrough](docs/M2.md) for the container APIs, array growth,
queue viewing, pointer ownership, and memory checks.

## Build and try M1

On this Windows machine, run from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_m1.ps1
```

The script uses g++ from PATH, or the installed Qt MinGW compiler. It builds and
runs both the tests and the demonstration. The execution-policy option applies
only to this process; it does not change the machine's settings.

For a standard g++ environment, first create `build/`, then run:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp examples/m1_demo.cpp -o build/m1_demo
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I. Player.cpp Teams.cpp Roles.cpp World.cpp tests/m1_tests.cpp -o build/m1_tests
./build/m1_tests
./build/m1_demo
```

Read [the M1 walkthrough](docs/M1.md) for the file responsibilities, copy examples,
and memory-check instructions. Generated executables stay under ignored `build/`.

## Requirements and scope

The plan combines the supplied Design, Milestones, and Gaps documents, with the
Fall 2026 CSCE 1101 handout setting the implementation's complexity level.

- Compile as C++17 with `g++ -std=c++17 -Wall`, without warnings.
- Separate ordinary class declarations and definitions into `.h` and `.cpp` files.
  Template definitions stay in `Stack.h` and `Queue.h` so each caller can compile
  the element type it uses, as anticipated by the design's suggested file list.
- Use basic classes, inheritance, virtual functions, pointers, dynamic arrays,
  simple templates, loops, and basic `std::string` and `std::vector` operations.
- Implement the abstract Player -> team -> concrete role hierarchy with Villager,
  Seer, Witch, Hunter, Werewolf, and AlphaWolf.
- Each concrete role owns a variable-size dynamic array and implements a deep-copy
  constructor, assignment operator, and destructor. `clone()` preserves the ID.
- Implement custom dynamic-array Stack and Queue templates. Do not use
  `std::stack`, `std::queue`, `std::deque`, `std::priority_queue`, or `std::list`.
- Normal turns use virtual dispatch without type checks. Checked `dynamic_cast`
  is reserved for user-selected Inspector and PotionUser capabilities.
- Avoid advanced template techniques, lambdas, smart pointers, move semantics,
  concurrency, external application libraries, and unnecessary frameworks.
- Validate all input and verify ownership with AddressSanitizer and, where
  available, Valgrind. No global variables except constants.

## Milestones

Work proceeds one milestone at a time, with an explanation and a pause for user
testing before advancing.

1. **Player hierarchy:** all six roles, constructors, deep copies, virtual
   destructors, `clone()`, `describe()`, and `act()`.
2. **Stack and Queue:** simple templates, safe empty operations, growth, and
   non-destructive queue viewing.
3. **World and turns:** one `vector<Player*>`, unique IDs, Night/Day phases, and
   execution of one or multiple turns.
4. **Events:** Kill, Heal, Vote, Reveal, and Hunter Shoot events; ordered resolution,
   manual event creation, and pending-event viewing.
5. **Player operations:** create with defaults or values, delete, duplicate with a
   new ID, and validated modification.
6. **Undo:** commands for create, delete, duplicate, and modify, stored on the
   custom stack; exact restoration of saved player state and position.
7. **Special capabilities:** Inspector and PotionUser interfaces with checked casts.
8. **Menu and input:** expose all operations and handle invalid, overflowing,
   empty, and end-of-input cases safely.
9. **Win conditions and statistics:** check after each turn; report rounds, votes,
   and kill history. Villagers win with no wolves; wolves win at numerical parity
   or greater against the remaining village team.
10. **Final verification and documentation:** warning-free build, memory checks,
    integration scenarios, class comments, and a project header containing team
    names, world description, hierarchy diagram, F1-F11 feature map, compilation
    instructions, and an accurate AI usage log.

## Gap fixes included

- Give every role distinct phase-dependent behavior. Hunter's virtual `onDeath()`
  queues a ShootEvent; AlphaWolf kills at night and has a double-weight day vote.
- Use virtual `nightOrder()` to run Witch and Seer at order 0 before wolves at
  order 1, with no role type checks in the turn loop.
- A HealEvent sets protection before queued KillEvents resolve. Kills check that
  flag; protection clears at the end of the night. Keep event creation order.
- Only duplication assigns a new ID after cloning. Delete and modify commands
  retain snapshots and vector positions for exact undo.
- Events reference player IDs rather than player pointers and safely ignore
  missing players. Delete processed events. Commands own their snapshots, and
  undo/history cleanup releases commands and their saved data.

## Approved gameplay rules for later milestones

- Start at Night. One turn is one phase; one round is Night followed by Day.
- Dead players remain inspectable; administrative deletion removes them and does
  not trigger a Hunter death reaction.
- Use simple deterministic targeting: role knowledge/suspicion where available,
  otherwise the lowest eligible ID. No self-voting. Wolves share one night victim.
- Each Witch has one healing potion and one poison potion. Healing protects a
  living target, with no resurrection; poison follows the protection rule.
- Resolve queued events and all death reactions before checking victory. Trigger
  a Hunter's retaliation only once per elimination.
- Tied highest vote totals or no valid votes produce no elimination. Count
  ballots separately from their weighted voting total.
- Undo covers player-management edits between turns; starting a turn clears that
  editing history. Turn execution itself is not undoable.
- An empty world cannot start; if no one survives an event chain, declare a draw.
- The menu is an administrator view that may reveal roles. Automatic decisions
  use the knowledge available to the acting role.

The original F1-F11 specification has not been supplied, so the full numbered
feature mapping must not be invented. Team-member names also remain to be supplied.

## Validation plan

Check independent copies for every role; container ordering and growth; protected
targets; Hunter death reactions; AlphaWolf voting weight; exact delete/modify undo;
events referencing deleted players; repeated undo; invalid input; and both victory
conditions with matching statistics. Run memory checks over these interactions.

M1 tests cover all six roles with empty and populated copies, assignment over
existing arrays, self-assignment, source destruction, clone independence, growth,
invalid indices/values, and deletion through `Player*`.

M2 tests cover LIFO/FIFO order, safe empty operations, repeated growth and queue
wraparound, non-destructive viewing, independent copies, strings and pointer
values, and cleanup when element copying throws an exception.

M3 tests cover all six role messages in both phases, night priorities, stable
player order, unique IDs, safe removal, dead-player skipping, zero/negative turn
requests, empty/all-dead worlds, multi-turn counting, counter overflow, output
failure recovery, vector growth, and ownership cleanup.
