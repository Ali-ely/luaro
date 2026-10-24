#include "World.h"
#include "Roles.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Local test results, with no mutable global state.
class Checks {
public:
    int total;
    int failures;
    Checks() : total(0), failures(0) { }
    void expect(bool value, const char* message) {
        ++total;
        if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    }
};

std::string description(const Player* player) {
    std::ostringstream output;
    player->describe(output);
    return output.str();
}

bool hasText(const Player* player, const std::string& text) {
    return description(player).find(text) != std::string::npos;
}

void testCapabilityMatrix(Checks& checks) {
    std::ostringstream output;
    World world(output);
    for (int role = 0; role < 6; ++role) { world.createPlayer(static_cast<RoleKind>(role)); }
    int target = world.createPlayer(VILLAGER_ROLE);
    world.clearUndoHistory();
    for (int role = 0; role < 6; ++role) {
        checks.expect(world.requestInspection(role + 1, target) == (role == SEER_ROLE),
                      "Only Seer supports Inspector through a Player pointer");
        checks.expect(world.requestPotion(role + 1, target, true) == (role == WITCH_ROLE),
                      "Only Witch supports healing through PotionUser");
        checks.expect(world.requestPotion(role + 1, target, false) == (role == WITCH_ROLE),
                      "Only Witch supports poison through PotionUser");
    }
    checks.expect(world.pendingEventCount() == 3 && world.undoCount() == 0,
                  "Supported requests queue events without adding undo commands");
    checks.expect(hasText(world.findPlayer(2), "inspections: 0")
                  && hasText(world.findPlayer(3), "potions used: 0")
                  && !world.findPlayer(target)->isProtected(), "Requests have no immediate gameplay effect");
    checks.expect(output.str().find("This player cannot inspect.") != std::string::npos
                  && output.str().find("This player cannot use potions.") != std::string::npos,
                  "Failed casts produce clear messages");
    output.str("");
    world.viewPendingEvents();
    const std::string firstView = output.str();
    output.str("");
    world.viewPendingEvents();
    checks.expect(firstView == output.str() && world.pendingEventCount() == 3,
                  "Special-action events support non-destructive viewing");
    world.processEvents();
    checks.expect(hasText(world.findPlayer(2), "inspections: 1 [7:Villager]"), "Inspection records exact target role");
    checks.expect(hasText(world.findPlayer(3), "potions used: 2 [7:heal] [7:poison]"), "Potions consumed only on application");
    checks.expect(world.findPlayer(target)->isAlive() && world.findPlayer(target)->isProtected(),
                  "FIFO healing blocks later special poison");
    checks.expect(!world.requestPotion(3, target, true) && !world.requestPotion(3, target, false)
                  && world.pendingEventCount() == 0, "Spent potions rejected before queueing");
}

void testValidation(Checks& checks) {
    std::ostringstream output;
    World world(output);
    int seer = world.createPlayer(SEER_ROLE);
    int witch = world.createPlayer(WITCH_ROLE);
    int target = world.createPlayer(VILLAGER_ROLE);
    world.clearUndoHistory();
    checks.expect(!world.requestInspection(999, target) && !world.requestPotion(-1, target, true),
                  "Unknown and negative acting IDs rejected safely");
    checks.expect(!world.requestInspection(seer, 0) && !world.requestPotion(witch, 999, false),
                  "Missing target IDs rejected");
    checks.expect(!world.requestInspection(seer, seer) && !world.requestPotion(witch, witch, false),
                  "Self inspection and self poison rejected");
    world.modifyPlayer(target, "Dead", false, false);
    checks.expect(!world.requestInspection(seer, target) && !world.requestPotion(witch, target, true),
                  "Dead targets rejected without spending potions");
    world.modifyPlayer(target, "Target", true, false);
    world.modifyPlayer(seer, "Dead Seer", false, false);
    world.modifyPlayer(witch, "Dead Witch", false, false);
    checks.expect(!world.requestInspection(seer, target) && !world.requestPotion(witch, target, false),
                  "Dead actors rejected even when they have the capability");
    checks.expect(world.pendingEventCount() == 0 && hasText(world.findPlayer(witch), "potions used: 0"),
                  "Validation failures do not enqueue or consume");
    const int history = world.undoCount();
    checks.expect(!world.requestInspection(target, seer) && world.undoCount() == history,
                  "Incompatible requests preserve management history");

    World day(output);
    day.createPlayer(SEER_ROLE);
    day.createPlayer(VILLAGER_ROLE);
    day.runTurn();
    int newWitch = day.createPlayer(WITCH_ROLE);
    checks.expect(day.getPhase() == DAY && !day.requestInspection(1, 2)
                  && !day.requestPotion(newWitch, 2, true)
                  && !day.requestPotion(newWitch, 2, false), "Special actions follow Night-only rule");
    checks.expect(day.pendingEventCount() == 0 && day.undoCount() == 1,
                  "Wrong-phase requests leave history and queue unchanged");
}

void testRepeatedRequestsAndAutomaticActions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    int witch = world.createPlayer(WITCH_ROLE);
    int seer = world.createPlayer(SEER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    int wolf = world.createPlayer(WEREWOLF_ROLE);
    checks.expect(world.requestInspection(seer, wolf), "Explicit inspection target accepted");
    checks.expect(world.requestPotion(witch, witch, true) && world.requestPotion(witch, witch, true),
                  "Pending requests do not consume or reserve a potion");
    checks.expect(world.requestPotion(witch, wolf, false) && world.requestPotion(witch, wolf, false),
                  "Repeated poison requests may queue before resolution");
    checks.expect(world.pendingEventCount() == 5 && hasText(world.findPlayer(witch), "potions used: 0"),
                  "All pending requests remain deferred");
    world.runTurn();
    checks.expect(hasText(world.findPlayer(witch), "potions used: 2 [1:heal] [4:poison]"),
                  "Repeated/manual plus automatic requests cannot spend either potion twice");
    checks.expect(world.findPlayer(witch)->isAlive() && !world.findPlayer(wolf)->isAlive()
                  && !world.findPlayer(witch)->isProtected(), "Poison resolves, committed attack blocked, protection expires");
    checks.expect(hasText(world.findPlayer(seer), "[4:Werewolf]")
                  && hasText(world.findPlayer(seer), "[1:Witch]"), "Explicit and automatic inspections both use event delivery");
    checks.expect(world.pendingEventCount() == 0 && world.undoCount() == 0, "Turn drains special events and clears history");
}

