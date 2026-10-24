#include "Console.h"

#include <iostream>
#include <sstream>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Test counters live locally; simulated input exercises the real menu.
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

void runConsole(World& world, std::ostream& output, const std::string& commands) {
    std::istringstream input(commands);
    Console console(world, input, output);
    console.run();
}

void populate(World& world) {
    world.createPlayer(WITCH_ROLE, "Witch One");
    world.createPlayer(SEER_ROLE, "Seer Two");
    world.createPlayer(VILLAGER_ROLE, "Villager Three");
    world.createPlayer(HUNTER_ROLE, "Hunter Four");
    world.createPlayer(WEREWOLF_ROLE, "Wolf Five");
    world.createPlayer(ALPHA_WOLF_ROLE, "Alpha Six");
    world.clearUndoHistory();
}

std::string snapshot(const World& world) {
    std::ostringstream state;
    for (int i = 0; i < world.playerCount(); ++i) { world.playerAt(i)->describe(state); }
    return state.str();
}

bool contains(const std::ostringstream& output, const std::string& text) {
    return output.str().find(text) != std::string::npos;
}

int occurrences(const std::string& text, const std::string& part) {
    int count = 0;
    std::string::size_type index = text.find(part);
    while (index != std::string::npos) { ++count; index = text.find(part, index + part.size()); }
    return count;
}

void testCombinedMenu(Checks& checks) {
    std::ostringstream output;
    World world(output);
    populate(world);
    runConsole(world, output,
        "1\n13\n2\n11\n3\n5\n12\n3\n5\n1\n"
        "3\n4\n2\n6\n2\n2\n11\n2\n5\n12\n1\n1\n1\n12\n1\n5\n2\n"
        "6\n1\n1\n8\n7\n9\n8\nEdited Copy\n0\n0\n7\n8\n"
        "10\n13\n8\n10\n10\n10\n4\n3\n3\n2\n6\n5\n1\n13\n2\n13\n5\n0\n");
    checks.expect(world.playerCount() == 6 && world.findPlayer(7) == nullptr
                  && world.findPlayer(8) == nullptr, "Menu create/duplicate/modify/delete and repeated undo integrate");
    checks.expect(contains(output, "ID 8 | Edited Copy | Villager | Village | dead"),
                  "Details show exact state restored after deletion undo");
    checks.expect(contains(output, "This player cannot inspect.")
                  && contains(output, "This player cannot use potions."), "Unsupported capabilities return to menu");
    checks.expect(contains(output, "Event queued.") && contains(output, "Special action queued."),
                  "Manual and capability actions reachable");
    checks.expect(occurrences(output.str(), "1. RevealEvent source 2 -> target 6") == 2,
                  "Repeated queue viewing preserves first pending event");
    checks.expect(!world.findPlayer(5)->isAlive() && contains(output, "Player 5 died by poison"),
                  "Potion menu queues and resolves poison");
    checks.expect(contains(output, "[6:AlphaWolf]") && contains(output, "[5:Werewolf]"),
                  "Manual and special inspection results persist");
    checks.expect(contains(output, "ID 5 | Wolf Five | Werewolf | Wolves | dead"),
                  "Single-player display works for dead players");
    checks.expect(world.getCompletedTurns() == 2 && world.getCompletedRounds() == 1
                  && world.getPhase() == NIGHT && world.pendingEventCount() == 0
                  && world.undoCount() == 0, "One-turn and batch commands update status and clear history");
    checks.expect(contains(output, "Completed 1 turn(s).") && contains(output, "Goodbye."),
                  "Batch completion and explicit quit reported");
}

