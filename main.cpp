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
        world.clearUndoHistory();
        if (argc == 2) {
            world.describePlayers();
            world.requestInspection(3, 5);
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
            world.undo();
            world.describePlayers();
            world.runTurns(20);
            std::istringstream demoInput("14\n13\n2\n0\n");
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
