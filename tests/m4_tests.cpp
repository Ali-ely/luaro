#include "World.h"
#include "Event.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Keeps all test counters local to this executable.
class Checks {
    int total;
    int failures;
public:
    Checks() : total(0), failures(0) { }
    void expect(bool condition, const char* message) {
        ++total;
        if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
    }
    int getTotal() const { return total; }
    int getFailures() const { return failures; }
};

// Records application/destruction and can append an event or deliberately fail.
// References borrow local test data that outlives the test World.
class ProbeEvent : public Event {
    std::vector<int>& applied;
    int& destroyed;
    int marker;
    int appendMarker;
    bool fail;
public:
    ProbeEvent(std::vector<int>& applied, int& destroyed, int marker,
               int appendMarker = 0, bool fail = false)
        : Event(1, 2), applied(applied), destroyed(destroyed), marker(marker),
          appendMarker(appendMarker), fail(fail) { }
    ~ProbeEvent() override { ++destroyed; }
    void apply(World& world) override {
        applied.push_back(marker);
        if (appendMarker != 0) {
            world.enqueueEvent(new ProbeEvent(applied, destroyed, appendMarker));
        }
        if (fail) { throw std::runtime_error("Deliberate event failure."); }
    }
    void describe(std::ostream& output) const override { output << "Probe " << marker << '\n'; }
};

std::string describe(const Player* player) {
    std::ostringstream output;
    player->describe(output);
    return output.str();
}

void testQueueOwnershipAndOrder(Checks& checks) {
    std::vector<int> applied;
    int destroyed = 0;
    std::ostringstream output;
    {
        World world(output);
        world.enqueueEvent(new ProbeEvent(applied, destroyed, 1, 3));
        world.enqueueEvent(new ProbeEvent(applied, destroyed, 2));
        world.viewPendingEvents();
        std::string firstView = output.str();
        output.str("");
        world.viewPendingEvents();
        checks.expect(output.str() == firstView && world.pendingEventCount() == 2
                      && applied.empty() && destroyed == 0, "Viewing neither applies nor removes events");
        world.processEvents();
        checks.expect(applied.size() == 3 && applied[0] == 1 && applied[1] == 2 && applied[2] == 3,
                      "Newly appended reactions follow already-pending events");
        checks.expect(destroyed == 3 && world.pendingEventCount() == 0,
                      "Each processed event is deleted exactly once");
        world.processEvents();
        checks.expect(destroyed == 3, "Processing an empty queue is harmless");
        world.enqueueEvent(new ProbeEvent(applied, destroyed, 4));
        world.enqueueEvent(new ProbeEvent(applied, destroyed, 5));
    }
    checks.expect(destroyed == 5 && applied.size() == 3, "World deletes unapplied events at shutdown");
    World failureWorld(output);
    failureWorld.enqueueEvent(new ProbeEvent(applied, destroyed, 6, 0, true));
    failureWorld.enqueueEvent(new ProbeEvent(applied, destroyed, 7));
    bool failed = false;
    try { failureWorld.processEvents(); }
    catch (const std::runtime_error&) { failed = true; }
    checks.expect(failed && destroyed == 7 && failureWorld.pendingEventCount() == 0
                  && applied.back() == 6, "Failure deletes current and pending events without applying the tail");
    failureWorld.enqueueEvent(new ProbeEvent(applied, destroyed, 8));
    failureWorld.processEvents();
    checks.expect(applied.back() == 8 && destroyed == 8, "Resolution guard resets after an exception");
}

void testHealingAndPotions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(WITCH_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.enqueueEvent(new HealEvent(1, 2));
    world.enqueueEvent(new KillEvent(3, 2));
    world.enqueueEvent(new KillEvent(1, 2, true));
    world.processEvents();
    checks.expect(world.findPlayer(2)->isAlive() && world.findPlayer(2)->isProtected(),
                  "Healing before normal and poison attacks protects the target");
    checks.expect(describe(world.findPlayer(1)).find("potions used: 2 [2:heal] [2:poison]") != std::string::npos,
                  "Valid potion effects consume and record both potions, including blocked poison");
    world.enqueueEvent(new HealEvent(1, 1));
    world.enqueueEvent(new KillEvent(1, 3, true));
    world.processEvents();
    checks.expect(!world.findPlayer(1)->isProtected() && world.findPlayer(3)->isAlive(),
                  "Consumed potions cannot be used again");
    world.enqueueEvent(new HealEvent(2, 1));
    world.enqueueEvent(new KillEvent(2, 3, true));
    world.processEvents();
    checks.expect(!world.findPlayer(1)->isProtected() && world.findPlayer(3)->isAlive(),
                  "Non-Witches cannot consume potions through event delivery");

    World reversed(output);
    reversed.createPlayer(WITCH_ROLE);
    reversed.createPlayer(VILLAGER_ROLE);
    reversed.createPlayer(WEREWOLF_ROLE);
    reversed.enqueueEvent(new KillEvent(3, 2));
    reversed.enqueueEvent(new HealEvent(1, 2));
    reversed.processEvents();
    checks.expect(!reversed.findPlayer(2)->isAlive() && !reversed.findPlayer(2)->isProtected(),
                  "Healing after a kill does not resurrect the target");
    checks.expect(describe(reversed.findPlayer(1)).find("potions used: 0") != std::string::npos,
                  "An invalid dead target does not consume a potion");
}

