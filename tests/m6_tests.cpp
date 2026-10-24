#include "World.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Local test state; no owning pointers outlive their World.
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

void testSequenceAndFailures(Checks& checks) {
    std::ostringstream output;
    World world(output);
    checks.expect(!world.undo() && output.str().find("Nothing to undo.") != std::string::npos,
                  "Empty undo prints a message and continues");
    int original = world.createPlayer(VILLAGER_ROLE, "Original", true, true);
    world.clearUndoHistory();
    const std::string originalState = description(world.findPlayer(original));
    const int created = world.createPlayer(SEER_ROLE);
    const int duplicate = world.duplicatePlayer(original);
    world.modifyPlayer(duplicate, "Edited", false, false);
    const std::string edited = description(world.findPlayer(duplicate));
    world.removePlayer(duplicate);
    checks.expect(world.undoCount() == 4, "Four successful edits are recorded");
    checks.expect(world.undo() && description(world.findPlayer(duplicate)) == edited
                  && world.playerAt(2)->getId() == duplicate, "Undo delete restores last position and state");
    checks.expect(world.undo() && world.findPlayer(duplicate)->isProtected()
                  && world.findPlayer(duplicate)->getName() == "Original", "Undo modify restores prior flags and name");
    checks.expect(world.undo() && world.findPlayer(duplicate) == nullptr
                  && description(world.findPlayer(original)) == originalState, "Undo duplicate preserves source");
    checks.expect(world.undo() && world.findPlayer(created) == nullptr
                  && world.undoCount() == 0 && world.playerCount() == 1, "Undo create finishes LIFO sequence");
    checks.expect(world.getCompletedTurns() == 0 && world.getPhase() == NIGHT,
                  "Edits and undo do not advance gameplay");
    checks.expect(world.createPlayer(HUNTER_ROLE) > duplicate, "Undo never recycles assigned IDs");
    const int count = world.undoCount();
    int rejected = 0;
    try { world.modifyPlayer(original, " ", false, false); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.modifyPlayer(original, "Invalid", false, true); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.createPlayer(SEER_ROLE, ""); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.createPlayer(SEER_ROLE, "Invalid", false, true); } catch (const std::invalid_argument&) { ++rejected; }
    try { world.createPlayer(static_cast<RoleKind>(6)); } catch (const std::invalid_argument&) { ++rejected; }
    checks.expect(rejected == 5 && world.undoCount() == count
                  && description(world.findPlayer(original)) == originalState, "Invalid edits do not change history or player");
    checks.expect(!world.removePlayer(999) && world.duplicatePlayer(999) == 0
                  && !world.modifyPlayer(999, "Missing", true, false)
                  && world.undoCount() == count, "Missing IDs do not record commands");
    world.undo();
    checks.expect(world.playerCount() == 1, "Failed actions do not hide previous successful edit");
}

void testSnapshotsAndPositions(Checks& checks) {
    for (int role = 0; role < 6; ++role) {
        std::ostringstream output;
        World world(output);
        const int first = world.createPlayer(VILLAGER_ROLE, "First");
        const int subject = world.createPlayer(static_cast<RoleKind>(role), "Subject");
        const int last = world.createPlayer(WEREWOLF_ROLE, "Last");
        Player* player = world.findPlayer(subject);
        player->act(world);
        if (role == HUNTER_ROLE) {
            player->setAlive(false);
            player->onDeath(world);
        }
        world.processEvents();
        player->setAlive(true);
        player->setProtected(true);
        world.clearUndoHistory();
        const std::string saved = description(player);
        world.modifyPlayer(subject, "Changed", false, false);
        checks.expect(world.undo() && description(world.findPlayer(subject)) == saved,
                      "Modify undo restores exact role data, identity, and flags for every role");
        checks.expect(world.playerAt(1)->getId() == subject, "Modify undo retains middle position");
        world.removePlayer(subject);
        world.removePlayer(first);
        world.removePlayer(last);
        checks.expect(world.playerCount() == 0 && world.pendingEventCount() == 0,
                      "Administrative deletes trigger no death reactions");
        checks.expect(world.undo() && world.playerAt(0)->getId() == last, "Restore from empty list");
        checks.expect(world.undo() && world.playerAt(0)->getId() == first, "Restore first position");
        checks.expect(world.undo() && world.playerAt(1)->getId() == subject
                      && world.playerAt(2)->getId() == last
                      && description(world.findPlayer(subject)) == saved,
                      "Delete undo restores middle position and complete dynamic state");
        const int copy = world.duplicatePlayer(subject);
        world.removePlayer(subject);
        world.undo();
        checks.expect(world.findPlayer(copy)->getRole() == static_cast<RoleKind>(role),
                      "Copy retains type while original is restored");
        world.undo();
        checks.expect(world.findPlayer(copy) == nullptr && description(world.findPlayer(subject)) == saved,
                      "Snapshot ownership independent of duplicate cleanup");
        world.modifyPlayer(subject, "Pending snapshot", false, false);
        world.removePlayer(subject); // Destructor releases both kinds of saved clone.
    }
}