void testCreateAndModifyPrompts(Checks& checks) {
    std::ostringstream output;
    World world(output);
    std::ostringstream commands;
    for (int role = 1; role <= 6; ++role) { commands << "6\n" << role << "\n1\n"; }
    commands << "6\n99\n2\n9\n2\n\n \t\nAda Lovelace\n2\n1\n-1\n0\n"
             << "6\n4\n2\nDead Hunter\n0\n1\n0\n0\n"
             << "9\n7\n\nUpdated Ada\n0\n1\n1\n1\n0\n";
    runConsole(world, output, commands.str());
    for (int role = 0; role < 6; ++role) {
        const Player* player = world.findPlayer(role + 1);
        checks.expect(player != nullptr && player->getRole() == static_cast<RoleKind>(role)
                      && player->getName() == roleName(static_cast<RoleKind>(role))
                      && player->isAlive() && !player->isProtected(), "Default creation available for each role");
    }
    checks.expect(world.playerCount() == 8 && world.undoCount() == 9, "Invalid prompt retries do not create extra edits or IDs");
    checks.expect(world.findPlayer(7)->getName() == "Updated Ada" && world.findPlayer(7)->isProtected(),
                  "Names with spaces and validated modification supported");
    checks.expect(!world.findPlayer(8)->isAlive() && !world.findPlayer(8)->isProtected(),
                  "Dead/protected conflict reprompts before creation");
    checks.expect(contains(output, "A player name cannot be blank.")
                  && contains(output, "A dead player cannot be protected."), "Names and cross-field validation explained");
    world.undo();
    checks.expect(world.findPlayer(7)->getName() == "Ada Lovelace" && !world.findPlayer(7)->isProtected(),
                  "Only completed valid modification enters undo history");
}

void testNumericInputAndMissingPlayers(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(VILLAGER_ROLE);
    world.clearUndoHistory();
    const std::string before = snapshot(world);
    const char* invalid[] = {"", " \t", "abc", "14", "-1", "2147483648", "-2147483649",
        "999999999999999999999999999999999999", "1 2", "2.5", "1e3", "+", "--1", "0x10"};
    std::ostringstream commands;
    for (const char* token : invalid) { commands << token << '\n'; }
    commands << "13\n0\n-1\n2147483648\n2147483647\n8\n999\n9\n999\n7\n999\n"
             << "11\n999\n1\n12\n1\n1\n3\n1\n +0 \n";
    runConsole(world, output, commands.str());
    checks.expect(snapshot(world) == before && world.undoCount() == 0
                  && world.pendingEventCount() == 0, "Malformed and unknown IDs leave World unchanged");
    checks.expect(occurrences(output.str(), "Enter one whole number from 0 to 13.") == 14,
                  "Menu rejects every malformed and out-of-range integer");
    checks.expect(occurrences(output.str(), "Unknown player ID.") == 5,
                  "All ID-based operations handle missing players safely");
    checks.expect(contains(output, "Enter one whole number from 1 to 2.")
                  && contains(output, "This player cannot use potions."), "Potion choice reprompts before capability validation");
    checks.expect(contains(output, "Goodbye.") && !contains(output, "End of input."),
                  "Whitespace and signed zero accepted as a complete quit choice");
}

void testDetailsAndReadOnlyActions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    populate(world);
    world.modifyPlayer(4, "Dead Hunter", false, false);
    const std::string before = snapshot(world);
    runConsole(world, output, "13\n4\n2\n0\n");
    checks.expect(contains(output, "ID 4 | Dead Hunter | Hunter | Village | dead")
                  && !contains(output, "ID 1 |"), "Single-player details print only the selected player");
    checks.expect(snapshot(world) == before && world.undoCount() == 1
                  && world.getCompletedTurns() == 0, "Details and queue view preserve state, history, and phase");
}

void testPartialInputEOF(Checks& checks) {
    const char* partial[] = {"", "abc\n", "3\n", "3\n1\n", "3\n1\n1\n", "5\n",
        "5\n1001\n", "6\n", "6\n1\n", "6\n1\n2\n", "6\n1\n2\nPartial\n",
        "6\n1\n2\nPartial\n1\n", "6\n1\n2\nPartial\n0\n1\n", "7\n", "8\n",
        "9\n", "9\n1\n", "9\n1\nNew\n", "9\n1\nNew\n1\n", "11\n", "11\n2\n",
        "12\n", "12\n1\n", "12\n1\n3\n", "13\n", "13\n999\n"};
    for (const char* script : partial) {
        std::ostringstream output;
        World world(output);
        populate(world);
        const std::string before = snapshot(world);
        runConsole(world, output, script);
        checks.expect(snapshot(world) == before && world.undoCount() == 0
                      && world.pendingEventCount() == 0 && world.getCompletedTurns() == 0,
                      "EOF during incomplete command changes nothing");
        checks.expect(contains(output, "End of input. Goodbye."), "EOF always exits cleanly");
    }
    std::ostringstream output;
    World world(output);
    runConsole(world, output, "6\n1\n1");
    checks.expect(world.playerCount() == 1 && world.undoCount() == 1, "Complete last line commits without a final newline");
    runConsole(world, output, "9\n1\nFinal Name\n1\n0");
    checks.expect(world.findPlayer(1)->getName() == "Final Name" && world.undoCount() == 2,
                  "Completed modification retained when EOF follows final value");
}