void testNormalNightAndExpiry(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(WITCH_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.createPlayer(ALPHA_WOLF_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    checks.expect(world.runTurn() && world.livingPlayerCount() == 5,
                  "Witch self-healing resolves before both wolves' attacks");
    checks.expect(!world.findPlayer(1)->isProtected() && world.pendingEventCount() == 0,
                  "Night clears protection only after all queued effects resolve");
    std::string log = output.str();
    checks.expect(log.find("Resolve: HealEvent") < log.find("Resolve: KillEvent"),
                  "Normal night preserves heal-before-kill creation order");
    checks.expect(describe(world.findPlayer(3)).find("past targets: 1 [1]") != std::string::npos,
                  "Werewolf records its chosen victim");
    checks.expect(describe(world.findPlayer(4)).find("pack members: 1 [3]") != std::string::npos,
                  "AlphaWolf records the other living pack members");
    world.enqueueEvent(new KillEvent(3, 1));
    world.processEvents();
    checks.expect(!world.findPlayer(1)->isAlive(), "Expired protection cannot block a later attack");

    World unprotected(output);
    unprotected.createPlayer(VILLAGER_ROLE);
    unprotected.createPlayer(VILLAGER_ROLE);
    unprotected.createPlayer(WEREWOLF_ROLE);
    unprotected.createPlayer(ALPHA_WOLF_ROLE);
    unprotected.runTurn();
    checks.expect(!unprotected.findPlayer(1)->isAlive() && unprotected.findPlayer(2)->isAlive()
                  && unprotected.livingPlayerCount() == 3,
                  "Multiple wolves share one victim instead of choosing additional victims");
    unprotected.removePlayer(3);
    unprotected.findPlayer(4)->act(unprotected);
    unprotected.processEvents();
}

void testRevealsAndKnowledge(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(SEER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.findPlayer(1)->act(world);
    checks.expect(world.pendingEventCount() == 1
                  && describe(world.findPlayer(1)).find("inspections: 0") != std::string::npos,
                  "An inspection remains pending until event resolution");
    world.processEvents();
    checks.expect(describe(world.findPlayer(1)).find("inspections: 1 [2:Villager]") != std::string::npos,
                  "Reveal records the exact role in the Seer's owned array");
    world.findPlayer(1)->act(world);
    world.processEvents();
    checks.expect(describe(world.findPlayer(1)).find("[3:Villager]") != std::string::npos,
                  "Seer next selects the lowest uninspected living target");
    world.findPlayer(1)->act(world);
    checks.expect(world.pendingEventCount() == 0, "Seer safely skips when all legal targets are known");

    World voting(output);
    voting.createPlayer(VILLAGER_ROLE);
    voting.createPlayer(SEER_ROLE);
    voting.createPlayer(VILLAGER_ROLE);
    voting.createPlayer(WEREWOLF_ROLE);
    voting.enqueueEvent(new RevealEvent(2, 4));
    voting.runTurn();
    voting.runTurn();
    checks.expect(voting.voteTotal(4) == 1 && voting.voteTotal(2) == 2,
                  "Seer votes for a known wolf while other villagers use their own knowledge");
}

void testVoting(Checks& checks) {
    std::ostringstream output;
    World tied(output);
    tied.createPlayer(VILLAGER_ROLE);
    tied.createPlayer(VILLAGER_ROLE);
    tied.runTurn();

    tied.modifyPlayer(1, "Villager", true, false);
    tied.runTurn();
    checks.expect(tied.ballotCount() == 2 && tied.voteTotal(1) == 1 && tied.voteTotal(2) == 1
                  && tied.livingPlayerCount() == 2, "Equal highest totals cause no elimination");
    World alone(output);
    alone.createPlayer(VILLAGER_ROLE);
    alone.runTurn();
    alone.modifyPlayer(1, "Villager", true, false);
    alone.runTurn();
    checks.expect(alone.ballotCount() == 0 && alone.livingPlayerCount() == 1,
                  "No eligible target produces no self-vote or elimination");

    World weighted(output);
    weighted.createPlayer(ALPHA_WOLF_ROLE);
    weighted.createPlayer(VILLAGER_ROLE);
    weighted.createPlayer(VILLAGER_ROLE);
    weighted.createPlayer(WITCH_ROLE);
    weighted.enqueueEvent(new HealEvent(4, 2));
    weighted.runTurn();
    checks.expect(weighted.livingPlayerCount() == 4, "Manual early healing can preserve the pack victim");
    weighted.addManualEvent(VOTE_EVENT, 1, 2);
    weighted.addManualEvent(VOTE_EVENT, 2, 3);
    weighted.addManualEvent(VOTE_EVENT, 3, 2);
    weighted.addManualEvent(VOTE_EVENT, 4, 3);
    weighted.runTurn();
    checks.expect(weighted.ballotCount() == 4 && weighted.voteTotal(2) == 3 && weighted.voteTotal(3) == 2,
                  "First manual ballots override duplicates and AlphaWolf contributes weight two");
    checks.expect(!weighted.findPlayer(2)->isAlive() && weighted.findPlayer(3)->isAlive(),
                  "Double weight breaks what would otherwise be a tied day tally");
    weighted.runTurn();
    checks.expect(weighted.ballotCount() == 0, "Old ballots are cleared before the next night");
}

void testHunterReactions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(HUNTER_ROLE);
    world.createPlayer(HUNTER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.enqueueEvent(new KillEvent(4, 1));
    world.enqueueEvent(new KillEvent(4, 1));
    world.processEvents();
    checks.expect(!world.findPlayer(1)->isAlive() && !world.findPlayer(2)->isAlive()
                  && !world.findPlayer(3)->isAlive() && world.findPlayer(4)->isAlive(),
                  "Hunter death reactions chain through the event queue");
    checks.expect(describe(world.findPlayer(1)).find("past shots: 1 [2]") != std::string::npos
                  && describe(world.findPlayer(2)).find("past shots: 1 [3]") != std::string::npos,
                  "Repeated attacks do not repeat a Hunter's death reaction");
    checks.expect(world.pendingEventCount() == 0, "All Hunter reactions finish before resolution returns");
    std::string log = output.str();
    std::string::size_type firstKill = log.find("Resolve: KillEvent");
    std::string::size_type secondKill = log.find("Resolve: KillEvent", firstKill + 1);
    checks.expect(secondKill < log.find("Resolve: ShootEvent"),
                  "Hunter reaction is appended behind already-created events");

    World day(output);
    day.createPlayer(HUNTER_ROLE);
    day.createPlayer(VILLAGER_ROLE);
    day.createPlayer(VILLAGER_ROLE);
    day.runTurn();
    day.modifyPlayer(1, "Hunter", true, false);
    day.runTurn();
    checks.expect(!day.findPlayer(1)->isAlive() && !day.findPlayer(2)->isAlive()
                  && day.findPlayer(3)->isAlive() && day.pendingEventCount() == 0,
                  "Day tally resolves the eliminated Hunter's shot before changing phase");

    World protectedTarget(output);
    protectedTarget.createPlayer(HUNTER_ROLE);
    protectedTarget.createPlayer(VILLAGER_ROLE);
    protectedTarget.createPlayer(WEREWOLF_ROLE);
    protectedTarget.findPlayer(2)->setProtected(true);
    protectedTarget.enqueueEvent(new KillEvent(3, 1));
    protectedTarget.processEvents();
    checks.expect(protectedTarget.findPlayer(2)->isAlive(), "Hunter shots respect active protection");
    World administrative(output);
    administrative.createPlayer(HUNTER_ROLE);
    administrative.createPlayer(VILLAGER_ROLE);
    administrative.removePlayer(1);
    checks.expect(administrative.pendingEventCount() == 0 && administrative.findPlayer(2)->isAlive(),
                  "Administrative deletion does not trigger a shot");
}

void testMissingPlayersAndCommittedActions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(WITCH_ROLE);
    world.createPlayer(SEER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(HUNTER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.enqueueEvent(new HealEvent(1, 3));
    world.enqueueEvent(new RevealEvent(2, 3));
    world.enqueueEvent(new KillEvent(5, 3));
    world.enqueueEvent(new ShootEvent(4, 3));
    world.enqueueEvent(new VoteEvent(5, 3));
    world.removePlayer(3);
    int replacement = world.createPlayer(VILLAGER_ROLE);
    world.processEvents();
    checks.expect(replacement == 6 && world.findPlayer(6)->isAlive() && !world.findPlayer(6)->isProtected()
                  && world.pendingEventCount() == 0, "Events about removed IDs do not hit replacement players");
    checks.expect(describe(world.findPlayer(1)).find("potions used: 0") != std::string::npos
                  && describe(world.findPlayer(2)).find("inspections: 0") != std::string::npos,
                  "Missing targets neither consume potions nor create inspection records");
    world.enqueueEvent(new HealEvent(1, 6));
    world.enqueueEvent(new RevealEvent(2, 6));
    world.enqueueEvent(new KillEvent(5, 6));
    world.enqueueEvent(new ShootEvent(4, 6));
    world.removePlayer(1); world.removePlayer(2); world.removePlayer(4); world.removePlayer(5);
    world.processEvents();
    checks.expect(world.findPlayer(6)->isAlive() && !world.findPlayer(6)->isProtected(),
                  "Events with deleted sources are ignored safely");

    World committed(output);
    committed.createPlayer(WEREWOLF_ROLE);
    committed.createPlayer(VILLAGER_ROLE);
    committed.createPlayer(VILLAGER_ROLE);
    committed.enqueueEvent(new KillEvent(3, 1));
    committed.enqueueEvent(new KillEvent(1, 2));
    committed.processEvents();
    checks.expect(!committed.findPlayer(1)->isAlive() && !committed.findPlayer(2)->isAlive(),
                  "A previously queued attack remains committed when its source dies");
}

void testManualValidationAndStress(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(WITCH_ROLE);
    world.createPlayer(SEER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    checks.expect(!world.addManualEvent(KILL_EVENT, 999, 3)
                  && !world.addManualEvent(KILL_EVENT, 1, 999), "Manual event rejects unknown IDs");
    checks.expect(!world.addManualEvent(VOTE_EVENT, 1, 3), "Manual vote rejects Night phase");
    checks.expect(!world.addManualEvent(HEAL_EVENT, 3, 1)
                  && !world.addManualEvent(REVEAL_EVENT, 3, 1), "Manual capabilities validate the source role");
    checks.expect(!world.addManualEvent(KILL_EVENT, 1, 1)
                  && !world.addManualEvent(POISON_EVENT, 1, 1), "Manual non-healing actions reject self-targets");
    checks.expect(!world.addManualEvent(static_cast<EventKind>(7), 1, 3), "Unknown event kind rejected");
    checks.expect(world.pendingEventCount() == 0, "Invalid manual events leave queue unchanged");
    checks.expect(world.addManualEvent(HEAL_EVENT, 1, 1)
                  && world.addManualEvent(REVEAL_EVENT, 2, 3), "Valid manual effects enter the queue");
    world.runTurn();
    checks.expect(!world.addManualEvent(KILL_EVENT, 1, 3)
                  && !world.addManualEvent(HEAL_EVENT, 1, 3), "Manual night effects reject Day phase");
    world.findPlayer(3)->setAlive(false);
    checks.expect(!world.addManualEvent(VOTE_EVENT, 3, 1)
                  && !world.addManualEvent(VOTE_EVENT, 1, 3), "Manual events reject dead participants");
    bool rejected = false;
    try { world.enqueueEvent(nullptr); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected, "Null event rejected");
    rejected = false;
    try { KillEvent invalid(0, 3); }
    catch (const std::invalid_argument&) { rejected = true; }
    checks.expect(rejected, "Event constructors reject nonpositive IDs");

    const RoleKind roles[] = {WITCH_ROLE, SEER_ROLE, HUNTER_ROLE, VILLAGER_ROLE,
                              WEREWOLF_ROLE, ALPHA_WOLF_ROLE};
    for (int repetition = 0; repetition < 20; ++repetition) {
        std::ostringstream trace;
        World simulation(trace);
        for (int i = 0; i < 12; ++i) { simulation.createPlayer(roles[i % 6]); }
        checks.expect(simulation.runTurns(4) > 0 && simulation.pendingEventCount() == 0,
                      "Mixed-role turns drain events and death reactions");
        simulation.modifyPlayer(4, "Fixture target", true, false);
        simulation.enqueueEvent(new KillEvent(5, 4));
        simulation.removePlayer(4);
        simulation.processEvents();
        simulation.enqueueEvent(new RevealEvent(2, 5));
    }
}

int main() {
    Checks checks;
    std::cout << "Running M4 event and role-effect tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testQueueOwnershipAndOrder(checks);
    testHealingAndPotions(checks);
    testNormalNightAndExpiry(checks);
    testRevealsAndKnowledge(checks);
    testVoting(checks);
    testHunterReactions(checks);
    testMissingPlayersAndCommittedActions(checks);
    testManualValidationAndStress(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding allocations after event tests");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.getTotal() << " checks, " << checks.getFailures() << " failures.\n";
    return checks.getFailures() == 0 ? 0 : 1;
}
