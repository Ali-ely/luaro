#include "Console.h"

#include <exception>
#include <istream>
#include <limits>
#include <ostream>
#include <sstream>

Console::Console(World& world, std::istream& input, std::ostream& output)
    : world(world), input(input), output(output) { }

bool Console::readInteger(const std::string& prompt, int minimum, int maximum, int& value) {
    std::string line;
    while (true) {
        output << prompt;
        if (!std::getline(input, line)) { return false; }
        std::istringstream parsed(line);
        int candidate = 0;
        char extra;
        if ((parsed >> candidate) && !(parsed >> extra)
            && candidate >= minimum && candidate <= maximum) {
            value = candidate;
            return true;
        }
        output << "Enter one whole number from " << minimum << " to " << maximum << ".\n";
    }
}

bool Console::readName(std::string& name) {
    while (true) {
        output << "Name: ";
        if (!std::getline(input, name)) { return false; }
        if (name.find_first_not_of(" \t\r\n") != std::string::npos) { return true; }
        output << "A player name cannot be blank.\n";
    }
}

bool Console::readValues(std::string& name, bool& alive, bool& protection) {
    if (!readName(name)) { return false; }
    while (true) {
        int aliveInput = 0, protectionInput = 0;
        if (!readInteger("Alive (0/1): ", 0, 1, aliveInput)
            || !readInteger("Protected (0/1): ", 0, 1, protectionInput)) { return false; }
        if (aliveInput == 0 && protectionInput == 1) {
            output << "A dead player cannot be protected. Enter both flags again.\n";
            continue;
        }
        alive = aliveInput == 1;
        protection = protectionInput == 1;
        return true;
    }
}

bool Console::createPlayer() {
    output << "1 Villager, 2 Seer, 3 Witch, 4 Hunter, 5 Werewolf, 6 AlphaWolf\n";
    int role = 0, mode = 0;
    if (!readInteger("Role (1-6): ", 1, 6, role)
        || !readInteger("1 Defaults, 2 Custom values: ", 1, 2, mode)) { return false; }
    int id = 0;
    if (mode == 1) { id = world.createPlayer(static_cast<RoleKind>(role - 1)); }
    else {
        std::string name;
        bool alive = false, protection = false;
        if (!readValues(name, alive, protection)) { return false; }
        id = world.createPlayer(static_cast<RoleKind>(role - 1), name, alive, protection);
    }
    output << "Created player " << id << ".\n";
    return true;
}

bool Console::managePlayer(int choice) {
    int id = 0;
    if (!readInteger("Player ID: ", 1, std::numeric_limits<int>::max(), id)) { return false; }
    const Player* player = world.findPlayer(id);
    if (player == nullptr) { output << "Unknown player ID.\n"; return true; }
    if (choice == 7) {
        world.removePlayer(id);
        output << "Deleted player " << id << ".\n";
    } else if (choice == 8) {
        const int duplicate = world.duplicatePlayer(id);
        output << "Duplicate created with ID " << duplicate << ".\n";
    } else if (choice == 9) {
        std::string name;
        bool alive = false, protection = false;
        if (!readValues(name, alive, protection)) { return false; }
        world.modifyPlayer(id, name, alive, protection);
        output << "Modified player " << id << ".\n";
    } else {
        player->describe(output); // Administrator details, including dead players.
    }
    return true;
}

bool Console::addEvent() {
    output << "1 Kill, 2 Heal, 3 Vote, 4 Reveal, 5 Poison\n";
    int kind = 0, source = 0, target = 0;
    if (!readInteger("Event (1-5): ", 1, 5, kind)
        || !readInteger("Source ID: ", 1, std::numeric_limits<int>::max(), source)
        || !readInteger("Target ID: ", 1, std::numeric_limits<int>::max(), target)) { return false; }
    if (world.addManualEvent(static_cast<EventKind>(kind - 1), source, target)) {
        output << "Event queued. Run a turn to apply it.\n";
    }
    return true;
}

bool Console::specialAction(int choice) {
    int source = 0, target = 0, potion = 1;
    if (!readInteger("Acting player ID: ", 1, std::numeric_limits<int>::max(), source)
        || !readInteger("Target ID: ", 1, std::numeric_limits<int>::max(), target)) { return false; }
    if (choice == 12 && !readInteger("1 Heal, 2 Poison: ", 1, 2, potion)) { return false; }
    const bool queued = choice == 11 ? world.requestInspection(source, target)
        : world.requestPotion(source, target, potion == 1);
    if (queued) { output << "Special action queued. Run a turn to apply it.\n"; }
    return true;
}

void Console::showMenu() const {
    output << "\nLoup-Garou SimWorld | Next phase: "
           << (world.getPhase() == NIGHT ? "Night" : "Day")
           << " | Turns: " << world.getCompletedTurns()
           << " | Rounds: " << world.getCompletedRounds()
           << " | Undoable edits: " << world.undoCount()
           << "\n1 List players\n2 View pending events\n3 Add event\n"
           << "4 Run one turn\n5 Run several turns\n6 Create player\n"
           << "7 Delete player\n8 Duplicate player\n9 Modify player\n10 Undo\n"
           << "11 Ask a player to inspect\n12 Use a potion\n13 Show player details\n0 Quit\n";
}

bool Console::perform(int choice) {
    switch (choice) {
        case 1: world.describePlayers(); break;
        case 2: world.viewPendingEvents(); break;
        case 3: return addEvent();
        case 4: world.runTurn(); break;
        case 5: {
            int count = 0;
            // Bound a console batch so a huge but valid int cannot monopolize it.
            if (!readInteger("Number of turns (1-1000): ", 1, 1000, count)) { return false; }
            const int completed = world.runTurns(count);
            output << "Completed " << completed << " turn(s).\n";
            break;
        }
        case 6: return createPlayer();
        case 7: case 8: case 9: case 13: return managePlayer(choice);
        case 10: world.undo(); break;
        case 11: case 12: return specialAction(choice);
    }
    return true;
}

void Console::run() {
    while (true) {
        showMenu();
        int choice = 0;
        if (!readInteger("Choice (0-13): ", 0, 13, choice)) {
            output << "\nEnd of input. Goodbye.\n";
            return;
        }
        if (choice == 0) { output << "Goodbye.\n"; return; }
        try {
            if (!perform(choice)) { output << "\nEnd of input. Goodbye.\n"; return; }
        } catch (const std::exception& error) {
            output << "Action failed: " << error.what() << '\n';
        }
    }
}