void testTurnsAndEmptyWorld(Checks& checks) {
    std::ostringstream output;
    World world(output);
    runConsole(world, output, "1\n2\n10\n4\n5\n2\n0\n");
    checks.expect(contains(output, "The world has no players.") && contains(output, "No pending events.")
                  && contains(output, "Nothing to undo.") && contains(output, "Completed 0 turn(s)."),
                  "Empty world menu operations report results safely");
    checks.expect(world.getCompletedTurns() == 0, "Refused empty turns never advance phase");
    world.createPlayer(VILLAGER_ROLE);
    runConsole(world, output, "5\nabc\n\n0\n-1\n1001\n2147483647\n999999999999999999999\n2\n0\n");
    checks.expect(world.getCompletedTurns() == 2 && world.getCompletedRounds() == 1,
                  "Invalid turn counts reprompt before valid bounded batch");
    runConsole(world, output, "5\n1000\n0\n");
    checks.expect(world.getCompletedTurns() == 1002 && contains(output, "Completed 1000 turn(s)."),
                  "Maximum console batch accepted and terminates");
    checks.expect(world.undoCount() == 0, "Menu turn batches clear editing history");
}

void testEventAndSpecialValidation(Checks& checks) {
    std::ostringstream output;
    World world(output);
    populate(world);
    runConsole(world, output,
        "3\n9\n4\n3\n5\n3\n1\n1\n1\n11\n2\n2\n12\n1\n1\n2\n"
        "3\n4\n2\n999\n0\n");
    checks.expect(world.pendingEventCount() == 0 && world.undoCount() == 0,
                  "Invalid kind/source/self/target requests enqueue nothing");
    checks.expect(contains(output, "Enter one whole number from 1 to 5.")
                  && contains(output, "Not queued:"), "Event prompt and logical failures explained");
    runConsole(world, output, "4\n11\n2\n6\n12\n1\n3\n2\n0\n");
    checks.expect(world.getPhase() == DAY && world.pendingEventCount() == 0,
                  "Special capability actions respect phase through full menu");
}

void testActionExceptionRecovery(Checks& checks) {
    std::ostringstream worldOutput, consoleOutput;
    World world(worldOutput);
    world.createPlayer(VILLAGER_ROLE);
    worldOutput.exceptions(std::ios::badbit | std::ios::failbit);
    try { worldOutput.setstate(std::ios::badbit); } catch (const std::ios_base::failure&) { }
    runConsole(world, consoleOutput, "4\n13\n1\n6\n1\n1\n0\n");
    checks.expect(contains(consoleOutput, "Action failed:") && contains(consoleOutput, "ID 1 | Villager"),
                  "Action exceptions return to menu and allow further commands");
    checks.expect(world.playerCount() == 2 && world.getCompletedTurns() == 0,
                  "Failed turn guard resets so subsequent creation succeeds");
    worldOutput.clear();
}

int main() {
    Checks checks;
    std::cout << "Running M8 console and input tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testCombinedMenu(checks);
    testCreateAndModifyPrompts(checks);
    testNumericInputAndMissingPlayers(checks);
    testDetailsAndReadOnlyActions(checks);
    testPartialInputEOF(checks);
    testTurnsAndEmptyWorld(checks);
    testEventAndSpecialValidation(checks);
    testActionExceptionRecovery(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding allocations after console sessions");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.total << " checks, " << checks.failures << " failures.\n";
    return checks.failures == 0 ? 0 : 1;
}
