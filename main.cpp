/*
Loup-Garou SimWorld - M5: player management.
Team members: names have not yet been supplied.
World: a village simulation with alternating Night and Day turns.

Hierarchy:
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf

Feature mapping confirmed by the supplied gaps/design documents:
F4: phase-dependent actions and event effects.
F6: owned FIFO event queue and ordered healing.
F7: identity-preserving clone and duplication with new IDs; undo follows in M6.
F10: deep copying, virtual destruction, and World player ownership; event ownership in M4.
F11: win detection and statistics are planned for M9.
The complete original numbered specification is needed to verify other labels.

Build from the repository root (create build/ first):
g++ -std=c++17 -Wall Player.cpp Teams.cpp Roles.cpp World.cpp Event.cpp main.cpp -o build/simworld
Tests: see README.md and tests/m1_tests.cpp through tests/m5_tests.cpp.

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
*/




#include "World.h"
#include <iostream>
#include <exception>
#include <sstream>
#include <stdexcept>
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

// Return false on EOF; throw for invalid values before any World mutation.
bool readPlayerValues(std::string& name, bool& alive, bool& protection) {
    std::cout << "Name: ";
    if (!std::getline(std::cin, name)) { return false; }
    int aliveInput, protectedInput;
    if (!readInteger("Alive (0/1): ", aliveInput) ||
        !readInteger("Protected (0/1): ", protectedInput)) { return false; }
    if ((aliveInput != 0 && aliveInput != 1) ||
        (protectedInput != 0 && protectedInput != 1)) {
        throw std::invalid_argument("Flags must be 0 or 1.");
    }
    alive = aliveInput == 1;
    protection = protectedInput == 1;
    return true;
}

bool managePlayer(World& world, int choice) {
    if (choice == 6) {
        int role, mode;
        std::cout << "1 Villager, 2 Seer, 3 Witch, 4 Hunter, 5 Werewolf, 6 AlphaWolf\n";
        if (!readInteger("Role: ", role)) { return false; }
        if (role < 1 || role > 6) { throw std::invalid_argument("Choose a role from 1 to 6."); }
        if (!readInteger("1 Defaults, 2 Custom values: ", mode)) { return false; }
        int id;
        if (mode == 1) { id = world.createPlayer(static_cast<RoleKind>(role - 1)); }
        else if (mode == 2) {
            std::string name;
            bool alive, protection;
            if (!readPlayerValues(name, alive, protection)) { return false; }
            id = world.createPlayer(static_cast<RoleKind>(role - 1), name, alive, protection);
        } else { throw std::invalid_argument("Choose creation mode 1 or 2."); }
        std::cout << "Created player " << id << ".\n";
    } else {
        int id;
        if (!readInteger("Player ID: ", id)) { return false; }
        if (world.findPlayer(id) == nullptr) {
            std::cout << "Unknown player ID.\n";
            return true;
        }
        if (choice == 7) {
            world.removePlayer(id);
            std::cout << "Deleted player " << id << ".\n";
        } else if (choice == 8) {
            std::cout << "Duplicate created with ID " << world.duplicatePlayer(id) << ".\n";
        } else {
            std::string name;
            bool alive, protection;
            if (!readPlayerValues(name, alive, protection)) { return false; }
            world.modifyPlayer(id, name, alive, protection);
            std::cout << "Modified player " << id << ".\n";
        }
    }
    return true;
}

void runEventMenu(World& world) {
    int choice;
    while (true) {
        std::cout << "\nM5 players and events | Next phase: "
                  << (world.getPhase() == NIGHT ? "Night" : "Day")
                  << "\n1 List players\n2 View pending events\n3 Add event\n"
                  << "4 Run one turn\n5 Run several turns\n6 Create player\n"
                  << "7 Delete player\n8 Duplicate player\n9 Modify player\n0 Quit\n";
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
        else if (choice >= 6 && choice <= 9) {
            try { if (!managePlayer(world, choice)) { return; } }
            catch (const std::exception& error) { std::cout << "Player action failed: " << error.what() << '\n'; }
        }
        else { std::cout << "Choose a menu option from 0 to 9.\n"; }
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
        int duplicateId = world.duplicatePlayer(2);
        std::cout << "Duplicated Seer 2 as " << duplicateId << ".\n";
        world.modifyPlayer(duplicateId, "Basil copy", true, false);
        world.findPlayer(2)->describe(std::cout);
        world.findPlayer(duplicateId)->describe(std::cout);
        world.removePlayer(duplicateId);
        std::cout << "Deleted duplicate; original remains.\n";
        world.describePlayers();
    } else { runEventMenu(world); }
    return 0;
}
