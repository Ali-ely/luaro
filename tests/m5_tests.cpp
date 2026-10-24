#include "World.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Small test counter, with no global mutable state.
class Checks {
public:
    int total;
    int failures;
    Checks() : total(0), failures(0) { }
    void expect(bool condition, const char* message) {
        ++total;
        if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    }
};

std::string state(const Player* player) {
    std::ostringstream text;
    player->describe(text);

    const std::string description = text.str();
    return description.substr(description.find('|'));
}

// Check that event callbacks cannot mutate the player collection or its state.
class ManagementProbe : public Event {
private:
    Checks& checks;
public:
    explicit ManagementProbe(Checks& checks) : Event(1, 1), checks(checks) { }
    void describe(std::ostream& output) const override { output << "Management probe\n"; }
    void apply(World& world) override {
        int blocked = 0;
        try { world.createPlayer(VILLAGER_ROLE); } catch (const std::logic_error&) { ++blocked; }
        try { world.removePlayer(1); } catch (const std::logic_error&) { ++blocked; }
        try { world.duplicatePlayer(1); } catch (const std::logic_error&) { ++blocked; }
        try { world.modifyPlayer(1, "Edit", true, false); } catch (const std::logic_error&) { ++blocked; }
        checks.expect(blocked == 4, "All management actions blocked during event resolution");
    }
};

void testManagement(Checks& checks) {
    std::ostringstream output;
    World world(output);
    for (int role = 0; role < 6; ++role) {
        int id = world.createPlayer(static_cast<RoleKind>(role));
        const Player* player = world.findPlayer(id);
        checks.expect(player->getRole() == static_cast<RoleKind>(role), "Default role");
        checks.expect(player->getName() == roleName(static_cast<RoleKind>(role))
                      && player->isAlive() && !player->isProtected(), "Default attributes");
    }
    int custom = world.createPlayer(HUNTER_ROLE, "Custom Hunter", false, false);
    checks.expect(!world.findPlayer(custom)->isAlive(), "Create with custom dead state");
    int protectedId = world.createPlayer(VILLAGER_ROLE, "Protected", true, true);
    checks.expect(world.findPlayer(protectedId)->isProtected(), "Create protected living player");
    int before = world.playerCount();
    int rejected = 0;
    try { world.createPlayer(VILLAGER_ROLE, " "); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.createPlayer(VILLAGER_ROLE, "Bad", false, true); } catch (const std::invalid_argument&) { ++rejected; }

    try { world.createPlayer(static_cast<RoleKind>(6)); } catch (const std::invalid_argument&) { ++rejected; }
    checks.expect(rejected == 3 && world.playerCount() == before, "Bad creation leaves collection unchanged");
    checks.expect(world.createPlayer(SEER_ROLE) == protectedId + 1, "Invalid creation consumes no IDs");
    checks.expect(world.duplicatePlayer(999) == 0 && !world.removePlayer(999)
                  && !world.modifyPlayer(999, "Missing", true, false), "Unknown IDs harmless");

    const std::string original = state(world.findPlayer(1));
    rejected = 0;
    try { world.modifyPlayer(1, "\t ", false, false); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.modifyPlayer(1, "Changed", false, true); } catch (const std::invalid_argument&) { ++rejected; }
    checks.expect(rejected == 2 && state(world.findPlayer(1)) == original, "Invalid edits are atomic");
    const Player* address = world.findPlayer(1);
    checks.expect(world.modifyPlayer(1, "Renamed", true, true), "Valid edit succeeds");
    checks.expect(world.findPlayer(1) == address && world.playerAt(0) == address
                  && address->getId() == 1 && address->isProtected(), "Edit preserves identity and position");
    world.modifyPlayer(1, "Dead", false, false);
    checks.expect(!address->isAlive() && !address->isProtected(), "Administrative death clears protection");
    world.modifyPlayer(custom, "Revived Hunter", true, false);
    world.modifyPlayer(custom, "Dead Hunter", false, false);
    checks.expect(world.pendingEventCount() == 0, "Administrative edits trigger no death reaction");
    world.enqueueEvent(new ManagementProbe(checks));
    world.processEvents();
    world.enqueueEvent(new ManagementProbe(checks));
    world.runTurn();
    checks.expect(world.modifyPlayer(1, "After turn", true, false), "Management available after turn");
}

void testDuplicates(Checks& checks) {
    for (int role = 0; role < 6; ++role) {
        std::ostringstream output;
        World world(output);
        int sourceId = world.createPlayer(static_cast<RoleKind>(role), "Original");
        world.createPlayer(VILLAGER_ROLE, "Target");
        world.createPlayer(WEREWOLF_ROLE, "Packmate");
        Player* source = world.findPlayer(sourceId);

        source->act(world);
        if (role == HUNTER_ROLE) {
            source->setAlive(false);
            source->onDeath(world);
        }
        world.processEvents();
        source->setAlive(true);
        source->setProtected(true);
        const std::string snapshot = state(source);
        const int copyId = world.duplicatePlayer(sourceId);
        Player* copy = world.findPlayer(copyId);
        checks.expect(copyId == 4 && copy != source, "Duplicate gets fresh ID and object");
        checks.expect(copy->getRole() == source->getRole() && state(copy) == snapshot,
                      "Duplicate preserves concrete role and all described state");
        checks.expect(world.playerAt(3) == copy && world.playerAt(0) == source,
                      "Duplicate appended without moving source");
        Player* identityClone = copy->clone();
        checks.expect(identityClone->getId() == copyId, "Plain clone still preserves identity");
        delete identityClone;
        world.modifyPlayer(copyId, "Edited copy", false, false);
        checks.expect(state(source) == snapshot, "Editing copy leaves original unchanged");
        int deadCopy = world.duplicatePlayer(copyId);
        checks.expect(!world.findPlayer(deadCopy)->isAlive()
                      && state(world.findPlayer(deadCopy)) == state(copy), "Dead state preserved by duplicate");
        world.removePlayer(sourceId);
        checks.expect(copy->getName() == "Edited copy", "Copy survives source deletion");
        const std::string retained = state(copy);
        world.removePlayer(deadCopy);
        checks.expect(state(copy) == retained, "Copy data survives sibling deletion");
        checks.expect(world.createPlayer(VILLAGER_ROLE) > deadCopy, "Deleted IDs never reused");
        world.enqueueEvent(new KillEvent(3, copyId));
        world.removePlayer(copyId);
        world.processEvents();
        checks.expect(world.pendingEventCount() == 0, "Queued reference to removed copy is safe");
    }
}

int main() {
    Checks checks;
    std::cout << "Running M5 player-management tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testManagement(checks);
    testDuplicates(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding allocations after management tests");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.total << " checks, " << checks.failures << " failures.\n";
    return checks.failures == 0 ? 0 : 1;
}
