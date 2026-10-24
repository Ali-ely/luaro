#include "Roles.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// A small test counter, kept local to main rather than in global variables.
class Checks {
private:
    int total;
    int failures;

public:
    Checks() : total(0), failures(0) {}
    void expect(bool passed, const char* message) {
        ++total;
        if (!passed) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
    int getTotal() const { return total; }
    int getFailures() const { return failures; }
};

void testVillager(Checks& checks) {
    Villager original(1, "Original");
    original.setSuspicion(20, 7);
    original.setProtected(true);
    Villager copy(original);
    Villager assigned(2, "Old destination");
    assigned.setSuspicion(40, 2);
    assigned.setSuspicion(41, 3);
    checks.expect(&(assigned = original) == &assigned, "Villager: assignment returns itself");
    Villager* clone = original.clone();

    checks.expect(copy.getSuspicionCount() == 1 && assigned.getSuspicionCount() == 1
                  && clone->getSuspicionCount() == 1, "Villager: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "Villager: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "Villager: copies preserve protection");

    original.setSuspicion(20, 99);
    original.setSuspicion(21, 4);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getSuspicion(0).score == 7 && assigned.getSuspicion(0).score == 7
                  && clone->getSuspicion(0).score == 7, "Villager: copies retain data after source mutation");
    checks.expect(copy.getSuspicionCount() == 1 && assigned.getSuspicionCount() == 1
                  && clone->getSuspicionCount() == 1, "Villager: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "Villager: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "Villager: alive flags are independent");

