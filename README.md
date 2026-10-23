# Loup-Garou SimWorld

A turn-based C++ console simulation of a werewolf village, planned for CSCE 1101.

## Status

Planning complete. Implementation has not started and awaits explicit confirmation.
Repository setup does not authorize starting the implementation milestones.

## Requirements and scope

The plan combines the supplied Design, Milestones, and Gaps documents, with the
Fall 2026 CSCE 1101 handout setting the implementation's complexity level.

- Compile as C++17 with `g++ -std=c++17 -Wall`, without warnings.
- Separate declarations and definitions into `.h` and `.cpp` files.
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

## Decisions to settle before the affected milestone

Exact role target-selection rules, remaining phase-specific role actions, voting
ties, potion limits, round counting, and the boundary between player-edit undo
and turn execution still need concrete rules. The original F1-F11 specification
has not been supplied, so its full mapping must not be invented.

## Validation plan

Check independent copies for every role; container ordering and growth; protected
targets; Hunter death reactions; AlphaWolf voting weight; exact delete/modify undo;
events referencing deleted players; repeated undo; invalid input; and both victory
conditions with matching statistics. Run memory checks over these interactions.

There are no source files or build targets yet.