// Undo and history cleanup must not invalidate a running role/event callback.
class UndoProbe : public Event {
private:
    Checks& checks;
public:
    explicit UndoProbe(Checks& checks) : Event(1, 1), checks(checks) { }
    void describe(std::ostream& output) const override { output << "Undo probe\n"; }
    void apply(World& world) override {
        int blocked = 0;
        try { world.undo(); } catch (const std::logic_error&) { ++blocked; }
        try { world.clearUndoHistory(); } catch (const std::logic_error&) { ++blocked; }
        checks.expect(blocked == 2 && world.undoCount() == 0, "Undo blocked during gameplay with history already cleared");
    }
};

void testGameplayBoundary(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    checks.expect(world.runTurns(0) == 0 && world.undoCount() == 2, "Zero turns preserve edit history");
    bool rejected = false;
    try { world.runTurns(-1); } catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected && world.undoCount() == 2, "Rejected turn request preserves history");
    world.enqueueEvent(new UndoProbe(checks));
    world.runTurn();
    checks.expect(world.undoCount() == 0 && !world.undo(), "Starting turn clears all prior commands");
    world.modifyPlayer(1, "After turn", true, true);
    world.undo();
    checks.expect(world.findPlayer(1)->getName() == "Villager" && !world.findPlayer(1)->isProtected(),
                  "Post-turn edits undo to the post-turn state");
    world.createPlayer(HUNTER_ROLE);
    world.processEvents();
    checks.expect(world.undoCount() == 1, "Empty event processing preserves history");
    world.enqueueEvent(new UndoProbe(checks));
    world.processEvents();
    checks.expect(world.undoCount() == 0, "Nonempty direct event processing invalidates edit history");

    World empty(output);
    int dead = empty.createPlayer(HUNTER_ROLE, "Dead", false, false);
    empty.removePlayer(dead);
    checks.expect(!empty.runTurn() && empty.undoCount() == 2, "Refused empty turn preserves deletion undo");
    empty.undo();
    checks.expect(empty.findPlayer(dead) != nullptr && !empty.findPlayer(dead)->isAlive(), "Empty-world deletion remains recoverable");
    checks.expect(!empty.runTurn() && empty.undoCount() == 1, "All-dead world also preserves edits");

    World events(output);
    events.createPlayer(WEREWOLF_ROLE);
    int target = events.createPlayer(VILLAGER_ROLE);
    events.clearUndoHistory();
    events.enqueueEvent(new KillEvent(1, target));
    events.removePlayer(target);
    events.undo();
    checks.expect(events.pendingEventCount() == 1 && events.findPlayer(target)->isAlive(),
                  "Undo deletion restores original ID referenced by pending event");
    events.processEvents();
    checks.expect(!events.findPlayer(target)->isAlive() && events.undoCount() == 0,
                  "Restored ID receives committed event safely");
    int copy = events.duplicatePlayer(target);
    events.enqueueEvent(new RevealEvent(1, copy));
    events.undo();
    events.processEvents();
    checks.expect(events.findPlayer(copy) == nullptr && events.pendingEventCount() == 0,
                  "Undo duplication leaves safe missing event reference");
}

void testHistoryGrowthAndOutputFailure(Checks& checks) {
    std::ostringstream output;
    World world(output);
    for (int i = 0; i < 80; ++i) { world.createPlayer(VILLAGER_ROLE); }
    for (int i = 80; i > 0; --i) {
        checks.expect(world.undoCount() == i && world.undo() && world.playerCount() == i - 1,
                      "Growing history unwinds in LIFO order");
    }
    checks.expect(world.createPlayer(SEER_ROLE) == 81, "Large undo history never rewinds IDs");
    world.modifyPlayer(81, "Edited", true, false);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    try { output.setstate(std::ios::badbit); } catch (const std::ios_base::failure&) { }
    bool threw = false;
    try { world.undo(); } catch (const std::ios_base::failure&) { threw = true; }
    output.clear();
    checks.expect(threw && world.undoCount() == 2 && world.findPlayer(81)->getName() == "Edited",
                  "Failed output leaves undo command and player intact");
    checks.expect(world.undo() && world.findPlayer(81)->getName() == "Seer", "Undo succeeds after output recovery");
    world.modifyPlayer(81, "Saved", true, false);
    world.removePlayer(81);
    world.clearUndoHistory();
    checks.expect(world.undoCount() == 0 && world.findPlayer(81) == nullptr,
                  "Explicit history cleanup releases snapshots without restoring them");
}

int main() {
    Checks checks;
    std::cout << "Running M6 undo and snapshot tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testSequenceAndFailures(checks);
    testSnapshotsAndPositions(checks);
    testGameplayBoundary(checks);
    testHistoryGrowthAndOutputFailure(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding command or snapshot allocations");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.total << " checks, " << checks.failures << " failures.\n";
    return checks.failures == 0 ? 0 : 1;
}
