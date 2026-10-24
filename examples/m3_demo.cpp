#include "World.h"

#include <iostream>

int main() {
    World world;
    // Creation order deliberately differs from night priority order.
    world.createPlayer(ALPHA_WOLF_ROLE, "Farah");
    int aliceId = world.createPlayer(VILLAGER_ROLE, "Alice");
    world.createPlayer(WEREWOLF_ROLE, "Elias");
    int hunterId = world.createPlayer(HUNTER_ROLE, "Dalia");
    world.createPlayer(WITCH_ROLE, "Clara");
    world.createPlayer(SEER_ROLE, "Basil");

    std::cout << "M3: players in their original list order\n";
    world.describePlayers();
    std::cout << "\nRun one turn:\n";
    world.runTurn();
    std::cout << "\nRun the next three turns:\n";
    world.runTurns(3);
    std::cout << "Completed turns: " << world.getCompletedTurns()
              << "; completed rounds: " << world.getCompletedRounds() << '\n';

    std::cout << "\nMark Alice dead for the skip demonstration, then run one turn:\n";
    world.findPlayer(aliceId)->setAlive(false);
    world.runTurn();
    std::cout << "\nDead players remain listed:\n";
    world.describePlayers();

    std::cout << "\nRemove Dalia administratively (no death reaction):\n";
    world.removePlayer(hunterId);
    int newId = world.createPlayer(VILLAGER_ROLE, "Gamal");
    std::cout << "New player ID: " << newId << " (removed IDs are not reused).\n";
    world.describePlayers();
    std::cout << "\nM3 world demonstration using the current event effects.\n";
    // World automatically deletes all remaining owned players here.
    return 0;
}
