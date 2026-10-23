/*
Loup-Garou SimWorld - M1: player hierarchy and independent copying.
Team members: names have not yet been supplied.
World: a village simulation with alternating Night and Day turns (M3 onward).

Hierarchy:
Player (abstract)
  VillageTeam (abstract): Villager, Seer, Witch, Hunter
  WolfTeam (abstract): Werewolf, AlphaWolf

Feature mapping confirmed by the supplied gaps/design documents:
F4: role action contracts in M1; phase behavior follows in M3/M4.
F6: event queue and ordered healing are planned for M4.
F7: identity-preserving clone in M1; commands/undo follow in M5/M6.
F10: deep copying and virtual destruction in M1; event ownership follows in M4.
F11: win detection and statistics are planned for M9.
The complete original numbered specification is needed to verify other labels.

Build from the repository root (create build/ first):
g++ -std=c++17 -Wall Player.cpp Teams.cpp Roles.cpp main.cpp -o build/simworld
Tests: see README.md and tests/m1_tests.cpp.

AI usage log:
2026-10-23: Codex helped consolidate the supplied design, gaps, and course limits.
2026-10-24: User approved M1. Codex implemented the hierarchy, deep copying,
           demonstration, documentation, and ownership tests for user review.
2026-10-24: User approved M2. Codex added custom array-based Stack and Queue
           templates, standalone tests/demo, and the M2 walkthrough.
*/

#include "Roles.h"

#include <iostream>

int main() {
    Villager villager(1, "Alice");
    villager.setSuspicion(5, 7);
    Seer seer(2, "Basil");
    seer.recordInspection(5, WEREWOLF_ROLE);
    Witch witch(3, "Clara");
    witch.recordPotionUse(1, true);
    Hunter hunter(4, "Dalia");
    hunter.recordShot(5);
    Werewolf wolf(5, "Elias");
    wolf.recordTarget(1);
    AlphaWolf alpha(6, "Farah");
    alpha.addPackMember(5);

    // These pointers borrow stack objects; they must not be deleted.
    Player* players[] = {&villager, &seer, &witch, &hunter, &wolf, &alpha};
    std::cout << "M1: all six roles, described through Player pointers\n";
    for (int i = 0; i < 6; ++i) {
        players[i]->describe(std::cout);
    }

    std::cout << "\nIndependent copy demonstration (same ID is intentional):\n";
    Villager* copy = villager.clone();
    copy->setName("Alice's independent copy");
    copy->setSuspicion(5, 99);
    villager.describe(std::cout);
    copy->describe(std::cout);
    // Unlike the borrowed pointers above, clone() returns a new owned object.
    Player* ownedCopy = copy;
    delete ownedCopy;

    std::cout << "\nM1 only: turn actions and the interactive menu arrive later.\n";
    return 0;
}