// Calls special actions while events are resolving to test the public guard.
class CapabilityProbe : public Event {
private:
    Checks& checks;
public:
    explicit CapabilityProbe(Checks& checks) : Event(1, 2), checks(checks) { }
    void describe(std::ostream& output) const override { output << "Capability probe\n"; }
    void apply(World& world) override {
        int rejected = 0;
        try { world.requestInspection(1, 2); } catch (const std::logic_error&) { ++rejected; }
        try { world.requestPotion(2, 1, true); } catch (const std::logic_error&) { ++rejected; }
        checks.expect(rejected == 2, "User special actions blocked during event/turn execution");
    }
};

void testCopiesUndoAndMissingIds(Checks& checks) {
    std::ostringstream output;
    World world(output);
    const int seer = world.createPlayer(SEER_ROLE);
    const int witch = world.createPlayer(WITCH_ROLE);
    const int target = world.createPlayer(VILLAGER_ROLE);
    world.requestInspection(seer, witch);
    world.requestPotion(witch, witch, true);
    world.processEvents();
    int seerCopy = world.duplicatePlayer(seer);
    int witchCopy = world.duplicatePlayer(witch);
    checks.expect(world.requestInspection(seerCopy, target), "Duplicated Seer retains Inspector capability");
    checks.expect(!world.requestPotion(witchCopy, target, true)
                  && world.requestPotion(witchCopy, target, false), "Duplicated Witch retains capability and spent/available potions");
    world.removePlayer(seerCopy);
    world.undo();
    checks.expect(world.requestInspection(seerCopy, witchCopy), "Delete undo restores Inspector capability");
    const std::string saved = description(world.findPlayer(witchCopy));
    world.modifyPlayer(witchCopy, "Edited", false, false);
    world.undo();
    checks.expect(description(world.findPlayer(witchCopy)) == saved
                  && world.requestPotion(witchCopy, witchCopy, false) == false,
                  "Modify undo restores potion state and validation");
    world.processEvents();
    checks.expect(!world.findPlayer(target)->isAlive()
                  && hasText(world.findPlayer(seerCopy), "[5:Witch]"), "Restored sources resolve original queued IDs");
    checks.expect(!world.requestPotion(witchCopy, witch, false), "Spent poison stays spent after resolution");

    World missing(output);
    missing.createPlayer(SEER_ROLE);
    missing.createPlayer(WITCH_ROLE);
    missing.createPlayer(VILLAGER_ROLE);
    missing.requestInspection(1, 3);
    missing.requestPotion(2, 3, false);
    missing.removePlayer(3);
    missing.processEvents();
    checks.expect(hasText(missing.findPlayer(1), "inspections: 0")
                  && hasText(missing.findPlayer(2), "potions used: 0"), "Deleted target causes no effect or consumption");
    int replacement = missing.createPlayer(VILLAGER_ROLE);
    missing.requestInspection(1, replacement);
    missing.requestPotion(2, replacement, false);
    missing.removePlayer(1);
    missing.removePlayer(2);
    missing.processEvents();
    checks.expect(missing.findPlayer(replacement)->isAlive() && replacement > 3,
                  "Deleted sources ignored and stale IDs never retarget replacements");

    World guards(output);
    guards.createPlayer(SEER_ROLE);
    guards.createPlayer(WITCH_ROLE);
    guards.enqueueEvent(new CapabilityProbe(checks));
    guards.processEvents();
    guards.enqueueEvent(new CapabilityProbe(checks));
    guards.runTurn();
}

void testInterfaceDestructionAndStress(Checks& checks) {
    Inspector* inspector = new Seer(1);
    PotionUser* potionUser = new Witch(2);
    delete inspector;
    delete potionUser;
    checks.expect(true, "Virtual destruction through either interface succeeds");
    for (int repetition = 0; repetition < 20; ++repetition) {
        std::ostringstream output;
        World world(output);
        world.createPlayer(SEER_ROLE);
        world.createPlayer(WITCH_ROLE);
        world.createPlayer(WEREWOLF_ROLE);
        world.createPlayer(HUNTER_ROLE);
        world.requestInspection(1, 3);
        world.requestPotion(2, 4, true);
        world.requestPotion(2, 3, false);
        world.runTurns(2);
        checks.expect(world.pendingEventCount() == 0, "Mixed special actions and role turns drain queue");
        int copy = world.duplicatePlayer(1);
        world.modifyPlayer(copy, "Snapshot", true, false);
        world.removePlayer(copy);
        world.undo();
        world.undo();

        world.requestInspection(copy, 2);
    }
}

int main() {
    Checks checks;
    std::cout << "Running M7 capability tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testCapabilityMatrix(checks);
    testValidation(checks);
    testRepeatedRequestsAndAutomaticActions(checks);
    testCopiesUndoAndMissingIds(checks);
    testInterfaceDestructionAndStress(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding capability, event, or snapshot allocations");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.total << " checks, " << checks.failures << " failures.\n";
    return checks.failures == 0 ? 0 : 1;
}
