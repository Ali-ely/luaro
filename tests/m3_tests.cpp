#include "World.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Counts test outcomes locally rather than using global mutable counters.
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

void addMixedRoles(World& world) {
    world.createPlayer(ALPHA_WOLF_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.createPlayer(HUNTER_ROLE);
    world.createPlayer(WITCH_ROLE);
    world.createPlayer(SEER_ROLE);
}

int occurrences(const std::string& text, const std::string& fragment) {
    int count = 0;
    std::string::size_type position = text.find(fragment);
    while (position != std::string::npos) {
        ++count;
        position = text.find(fragment, position + fragment.size());
    }
    return count;
}

std::string actionLine(const std::string& text, int id) {
    std::string marker = "  [" + std::to_string(id) + "]";
    std::string::size_type start = text.find(marker);
    if (start == std::string::npos) {
        return "";
    }
    std::string::size_type end = text.find('\n', start);
    return text.substr(start, end - start);
}

void testEmptyWorldAndValidation(Checks& checks) {
    std::ostringstream output;
    World world(output);
    checks.expect(world.playerCount() == 0 && world.livingPlayerCount() == 0,
                  "New world has no players");
    checks.expect(world.getPhase() == NIGHT && world.getCompletedTurns() == 0
                  && world.getCompletedRounds() == 0, "World starts before its first night");
    checks.expect(!world.runTurn() && world.runTurns(5) == 0,
                  "Empty world cannot advance a turn or a batch");
    checks.expect(world.getPhase() == NIGHT && world.getCompletedTurns() == 0,
                  "Rejected empty turns preserve phase and counters");
    std::string previous = output.str();
    checks.expect(world.runTurns(0) == 0 && output.str() == previous,
                  "Zero requested turns is a silent no-op");
    world.describePlayers();
    checks.expect(output.str().find("The world has no players.") != std::string::npos,
                  "Empty world can be described");
    checks.expect(world.findPlayer(1) == nullptr && world.findPlayer(-1) == nullptr
                  && !world.removePlayer(0), "Missing IDs are handled safely");

    bool rejected = false;
    try { world.runTurns(-1); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected && world.getCompletedTurns() == 0, "Negative turn count rejected");
    rejected = false;
    try { world.playerAt(0); }
    catch (const std::out_of_range&) { rejected = true; }
    checks.expect(rejected, "Empty world rejects indexed access");
    rejected = false;
    try { world.playerAt(-1); }
    catch (const std::out_of_range&) { rejected = true; }
    checks.expect(rejected, "Negative player index rejected");
    rejected = false;
    try { world.createPlayer(VILLAGER_ROLE, " \t"); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected && world.playerCount() == 0, "Bad name leaves world unchanged");
    // Six is representable by RoleKind but is not one of the six valid roles.
    rejected = false;
    try { world.createPlayer(static_cast<RoleKind>(6), "Invalid"); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected && world.playerCount() == 0, "Unknown named role rejected");
    rejected = false;
    try { world.createPlayer(static_cast<RoleKind>(6)); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected && world.playerCount() == 0, "Unknown default role rejected");
    checks.expect(world.createPlayer(VILLAGER_ROLE, "Alice") == 1,
                  "Failed creation does not consume an ID");
    rejected = false;
    try { world.playerAt(world.playerCount()); }
    catch (const std::out_of_range&) { rejected = true; }
    checks.expect(rejected, "Past-end player index rejected");
    world.runTurn();
    rejected = false;
    try { world.runTurns(std::numeric_limits<int>::max()); }
    catch (const std::overflow_error&) { rejected = true; }
    checks.expect(rejected && world.getCompletedTurns() == 1 && world.getPhase() == DAY,
                  "Counter overflow is rejected before a batch starts");
}

void testCreationLookupAndRemoval(Checks& checks) {
    std::ostringstream output;
    World world(output);
    addMixedRoles(world);
    const RoleKind expected[] = {ALPHA_WOLF_ROLE, VILLAGER_ROLE, WEREWOLF_ROLE,
                                 HUNTER_ROLE, WITCH_ROLE, SEER_ROLE};
    const World& view = world;
    checks.expect(world.playerCount() == 6 && world.livingPlayerCount() == 6,
                  "World creates all six concrete roles");
    for (int i = 0; i < 6; ++i) {
        const Player* player = view.playerAt(i);
        checks.expect(player->getId() == i + 1 && player->getRole() == expected[i],
                      "IDs are unique and player list preserves creation order");
        checks.expect(player->getName() == roleName(expected[i])
                      && view.findPlayer(i + 1) == player && world.findPlayer(i + 1) == player,
                      "Default names and both lookup overloads agree");
    }
    checks.expect(view.findPlayer(999) == nullptr, "Const lookup handles an unknown ID");
    checks.expect(world.removePlayer(4) && world.findPlayer(4) == nullptr,
                  "Removing the middle player removes its ID");
    checks.expect(world.playerCount() == 5 && world.playerAt(3)->getId() == 5,
                  "Removal preserves the relative order of survivors");
    checks.expect(!world.removePlayer(4) && world.playerCount() == 5,
                  "Repeated removal is harmless");
    checks.expect(world.createPlayer(HUNTER_ROLE, "Replacement") == 7,
                  "A new player does not reuse a removed ID");
    checks.expect(world.removePlayer(1) && world.removePlayer(7),
                  "Removing first and last players is safe");
    checks.expect(world.playerAt(0)->getId() == 2 && world.playerAt(3)->getId() == 6,
                  "First and last removals preserve remaining positions");
    world.findPlayer(2)->setAlive(false);
    checks.expect(world.playerCount() == 4 && world.livingPlayerCount() == 3,
                  "Death keeps the object but reduces the living count");
    world.describePlayers();
    checks.expect(output.str().find("ID 2 | Villager | Villager | Village | dead")
                  != std::string::npos, "Dead player remains inspectable");
    checks.expect(output.str().find("ID 4 |") == std::string::npos,
                  "Administrative removal does not leave a listed player");
}

void testRoleActionsAndTurnOrder(Checks& checks) {
    std::ostringstream output;
    World world(output);
    addMixedRoles(world);
    checks.expect(world.runTurn(), "One night runs successfully");
    std::string night = output.str();
    checks.expect(world.getPhase() == DAY && world.getCompletedTurns() == 1
                  && world.getCompletedRounds() == 0, "Night advances to Day but not a full round");
    const int nightOrder[] = {5, 6, 1, 3, 2, 4};
    std::string::size_type previousPosition = 0;
    for (int i = 0; i < 6; ++i) {
        std::string marker = "  [" + std::to_string(nightOrder[i]) + "]";
        std::string::size_type position = night.find(marker);
        checks.expect(position != std::string::npos && (i == 0 || position > previousPosition),
                      "Night follows virtual priority and stable order within each group");
        previousPosition = position;
        checks.expect(occurrences(night, marker) == 1, "Each living player acts once at night");
        checks.expect(world.playerAt(i)->getId() == i + 1, "Night scheduling does not reorder players");
    }
    checks.expect(night.find("Witch checks night potions") != std::string::npos
                  && night.find("Seer prepares a secret role inspection") != std::string::npos
                  && night.find("Hunter stands watch") != std::string::npos,
                  "Special village roles have distinct night actions");
    checks.expect(night.find("Villager observes") != std::string::npos
                  && night.find("Werewolf prepares to attack") != std::string::npos
                  && night.find("AlphaWolf coordinates") != std::string::npos,
                  "Ordinary villager and wolf roles have distinct night actions");

    output.str("");
    checks.expect(world.runTurn(), "One day runs successfully");
    std::string day = output.str();
    checks.expect(world.getPhase() == NIGHT && world.getCompletedTurns() == 2
                  && world.getCompletedRounds() == 1, "Night plus Day completes one round");
    previousPosition = 0;
    for (int id = 1; id <= 6; ++id) {
        std::string marker = "  [" + std::to_string(id) + "]";
        std::string::size_type position = day.find(marker);
        checks.expect(position != std::string::npos && (id == 1 || position > previousPosition),
                      "Day follows the original player order");
        previousPosition = position;
        checks.expect(occurrences(day, marker) == 1, "Each living player acts once during Day");
        checks.expect(!actionLine(night, id).empty() && !actionLine(day, id).empty()
                      && actionLine(night, id) != actionLine(day, id),
                      "Every concrete role changes its action with the phase");
    }
    checks.expect(day.find("AlphaWolf prepares a double-weight day vote") != std::string::npos,
                  "AlphaWolf reports its distinct day behavior");
    checks.expect(world.livingPlayerCount() == 6,
                  "M3 action messages do not apply M4 event effects");

    output.str("");
    checks.expect(world.runTurns(5) == 5 && world.getCompletedTurns() == 7
                  && world.getCompletedRounds() == 3 && world.getPhase() == DAY,
                  "Running several turns updates phase and round bookkeeping");
    checks.expect(occurrences(output.str(), " - Night\n") == 3
                  && occurrences(output.str(), " - Day\n") == 2,
                  "A multi-turn batch alternates Night and Day");
}

void testDeadAndRemovedPlayers(Checks& checks) {
    std::ostringstream output;
    World world(output);
    addMixedRoles(world);
    world.findPlayer(5)->setAlive(false);
    world.findPlayer(2)->setAlive(false);
    world.removePlayer(3);
    checks.expect(world.runTurns(2) == 2, "Turns run with dead and removed players");
    checks.expect(occurrences(output.str(), "  [5]") == 0
                  && occurrences(output.str(), "  [2]") == 0
                  && occurrences(output.str(), "  [3]") == 0,
                  "Dead and removed players never act in either phase");
    checks.expect(occurrences(output.str(), "  [1]") == 2
                  && occurrences(output.str(), "  [4]") == 2
                  && occurrences(output.str(), "  [6]") == 2,
                  "Surviving players each act in both phases");
    for (int id = 1; id <= 6; ++id) {
        Player* player = world.findPlayer(id);
        if (player != nullptr) {
            player->setAlive(false);
        }
    }
    checks.expect(!world.runTurn() && world.runTurns(100) == 0,
                  "An all-dead world stops a batch without looping through empty turns");
    checks.expect(world.getCompletedTurns() == 2 && world.getPhase() == NIGHT,
                  "All-dead attempts do not advance the phase");
    checks.expect(world.createPlayer(VILLAGER_ROLE, "New arrival") == 7 && world.runTurn(),
                  "A world can run again after adding a living player");
}

void testOutputFailureRecovery(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(VILLAGER_ROLE);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    try { output.setstate(std::ios::badbit); }
    catch (const std::ios_base::failure&) { }
    bool failed = false;
    try { world.runTurn(); }
    catch (const std::ios_base::failure&) { failed = true; }
    checks.expect(failed && world.getCompletedTurns() == 0 && world.getPhase() == NIGHT,
                  "A failed turn does not claim completion");
    output.clear();
    checks.expect(world.createPlayer(SEER_ROLE) == 2 && world.removePlayer(1),
                  "Turn guard is reset when output throws");
    checks.expect(world.runTurn() && world.getCompletedTurns() == 1,
                  "World runs again after output recovers");
}

void testOwnershipAndVectorGrowth(Checks& checks) {
    const RoleKind roles[] = {VILLAGER_ROLE, SEER_ROLE, WITCH_ROLE, HUNTER_ROLE,
                              WEREWOLF_ROLE, ALPHA_WOLF_ROLE};
    for (int repetition = 0; repetition < 10; ++repetition) {
        std::ostringstream output;
        World world(output);
        for (int i = 0; i < 30; ++i) {
            checks.expect(world.createPlayer(roles[i % 6], std::string(200, 'a')) == i + 1,
                          "Vector growth preserves unique ID allocation");
        }
        for (int id = 2; id <= 30; id += 2) {
            checks.expect(world.removePlayer(id), "Repeated middle removals release owned players");
        }
        checks.expect(world.playerCount() == 15, "World retains exactly the surviving players");
        for (int i = 0; i < 15; ++i) {
            checks.expect(world.playerAt(i)->getId() == 2 * i + 1,
                          "Vector removals preserve surviving IDs and order");
        }
        checks.expect(world.createPlayer(WITCH_ROLE) == 31, "ID allocation continues after many removals");
        checks.expect(world.runTurns(4) == 4, "Remaining players still run after vector growth/removal");
        // Destruction must release every remaining player and its long name.
    }
}

void runTests(Checks& checks) {
    testEmptyWorldAndValidation(checks);
    testCreationLookupAndRemoval(checks);
    testRoleActionsAndTurnOrder(checks);
    testDeadAndRemovedPlayers(checks);
    testOutputFailureRecovery(checks);
    testOwnershipAndVectorGrowth(checks);
}

int main() {
    Checks checks;
    std::cout << "Running M3 World and turn-loop tests...\n";
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
    checks.expect(!leaked, "No outstanding allocations after World ownership tests");
    if (leaked) {
        _CrtMemDumpStatistics(&difference);
    }
#endif

    std::cout << checks.getTotal() << " checks, " << checks.getFailures() << " failures.\n";
    return checks.getFailures() == 0 ? 0 : 1;
}