    copy.setSuspicion(20, 55);
    clone->setSuspicion(20, 42);
    checks.expect(assigned.getSuspicionCount() == 1 && assigned.getSuspicion(0).score == 7,
                  "Villager: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getSuspicionCount() == 1 && assigned.getSuspicion(0).score == 7,
                  "Villager: populated self-assignment");
    Villager empty(3);
    Villager emptyCopy(empty);
    Villager* emptyClone = empty.clone();
    checks.expect(emptyCopy.getSuspicionCount() == 0 && emptyClone->getSuspicionCount() == 0,
                  "Villager: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getSuspicionCount() == 0 && assigned.getId() == 3,
                  "Villager: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getSuspicionCount() == 0, "Villager: empty self-assignment");

    {
        Villager temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.setSuspicion(20, 7);
        assigned = temporary;
    }
    checks.expect(assigned.getSuspicion(0).score == 7 && assigned.getId() == 8
                  && !assigned.isAlive(), "Villager: assignment survives source destruction");

    Villager* survivingClone = nullptr;
    {
        Villager temporary(9);
        temporary.setSuspicion(20, 7);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getSuspicion(0).score == 7,
                  "Villager: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getSuspicion(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Villager: empty access rejected");
    rejected = false;
    try {
        assigned.getSuspicion(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Villager: negative index rejected");
    rejected = false;
    try {
        assigned.getSuspicion(assigned.getSuspicionCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Villager: past-end access rejected");
}

void testSeer(Checks& checks) {
    Seer original(1, "Original");
    original.recordInspection(20, WEREWOLF_ROLE);
    original.setProtected(true);
    Seer copy(original);
    Seer assigned(2, "Old destination");
    assigned.recordInspection(40, ALPHA_WOLF_ROLE);
    assigned.recordInspection(41, WITCH_ROLE);
    checks.expect(&(assigned = original) == &assigned, "Seer: assignment returns itself");
    Seer* clone = original.clone();

    checks.expect(copy.getInspectionCount() == 1 && assigned.getInspectionCount() == 1
                  && clone->getInspectionCount() == 1, "Seer: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "Seer: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "Seer: copies preserve protection");

    original.recordInspection(21, VILLAGER_ROLE);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getInspection(0).role == WEREWOLF_ROLE && assigned.getInspection(0).role == WEREWOLF_ROLE
                  && clone->getInspection(0).role == WEREWOLF_ROLE, "Seer: copies retain data after source mutation");
    checks.expect(copy.getInspectionCount() == 1 && assigned.getInspectionCount() == 1
                  && clone->getInspectionCount() == 1, "Seer: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "Seer: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "Seer: alive flags are independent");

    copy.recordInspection(22, SEER_ROLE);
    clone->recordInspection(23, HUNTER_ROLE);
    checks.expect(assigned.getInspectionCount() == 1 && assigned.getInspection(0).role == WEREWOLF_ROLE,
                  "Seer: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getInspectionCount() == 1 && assigned.getInspection(0).role == WEREWOLF_ROLE,
                  "Seer: populated self-assignment");
    Seer empty(3);
    Seer emptyCopy(empty);
    Seer* emptyClone = empty.clone();
    checks.expect(emptyCopy.getInspectionCount() == 0 && emptyClone->getInspectionCount() == 0,
                  "Seer: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getInspectionCount() == 0 && assigned.getId() == 3,
                  "Seer: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getInspectionCount() == 0, "Seer: empty self-assignment");

    {
        Seer temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.recordInspection(20, WEREWOLF_ROLE);
        assigned = temporary;
    }
    checks.expect(assigned.getInspection(0).role == WEREWOLF_ROLE && assigned.getId() == 8
                  && !assigned.isAlive(), "Seer: assignment survives source destruction");

    Seer* survivingClone = nullptr;
    {
        Seer temporary(9);
        temporary.recordInspection(20, WEREWOLF_ROLE);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getInspection(0).role == WEREWOLF_ROLE,
                  "Seer: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getInspection(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Seer: empty access rejected");
    rejected = false;
    try {
        assigned.getInspection(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Seer: negative index rejected");
    rejected = false;
    try {
        assigned.getInspection(assigned.getInspectionCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Seer: past-end access rejected");
}

void testWitch(Checks& checks) {
    Witch original(1, "Original");
    original.recordPotionUse(20, true);
    original.setProtected(true);
    Witch copy(original);
    Witch assigned(2, "Old destination");
    assigned.recordPotionUse(40, true);
    assigned.recordPotionUse(41, false);
    checks.expect(&(assigned = original) == &assigned, "Witch: assignment returns itself");
    Witch* clone = original.clone();

    checks.expect(copy.getPotionUseCount() == 1 && assigned.getPotionUseCount() == 1
                  && clone->getPotionUseCount() == 1, "Witch: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "Witch: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "Witch: copies preserve protection");

    original.recordPotionUse(21, false);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getPotionUse(0).targetId == 20 && assigned.getPotionUse(0).targetId == 20
                  && clone->getPotionUse(0).targetId == 20, "Witch: copies retain data after source mutation");
    checks.expect(copy.getPotionUseCount() == 1 && assigned.getPotionUseCount() == 1
                  && clone->getPotionUseCount() == 1, "Witch: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "Witch: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "Witch: alive flags are independent");

    copy.recordPotionUse(22, false);
    clone->recordPotionUse(23, false);
    checks.expect(assigned.getPotionUseCount() == 1 && assigned.getPotionUse(0).targetId == 20,
                  "Witch: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getPotionUseCount() == 1 && assigned.getPotionUse(0).targetId == 20,
                  "Witch: populated self-assignment");
    Witch empty(3);
    Witch emptyCopy(empty);
    Witch* emptyClone = empty.clone();
    checks.expect(emptyCopy.getPotionUseCount() == 0 && emptyClone->getPotionUseCount() == 0,
                  "Witch: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getPotionUseCount() == 0 && assigned.getId() == 3,
                  "Witch: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getPotionUseCount() == 0, "Witch: empty self-assignment");

    {
        Witch temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.recordPotionUse(20, true);
        assigned = temporary;
    }
    checks.expect(assigned.getPotionUse(0).targetId == 20 && assigned.getId() == 8
                  && !assigned.isAlive(), "Witch: assignment survives source destruction");

    Witch* survivingClone = nullptr;
    {
        Witch temporary(9);
        temporary.recordPotionUse(20, true);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getPotionUse(0).targetId == 20,
                  "Witch: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getPotionUse(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Witch: empty access rejected");
    rejected = false;
    try {
        assigned.getPotionUse(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Witch: negative index rejected");
    rejected = false;
    try {
        assigned.getPotionUse(assigned.getPotionUseCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Witch: past-end access rejected");
}

void testHunter(Checks& checks) {
    Hunter original(1, "Original");
    original.recordShot(20);
    original.setProtected(true);
    Hunter copy(original);
    Hunter assigned(2, "Old destination");
    assigned.recordShot(40);
    assigned.recordShot(41);
    checks.expect(&(assigned = original) == &assigned, "Hunter: assignment returns itself");
    Hunter* clone = original.clone();

    checks.expect(copy.getShotCount() == 1 && assigned.getShotCount() == 1
                  && clone->getShotCount() == 1, "Hunter: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "Hunter: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "Hunter: copies preserve protection");

    original.recordShot(21);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getShot(0) == 20 && assigned.getShot(0) == 20
                  && clone->getShot(0) == 20, "Hunter: copies retain data after source mutation");
    checks.expect(copy.getShotCount() == 1 && assigned.getShotCount() == 1
                  && clone->getShotCount() == 1, "Hunter: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "Hunter: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "Hunter: alive flags are independent");

    copy.recordShot(22);
    clone->recordShot(23);
    checks.expect(assigned.getShotCount() == 1 && assigned.getShot(0) == 20,
                  "Hunter: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getShotCount() == 1 && assigned.getShot(0) == 20,
                  "Hunter: populated self-assignment");
    Hunter empty(3);
    Hunter emptyCopy(empty);
    Hunter* emptyClone = empty.clone();
    checks.expect(emptyCopy.getShotCount() == 0 && emptyClone->getShotCount() == 0,
                  "Hunter: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getShotCount() == 0 && assigned.getId() == 3,
                  "Hunter: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getShotCount() == 0, "Hunter: empty self-assignment");

    {
        Hunter temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.recordShot(20);
        assigned = temporary;
    }
    checks.expect(assigned.getShot(0) == 20 && assigned.getId() == 8
                  && !assigned.isAlive(), "Hunter: assignment survives source destruction");

    Hunter* survivingClone = nullptr;
    {
        Hunter temporary(9);
        temporary.recordShot(20);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getShot(0) == 20,
                  "Hunter: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getShot(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Hunter: empty access rejected");
    rejected = false;
    try {
        assigned.getShot(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Hunter: negative index rejected");
    rejected = false;
    try {
        assigned.getShot(assigned.getShotCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Hunter: past-end access rejected");
}

void testWerewolf(Checks& checks) {
    Werewolf original(1, "Original");
    original.recordTarget(20);
    original.setProtected(true);
    Werewolf copy(original);
    Werewolf assigned(2, "Old destination");
    assigned.recordTarget(40);
    assigned.recordTarget(41);
    checks.expect(&(assigned = original) == &assigned, "Werewolf: assignment returns itself");
    Werewolf* clone = original.clone();

    checks.expect(copy.getTargetCount() == 1 && assigned.getTargetCount() == 1
                  && clone->getTargetCount() == 1, "Werewolf: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "Werewolf: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "Werewolf: copies preserve protection");

    original.recordTarget(21);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getTarget(0) == 20 && assigned.getTarget(0) == 20
                  && clone->getTarget(0) == 20, "Werewolf: copies retain data after source mutation");
    checks.expect(copy.getTargetCount() == 1 && assigned.getTargetCount() == 1
                  && clone->getTargetCount() == 1, "Werewolf: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "Werewolf: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "Werewolf: alive flags are independent");

    copy.recordTarget(22);
    clone->recordTarget(23);
    checks.expect(assigned.getTargetCount() == 1 && assigned.getTarget(0) == 20,
                  "Werewolf: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getTargetCount() == 1 && assigned.getTarget(0) == 20,
                  "Werewolf: populated self-assignment");
    Werewolf empty(3);
    Werewolf emptyCopy(empty);
    Werewolf* emptyClone = empty.clone();
    checks.expect(emptyCopy.getTargetCount() == 0 && emptyClone->getTargetCount() == 0,
                  "Werewolf: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getTargetCount() == 0 && assigned.getId() == 3,
                  "Werewolf: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getTargetCount() == 0, "Werewolf: empty self-assignment");

    {
        Werewolf temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.recordTarget(20);
        assigned = temporary;
    }
    checks.expect(assigned.getTarget(0) == 20 && assigned.getId() == 8
                  && !assigned.isAlive(), "Werewolf: assignment survives source destruction");

    Werewolf* survivingClone = nullptr;
    {
        Werewolf temporary(9);
        temporary.recordTarget(20);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getTarget(0) == 20,
                  "Werewolf: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getTarget(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Werewolf: empty access rejected");
    rejected = false;
    try {
        assigned.getTarget(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Werewolf: negative index rejected");
    rejected = false;
    try {
        assigned.getTarget(assigned.getTargetCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "Werewolf: past-end access rejected");
}

void testAlphaWolf(Checks& checks) {
    AlphaWolf original(1, "Original");
    original.addPackMember(20);
    original.setProtected(true);
    AlphaWolf copy(original);
    AlphaWolf assigned(2, "Old destination");
    assigned.addPackMember(40);
    assigned.addPackMember(41);
    checks.expect(&(assigned = original) == &assigned, "AlphaWolf: assignment returns itself");
    AlphaWolf* clone = original.clone();

    checks.expect(copy.getPackMemberCount() == 1 && assigned.getPackMemberCount() == 1
                  && clone->getPackMemberCount() == 1, "AlphaWolf: all copies preserve array size");
    checks.expect(copy.getId() == 1 && assigned.getId() == 1 && clone->getId() == 1,
                  "AlphaWolf: copies preserve ID");
    checks.expect(copy.isProtected() && assigned.isProtected() && clone->isProtected(),
                  "AlphaWolf: copies preserve protection");

    original.addPackMember(21);
    original.setName("Changed");
    original.setAlive(false);
    checks.expect(copy.getPackMember(0) == 20 && assigned.getPackMember(0) == 20
                  && clone->getPackMember(0) == 20, "AlphaWolf: copies retain data after source mutation");
    checks.expect(copy.getPackMemberCount() == 1 && assigned.getPackMemberCount() == 1
                  && clone->getPackMemberCount() == 1, "AlphaWolf: source growth does not grow copies");
    checks.expect(copy.getName() == "Original" && assigned.getName() == "Original"
                  && clone->getName() == "Original", "AlphaWolf: names are independent");
    checks.expect(copy.isAlive() && assigned.isAlive() && clone->isAlive(),
                  "AlphaWolf: alive flags are independent");

    copy.addPackMember(22);
    clone->addPackMember(23);
    checks.expect(assigned.getPackMemberCount() == 1 && assigned.getPackMember(0) == 20,
                  "AlphaWolf: modifying other copies leaves assignment independent");
    delete clone;

    assigned = assigned;
    checks.expect(assigned.getPackMemberCount() == 1 && assigned.getPackMember(0) == 20,
                  "AlphaWolf: populated self-assignment");
    AlphaWolf empty(3);
    AlphaWolf emptyCopy(empty);
    AlphaWolf* emptyClone = empty.clone();
    checks.expect(emptyCopy.getPackMemberCount() == 0 && emptyClone->getPackMemberCount() == 0,
                  "AlphaWolf: empty copy and clone");
    delete emptyClone;
    assigned = empty;
    checks.expect(assigned.getPackMemberCount() == 0 && assigned.getId() == 3,
                  "AlphaWolf: assigning empty releases previous data");
    assigned = assigned;
    checks.expect(assigned.getPackMemberCount() == 0, "AlphaWolf: empty self-assignment");

    {
        AlphaWolf temporary(8, "Temporary");
        temporary.setAlive(false);
        temporary.addPackMember(20);
        assigned = temporary;
    }
    checks.expect(assigned.getPackMember(0) == 20 && assigned.getId() == 8
                  && !assigned.isAlive(), "AlphaWolf: assignment survives source destruction");

    AlphaWolf* survivingClone = nullptr;
    {
        AlphaWolf temporary(9);
        temporary.addPackMember(20);
        survivingClone = temporary.clone();
    }
    checks.expect(survivingClone->getPackMember(0) == 20,
                  "AlphaWolf: clone survives source destruction");

    Player* base = survivingClone;
    delete base;

    bool rejected = false;
    try {
        empty.getPackMember(0);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "AlphaWolf: empty access rejected");
    rejected = false;
    try {
        assigned.getPackMember(-1);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "AlphaWolf: negative index rejected");
    rejected = false;
    try {
        assigned.getPackMember(assigned.getPackMemberCount());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected, "AlphaWolf: past-end access rejected");
}

void testPolymorphism(Checks& checks) {
    Villager villager(1);
    villager.setSuspicion(20, 8);
    Seer seer(2);
    seer.recordInspection(20, WEREWOLF_ROLE);
    Witch witch(3);
    witch.recordPotionUse(20, true);
    Hunter hunter(4);
    hunter.recordShot(20);
    Werewolf wolf(5);
    wolf.recordTarget(20);
    AlphaWolf alpha(6);
    alpha.addPackMember(5);
    Player* players[] = {&villager, &seer, &witch, &hunter, &wolf, &alpha};
    const RoleKind roles[] = {VILLAGER_ROLE, SEER_ROLE, WITCH_ROLE, HUNTER_ROLE,
                              WEREWOLF_ROLE, ALPHA_WOLF_ROLE};

    for (int i = 0; i < 6; ++i) {
        Player* clone = players[i]->clone();
        checks.expect(clone != players[i] && clone->getRole() == roles[i],
                      "Polymorphic clone preserves concrete role");
        checks.expect(clone->isWolf() == (i >= 4), "Team identity is inherited");
        std::ostringstream originalText;
        std::ostringstream cloneText;
        players[i]->describe(originalText);
        clone->describe(cloneText);
        checks.expect(originalText.str() == cloneText.str(),
                      "Polymorphic description includes copied state");
        delete clone;
    }
    checks.expect(seer.nightOrder() == 0 && witch.nightOrder() == 0
                  && wolf.nightOrder() == 1 && alpha.nightOrder() == 1,
                  "Night priority puts Seer and Witch before wolves");
    checks.expect(villager.voteWeight() == 1 && seer.voteWeight() == 1
                  && witch.voteWeight() == 1 && hunter.voteWeight() == 1
                  && wolf.voteWeight() == 1 && alpha.voteWeight() == 2,
                  "Only AlphaWolf has double voting weight");
}

void testValidationAndGrowth(Checks& checks) {
    bool rejected = false;
    try {
        Villager invalid(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "Nonpositive player ID rejected");

    rejected = false;
    try {
        Seer invalid(1, " \t\r\n");
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "Blank name rejected");

    Villager villager(1, "Valid");
    rejected = false;
    try {
        villager.setName("");
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected && villager.getName() == "Valid", "Bad name leaves existing name unchanged");

    rejected = false;
    try {
        villager.setSuspicion(2, -1);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected && villager.getSuspicionCount() == 0, "Negative suspicion rejected");

    villager.setProtected(true);
    villager.setAlive(false);
    checks.expect(!villager.isProtected(), "Death clears protection");
    rejected = false;
    try {
        villager.setProtected(true);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected && !villager.isProtected(), "Dead player cannot be protected");

    Seer seer(2);
    Witch witch(3);
    Hunter hunter(4);
    Werewolf wolf(5);
    AlphaWolf alpha(6);
    for (int i = 1; i <= 100; ++i) {
        villager.setSuspicion(i, i);
        seer.recordInspection(i, VILLAGER_ROLE);
        hunter.recordShot(i);
        wolf.recordTarget(i);
        alpha.addPackMember(i);
    }
    checks.expect(villager.getSuspicionCount() == 100 && seer.getInspectionCount() == 100
                  && hunter.getShotCount() == 100 && wolf.getTargetCount() == 100
                  && alpha.getPackMemberCount() == 100, "Role arrays grow to 100 entries");
    checks.expect(villager.getSuspicion(99).score == 100 && seer.getInspection(99).playerId == 100
                  && hunter.getShot(99) == 100 && wolf.getTarget(99) == 100
                  && alpha.getPackMember(99) == 100, "Growth preserves the last entry");
    villager.setSuspicion(1, 500);
    alpha.addPackMember(1);
    checks.expect(villager.getSuspicionCount() == 100 && villager.getSuspicion(0).score == 500,
                  "Updating suspicion replaces the matching entry");
    checks.expect(alpha.getPackMemberCount() == 100, "Duplicate pack member is ignored");

    checks.expect(witch.hasHealingPotion() && witch.hasPoisonPotion(), "Witch starts with both potions");
    checks.expect(witch.recordPotionUse(2, true) && !witch.hasHealingPotion()
                  && witch.hasPoisonPotion(), "Healing potion can be consumed once");
    checks.expect(!witch.recordPotionUse(3, true) && witch.getPotionUseCount() == 1,
                  "Repeated healing use does not change the log");
    checks.expect(witch.recordPotionUse(2, false) && !witch.hasPoisonPotion(),
                  "Poison potion can be consumed once");
    checks.expect(!witch.recordPotionUse(3, false) && witch.getPotionUseCount() == 2,
                  "Repeated poison use does not change the log");
    rejected = false;
    try {
        villager.setSuspicion(0, 2);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "villager: invalid referenced ID rejected");
    rejected = false;
    try {
        seer.recordInspection(0, VILLAGER_ROLE);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "seer: invalid referenced ID rejected");
    rejected = false;
    try {
        witch.recordPotionUse(0, true);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "witch: invalid referenced ID rejected");
    rejected = false;
    try {
        hunter.recordShot(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "hunter: invalid referenced ID rejected");
    rejected = false;
    try {
        wolf.recordTarget(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "wolf: invalid referenced ID rejected");
    rejected = false;
    try {
        alpha.addPackMember(0);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    checks.expect(rejected, "alpha: invalid referenced ID rejected");
}

void runTests(Checks& checks) {
    testVillager(checks);
    testSeer(checks);
    testWitch(checks);
    testHunter(checks);
    testWerewolf(checks);
    testAlphaWolf(checks);
    testPolymorphism(checks);
    testValidationAndGrowth(checks);
}

int main() {
    Checks checks;

    std::cout << "Running M1 ownership and copying tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before;
    _CrtMemState after;
    _CrtMemState difference;
    _CrtMemCheckpoint(&before);
#endif

    runTests(checks);

#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding allocations after all tests");
    if (leaked) {
        _CrtMemDumpStatistics(&difference);
    }
#endif

    std::cout << checks.getTotal() << " checks, " << checks.getFailures() << " failures.\n";
    return checks.getFailures() == 0 ? 0 : 1;
}
