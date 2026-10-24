/*
Loup-Garou SimWorld - M8: complete console menu and safe input.
Team members: names have not yet been supplied.
World: a village simulation with alternating Night and Day turns.

Hierarchy:
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf
Seer also implements Inspector; Witch also implements PotionUser.

Feature mapping confirmed by the supplied gaps/design documents:
F4: phase-dependent actions and event effects.
F6: owned FIFO event queue and ordered healing.
F7: identity-preserving snapshots and multi-level undo with exact list restoration.
F10: deep copies, virtual destruction, and ownership of players, events, and command snapshots.
F11: win detection and statistics are planned for M9.
The complete original numbered specification is needed to verify other labels.

Build from the repository root (create build/ first):
g++ -std=c++17 -Wall Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp Command.cpp Capabilities.cpp Console.cpp main.cpp -o build/simworld
Tests: see README.md and tests/m1_tests.cpp through tests/m8_tests.cpp.

AI usage log:
2026-10-23: Codex helped consolidate the supplied design, gaps, and course limits.
2026-10-24: User approved M1. Codex implemented the hierarchy, deep copying,
           demonstration, documentation, and ownership tests for user review.
2026-10-24: User approved M2. Codex added custom array-based Stack and Queue
           templates, standalone tests/demo, and the M2 walkthrough.
2026-10-24: User approved M3. Codex added World ownership, unique IDs, phase
           tracking, virtual turn dispatch, role messages, and M3 tests/docs.
2026-10-24: User approved M4. Codex added events, role effects, menu, tests, and docs.
2026-10-24: User approved M5. Codex added player management and validation tests.
2026-10-24: User approved M6. Codex added undo commands, snapshot ownership, tests, and docs.
2026-10-24: User approved M7. Codex added capability interfaces, checked casts, menu actions, and tests.
2026-10-24: User approved M8. Codex added the Console class, validated prompts, details view, and menu tests.
*/

#include "Console.h"
#include <iostream>
#include <exception>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--demo")) {
        std::cerr << "Usage: simworld [--demo]\n";
        return 1;
    }
    try {
        World world;
        world.createPlayer(WITCH_ROLE, "Clara");
        world.createPlayer(SEER_ROLE, "Basil");
        world.createPlayer(VILLAGER_ROLE, "Alice");
        world.createPlayer(HUNTER_ROLE, "Dalia");
        world.createPlayer(WEREWOLF_ROLE, "Elias");
        world.createPlayer(ALPHA_WOLF_ROLE, "Farah");
        world.createPlayer(VILLAGER_ROLE, "Gamal");
        world.createPlayer(VILLAGER_ROLE, "Hana");
        world.clearUndoHistory(); // The sample village is setup, not an administrator edit.
        if (argc == 2) {
            world.describePlayers();
            world.requestInspection(3, 5); // Unsupported capabilities fail safely.
            world.requestPotion(3, 5, true);
            world.requestInspection(2, 5);
            world.requestPotion(1, 1, true);
            world.requestPotion(1, 5, false);
            world.viewPendingEvents();
            world.runTurns(2);
            world.viewPendingEvents();
            int duplicateId = world.duplicatePlayer(2);
            std::cout << "Duplicated Seer 2 as " << duplicateId << ".\n";
            world.modifyPlayer(duplicateId, "Basil copy", true, false);
            world.findPlayer(2)->describe(std::cout);
            world.findPlayer(duplicateId)->describe(std::cout);
            world.removePlayer(duplicateId);
            std::cout << "Undo delete, modify, and duplicate, newest first:\n";
            world.undo();
            world.findPlayer(duplicateId)->describe(std::cout);
            world.undo();
            world.findPlayer(duplicateId)->describe(std::cout);
            world.undo();
            world.undo(); // Safe empty-history demonstration.
            world.describePlayers();
            std::istringstream demoInput("13\n2\n0\n");
            Console console(world, demoInput, std::cout);
            console.run();
        } else {
            Console console(world, std::cin, std::cout);
            console.run();
        }
    } catch (const std::exception& error) {
        std::cerr << "Unable to continue: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
