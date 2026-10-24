Loup-Garou SimWorld - CSCE 1101
Team: Ali Elyazori (900244034), Youcef Rabehi (900244346).

WORLD AND DESIGN
A deterministic werewolf-village simulation. One turn is a Night or Day phase;
one full round is Night followed by Day. All six roles act through virtual
Player methods. Players keep independent role histories, and World owns them
in one vector<Player*>. Dead players remain inspectable until administratively
deleted. World owns queued events and undo commands as well.

Hierarchy:
Player (abstract: identity, flags, virtual actions/clone/description/destruction)
  VillageTeam (abstract: shared village identity)
    Villager, Seer, Witch, Hunter
  WolfTeam (abstract: shared wolf identity and night priority)
    Werewolf, AlphaWolf
Seer also implements the stateless Inspector interface.
Witch also implements the stateless PotionUser interface.
Common identity and validation belong to Player; team facts belong to the middle
level; concrete roles own their arrays and decisions. Optional capabilities are
separate interfaces so requesting one from an arbitrary Player requires a checked
runtime cast, while normal turns remain purely polymorphic.

VERIFIED FEATURE MAP (SimWorld assignment Section 4)
F1  Diverse entities: Player.h/.cpp, Teams.h/.cpp, Roles.h/.cpp; six roles,
    three inheritance levels, private state and validated setters.
F2  Creation: World::createPlayer overloads and Console::createPlayer.
    World/Stack/Queue use default construction; roles use parameterized and copy
    constructors. Every role owns a variable-size array with rule-of-three copying.
F3  Independent duplication: concrete clone methods and World::duplicatePlayer;
    clone preserves identity for snapshots; only duplication assigns a fresh ID.
F4  Turns: World::runTurn/runTurns and the six virtual act implementations.
    nightOrder and voteWeight avoid role checks in normal turn execution.
F5  Special capabilities: Capabilities.h/.cpp; Seer::inspect, Witch::usePotion,
    World::requestInspection/requestPotion. These contain the only two
    dynamic_cast expressions; both pointer results are checked before use.
F6  FIFO events: Queue.h, Event.h/.cpp, World::enqueueEvent/processEvents/
    viewPendingEvents/addManualEvent. Five event classes have distinct effects.
F7  Multi-level undo: Command.h/.cpp, Stack<Command*>, World::undo. Delete/modify
    own exact role clones plus original positions and restore both identity/data.
F8  Own data structures: Stack.h and circular-array Queue.h; growth, safe empty
    operations, independent copies, and non-destructive queue access.
F9  Console: Console.h/.cpp, Console::run and its line-based input helpers;
    every operation is reachable, including individual details and statistics.
F10 Memory: concrete rule-of-three methods, virtual destructors, World cleanup,
    ID-based events, and command-owned snapshots. No event owns a Player pointer.
    All suites are exercised by the final build and sanitizer/heap checks.
F11 Own feature: World::checkVictory, cumulative ballot/weight counters,
    KillRecord history, and World::describeStatistics. Menu option 14 displays it.

COMPILING AND RUNNING (from the root containing these source files)
g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp Capabilities.cpp Console.cpp main.cpp -o simworld
Run ./simworld on Linux or .\simworld.exe on Windows. --demo needs no input.
In the full repository, build_m10.ps1 runs every suite and creates a source-only
archive; -Interactive opens the console, -MemoryChecks adds Windows memory checks.
verify_linux.sh runs Linux sanitizers and Valgrind.

RULES, OWNERSHIP, AND LIMITS
- Night priorities: Witch/Seer first, then wolves, then Villager/Hunter. FIFO
  resolution never reorders events; manual actions queue before automatic ones.
- Wolves share one lowest eligible village victim per Night. Other decisions use
  known inspections/suspicion, then lowest eligible ID. Witch auto-heals herself
  once; explicit poison is a user action. Heal never resurrects. Protection blocks
  attacks, poison, and Hunter shots, then expires at Night's end.
- Day counts the first valid ballot per source; AlphaWolf's weight is two.
  A tied highest total or no ballots means no elimination.
- Death is marked before the Hunter reaction, which appends its shot FIFO.
  All reactions finish before checking village victory, wolf parity, or no-survivor draw.
- Administrative edits have no death reactions. Undo applies only to management,
  not turns/events/statistics. A starting turn clears that editing history.
  IDs are monotonic and are not recycled even after undo. Undo of modification
  replaces an object; callers retain IDs and look up borrowed pointers again.
- A finished result stops turns and new events. A successful management edit or
  undo reopens the same simulation without erasing its cumulative statistics.
- Empty/all-dead worlds cannot start. Menu batches allow 1-1000 phases. The menu
  is an administrator view and reveals roles. There is no save/load or randomness.
- Event/command ownership transfers to World; array containers own only their
  storage. Gameplay exceptions clean pending events but do not roll back facts
  already applied. Failed phases are not counted; final-report failures do not
  undo a phase whose counters and result were already committed.

AI USAGE DISCLOSURE
Tool: OpenAI Codex, through this conversation. The team supplied the design,
milestones, gap fixes, and course topic limit, then authorized implementation
one milestone at a time. Codex generated complete implementation files, tests,
scripts, and documentation; this was more than inline completion or tiny snippets.
The generated code was used directly and revised by Codex after tests/review.
No team-authored code adaptation is recorded in this conversation; names and
student IDs above were supplied by the team. This log does not claim manual
rewriting or line-by-line team review that has not been recorded.

2026-10-23: Asked to read design/milestones/gaps/handout and propose a project
            description. Used the consolidated plan and the user's confirmations.
2026-10-24 M1: Asked for the hierarchy; used generated role classes, deep-copy
            methods, tests and demo, revised for copy/ownership validation.
2026-10-24 M2: Asked for containers; used generated array Stack/Queue and tests,
            including growth, wraparound and failed-copy cleanup.
2026-10-24 M3: Asked for World/turns; used generated ownership/ID/phase code,
            virtual scheduling and tests, revised for stable order and errors.
2026-10-24 M4: Asked for events; used generated event classes, effects and tests,
            revised for FIFO healing, single-use potions and Hunter chains.
2026-10-24 M5: Asked for management; used generated create/delete/duplicate/edit
            actions and validation, revised for atomic edits and independent copies.
2026-10-24 M6: Asked for undo; used generated command classes and snapshots,
            revised for exact IDs/positions and gameplay history boundaries.
2026-10-24 M7: Asked for capabilities; used generated interfaces, checked casts,
            menu actions and tests while preserving virtual normal turns.
2026-10-24 M8: Asked for the full menu; used generated Console and input tests,
            revised for EOF, prompt ranges and stricter Windows compiler warnings.
2026-10-24 M9: Asked for results/stats; used generated outcome/history code and tests,
            revised older fixtures for terminal games and checked report totals.
2026-10-24 M10: Asked for final verification/cleanup; used the code review,
            complete F1-F11 map, class comments, reproducible verification scripts,
            final documentation, and source-only archive. Tests and actual tool
            results are recorded in docs/M10.md rather than presumed successful.
