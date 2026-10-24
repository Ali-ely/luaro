/*
Loup-Garou SimWorld - M4: queued events and role effects.
Team members: names have not yet been supplied.
World: a village simulation with alternating Night and Day turns.

Hierarchy:
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf

Feature mapping confirmed by the supplied gaps/design documents:
F4: phase-dependent actions and event effects.
F6: owned FIFO event queue and ordered healing.
F7: identity-preserving clone in M1; commands/undo follow in M5/M6.
F10: deep copying, virtual destruction, and World player ownership; event ownership in M4.
F11: win detection and statistics are planned for M9.
The complete original numbered specification is needed to verify other labels.

Build from the repository root (create build/ first):
g++ -std=c++17 -Wall Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp main.cpp -o build/simworld
Tests: see README.md and tests/m1_tests.cpp through tests/m4_tests.cpp.

AI usage log:
2026-10-23: Codex helped consolidate the supplied design, gaps, and course limits.
2026-10-24: User approved M1. Codex implemented the hierarchy, deep copying,
           demonstration, documentation, and ownership tests for user review.
2026-10-24: User approved M2. Codex added custom array-based Stack and Queue
           templates, standalone tests/demo, and the M2 walkthrough.
2026-10-24: User approved M3. Codex added World ownership, unique IDs, phase
           tracking, virtual turn dispatch, role messages, and M3 tests/docs.
2026-10-24: User approved M4. Codex added events, role effects, menu, tests, and docs.
*/




#include "World.h"
#include <iostream>
#include <exception>
#include <sstream>
#include <string>

// Read a complete line, rejecting overflow and trailing garbage.
bool readInteger(const std::string& prompt, int& value) {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) { return false; }
        std::istringstream input(line);
        char extra;
        if ((input >> value) && !(input >> extra)) { return true; }
        std::cout << "Enter one whole number within the int range.\n";
    }
}

void runEventMenu(World& world) {
    int choice;
    while (true) {
        std::cout << "\nM4 events | Next phase: "
                  << (world.getPhase() == NIGHT ? "Night" : "Day")
                  << "\n1 List players\n2 View pending events\n3 Add event\n"
                  << "4 Run one turn\n5 Run several turns\n0 Quit\n";
        if (!readInteger("Choice: ", choice) || choice == 0) { return; }
        if (choice == 1) { world.describePlayers(); }
        else if (choice == 2) { world.viewPendingEvents(); }
        else if (choice == 3) {
            int kind, source, target;
            std::cout << "1 Kill, 2 Heal, 3 Vote, 4 Reveal, 5 Poison\n";
            if (!readInteger("Event: ", kind)) { return; }
            if (kind < 1 || kind > 5) {
                std::cout << "Choose an event from 1 to 5.\n";
                continue;
            }
            if (!readInteger("Source ID: ", source) ||
                !readInteger("Target ID: ", target)) { return; }
            if (world.addManualEvent(static_cast<EventKind>(kind - 1), source, target)) {
                std::cout << "Event queued. Run a turn to apply it.\n";
            }
        }
        else if (choice == 4) { world.runTurn(); }
        else if (choice == 5) {
            int count;
            if (!readInteger("Number of turns: ", count)) { return; }
            if (count <= 0) { std::cout << "Enter a positive turn count.\n"; }
            else {
                try { world.runTurns(count); }
                catch (const std::exception& error) {
                    std::cout << "Could not finish turns: " << error.what() << '\n';
                }
            }
        }
        else { std::cout << "Choose a menu option from 0 to 5.\n"; }
    }
}


int main(int argc, char* argv[]) {
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--demo")) {
        std::cerr << "Usage: simworld [--demo]\n";
        return 1;
    }
    World world;
    world.createPlayer(WITCH_ROLE, "Clara");
    world.createPlayer(SEER_ROLE, "Basil");
    world.createPlayer(VILLAGER_ROLE, "Alice");
    world.createPlayer(HUNTER_ROLE, "Dalia");
    world.createPlayer(WEREWOLF_ROLE, "Elias");
    world.createPlayer(ALPHA_WOLF_ROLE, "Farah");
    world.createPlayer(VILLAGER_ROLE, "Gamal");
    world.createPlayer(VILLAGER_ROLE, "Hana");
    if (argc == 2) {
        world.describePlayers();
        world.addManualEvent(REVEAL_EVENT, 2, 5);
        world.viewPendingEvents();
        world.runTurns(2);
        world.viewPendingEvents();
        world.describePlayers();
    } else { runEventMenu(world); }
    return 0;
}
