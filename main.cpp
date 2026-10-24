/*
Loup-Garou SimWorld - M3: world ownership and Night/Day turns.
Team members: names have not yet been supplied.
World: a village simulation with alternating Night and Day turns.

Hierarchy:
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf

Feature mapping confirmed by the supplied gaps/design documents:
F4: phase-dependent action messages in M3; event effects follow in M4.
F6: event queue and ordered healing are planned for M4.
F7: identity-preserving clone in M1; commands/undo follow in M5/M6.
F10: deep copying, virtual destruction, and World player ownership; events follow in M4.
F11: win detection and statistics are planned for M9.
The complete original numbered specification is needed to verify other labels.

Build from the repository root (create build/ first):
g++ -std=c++17 -Wall Player.cpp Teams.cpp Roles.cpp World.cpp main.cpp -o build/simworld
Tests: see README.md and tests/m1_tests.cpp through tests/m3_tests.cpp.

AI usage log:
2026-10-23: Codex helped consolidate the supplied design, gaps, and course limits.
2026-10-24: User approved M1. Codex implemented the hierarchy, deep copying,
           demonstration, documentation, and ownership tests for user review.
2026-10-24: User approved M2. Codex added custom array-based Stack and Queue
           templates, standalone tests/demo, and the M2 walkthrough.
2026-10-24: User approved M3. Codex added World ownership, unique IDs, phase
           tracking, virtual turn dispatch, role messages, and M3 tests/docs.
*/

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
    std::cout << "\nM3 only: actions are messages. Event effects arrive in M4.\n";
    // World automatically deletes all remaining owned players here.
    return 0;
}
