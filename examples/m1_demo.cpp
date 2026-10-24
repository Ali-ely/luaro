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
