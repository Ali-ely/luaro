#include "World.h"
#include "Console.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Local test counters, including result and factual-history invariants.
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

// Destruction confirms ownership when a finished game rejects an event.
class CountedEvent : public Event {
private:
    int& destroyed;
public:
    explicit CountedEvent(int& destroyed) : Event(1, 2), destroyed(destroyed) { }
    ~CountedEvent() override { ++destroyed; }
    void apply(World&) override { }
    void describe(std::ostream& output) const override { output << "Counted event\n"; }
};

// Observe the temporary parity reached before the last Hunter reaction.
class ObserveChain : public Event {
private:
    Checks& checks;
    bool first;
public:
    ObserveChain(Checks& checks, bool first) : Event(3, 1), checks(checks), first(first) { }
    void describe(std::ostream& output) const override { output << "Observe chain\n"; }
    void apply(World& world) override {
        checks.expect(world.getResult() == GAME_ONGOING, "No result declared while reactions remain queued");
        if (first) { world.enqueueEvent(new ObserveChain(checks, false)); }
        else { checks.expect(world.livingPlayerCount() == 2, "Temporary wolf parity observed before final shot"); }
    }
};

// Interrupt a turn after an actual death to verify cumulative facts survive.
class FailingEvent : public Event {
public:
    FailingEvent() : Event(3, 1) { }
    void describe(std::ostream& output) const override { output << "Failing event\n"; }
    void apply(World&) override { throw std::runtime_error("Test interruption"); }
};

void testInitialAndTerminalState(Checks& checks) {
    std::ostringstream output;
    World empty(output);
    checks.expect(empty.getResult() == GAME_ONGOING && !empty.isFinished()
                  && empty.getTotalBallots() == 0 && empty.getTotalVoteWeight() == 0
                  && empty.killCount() == 0, "Initial result and statistics are empty");
    checks.expect(!empty.runTurn() && empty.getResult() == GAME_ONGOING
                  && empty.getCompletedTurns() == 0, "An empty world cannot start and is not an event-chain draw");
    int rejected = 0;
    try { empty.killAt(0); } catch (const std::out_of_range&) { ++rejected; }
    try { empty.killAt(-1); } catch (const std::out_of_range&) { ++rejected; }
    checks.expect(rejected == 2, "Kill history rejects invalid indices");
    empty.describeStatistics();
    checks.expect(output.str().find("Result: In progress") != std::string::npos,
                  "Statistics view does not declare an early outcome");

    World parity(output);
    parity.createPlayer(WITCH_ROLE, "Healer");
    parity.createPlayer(WEREWOLF_ROLE, "Wolf");
    checks.expect(parity.getResult() == GAME_ONGOING, "Setup parity is checked only after a completed turn");
    checks.expect(parity.runTurns(100) == 1 && parity.getResult() == WOLF_WIN,
                  "Wolves win at parity and a batch stops on the winning turn");
    checks.expect(parity.getCompletedTurns() == 1 && parity.getCompletedRounds() == 0
                  && parity.getPhase() == DAY && parity.killCount() == 0,
                  "Protected Night can end at parity without recording a kill or full round");
    const int completed = parity.getCompletedTurns();
    checks.expect(!parity.runTurn() && parity.runTurns(5) == 0
                  && parity.getCompletedTurns() == completed, "A finished game never advances further turns");
    checks.expect(!parity.addManualEvent(KILL_EVENT, 2, 1)
                  && !parity.requestInspection(1, 2) && !parity.requestPotion(1, 2, false),
                  "Finished games reject new manual and capability events");
    int destroyed = 0;
    bool threw = false;
    try { parity.enqueueEvent(new CountedEvent(destroyed)); }
    catch (const std::logic_error&) { threw = true; }
    checks.expect(threw && destroyed == 1 && parity.pendingEventCount() == 0,
                  "Rejected owning enqueue deletes its event");
    checks.expect(!parity.recordVote(1, 2) && !parity.eliminate(2, 1, "attack", false)
                  && parity.killCount() == 0, "Low-level vote/elimination helpers cannot change a finished game");
    checks.expect(!parity.removePlayer(999) && parity.duplicatePlayer(999) == 0
                  && !parity.modifyPlayer(999, "Missing", true, false)
                  && parity.isFinished(), "Unknown management IDs do not reopen game");
    threw = false;
    try { parity.modifyPlayer(1, " ", true, false); }
    catch (const std::invalid_argument&) { threw = true; }
    checks.expect(threw && parity.isFinished() && !parity.undo(), "Invalid edit and empty undo retain terminal result");
    checks.expect(parity.createPlayer(VILLAGER_ROLE, "Arrival") == 3 && !parity.isFinished(),
                  "Successful creation explicitly reopens simulation");
    parity.undo();
    checks.expect(!parity.isFinished() && parity.playerCount() == 2
                  && parity.getCompletedTurns() == completed, "Undo reopens while preserving cumulative turn history");

    World onlyWolves(output);
    onlyWolves.createPlayer(WEREWOLF_ROLE);
    onlyWolves.createPlayer(ALPHA_WOLF_ROLE);
    checks.expect(onlyWolves.runTurns(10) == 1 && onlyWolves.getResult() == WOLF_WIN
                  && onlyWolves.killCount() == 0, "Wolves also win when no village players remain");
}

void testVillageWinAndDraw(Checks& checks) {
    std::ostringstream output;
    World village(output);
    village.createPlayer(WITCH_ROLE, "Clara");
    village.createPlayer(SEER_ROLE, "Basil");
    village.createPlayer(VILLAGER_ROLE, "Alice");
    village.createPlayer(WEREWOLF_ROLE, "Elias");
    village.requestPotion(1, 1, true);
    village.requestPotion(1, 4, false);
    checks.expect(village.runTurns(20) == 1 && village.getResult() == VILLAGE_WIN,
                  "Villagers win immediately after the last wolf dies");
    checks.expect(village.killCount() == 1 && village.getTotalBallots() == 0
                  && village.getTotalVoteWeight() == 0, "Poison victory before Day has no ballots");
    const KillRecord poison = village.killAt(0);
    checks.expect(poison.sourceId == 1 && poison.targetId == 4 && poison.sourceName == "Clara"
                  && poison.targetName == "Elias" && poison.cause == "poison"
                  && poison.phase == NIGHT && poison.turnNumber == 1, "Poison records exact actor, target, cause, phase, and turn");
    checks.expect(village.findPlayer(1)->isAlive() && !village.findPlayer(1)->isProtected(),
                  "Committed dead wolf attack resolves before victory and protection cleanup");
    checks.expect(output.str().find("Result: Villagers win") != std::string::npos,
                  "Victory automatically prints final statistics");

    World draw(output);
    draw.createPlayer(HUNTER_ROLE, "Hunter");
    draw.createPlayer(WEREWOLF_ROLE, "Wolf");
    checks.expect(draw.runTurns(10) == 1 && draw.getResult() == GAME_DRAW
                  && draw.livingPlayerCount() == 0, "Hunter killing the last wolf after dying produces a draw");
    checks.expect(draw.killCount() == 2 && draw.killAt(0).sourceId == 2
                  && draw.killAt(0).targetId == 1 && draw.killAt(1).sourceId == 1
                  && draw.killAt(1).targetId == 2 && draw.killAt(1).cause == "Hunter shot",
                  "Dead Hunter's reaction is fully recorded in order");
    checks.expect(draw.pendingEventCount() == 0 && draw.getCompletedTurns() == 1,
                  "Draw declared only after complete queue drain");

    World chain(output);
    chain.createPlayer(HUNTER_ROLE, "First Hunter");
    chain.createPlayer(HUNTER_ROLE, "Second Hunter");
    chain.createPlayer(WEREWOLF_ROLE, "Wolf");
    chain.createPlayer(VILLAGER_ROLE, "Survivor");
    chain.enqueueEvent(new KillEvent(3, 1));
    chain.enqueueEvent(new ObserveChain(checks, true));
    checks.expect(chain.runTurn() && chain.getResult() == VILLAGE_WIN
                  && chain.findPlayer(4)->isAlive(), "Hunter chain can turn temporary parity into a village victory");
    checks.expect(chain.killCount() == 3 && chain.killAt(0).targetId == 1
                  && chain.killAt(1).targetId == 2 && chain.killAt(2).targetId == 3,
                  "Duplicate pack attack does not add a duplicate death record");

    World day(output);
    day.createPlayer(HUNTER_ROLE, "Day Hunter");
    day.createPlayer(WEREWOLF_ROLE, "Last Wolf");
    day.createPlayer(VILLAGER_ROLE);
    day.createPlayer(VILLAGER_ROLE);
    day.createPlayer(WITCH_ROLE);
    day.addManualEvent(HEAL_EVENT, 5, 1);
    day.runTurn();
    day.addManualEvent(VOTE_EVENT, 1, 2);
    for (int id = 2; id <= 5; ++id) { day.addManualEvent(VOTE_EVENT, id, 1); }
    day.runTurn();
    checks.expect(day.getResult() == VILLAGE_WIN && day.killCount() == 2
                  && day.getTotalBallots() == 5 && day.getTotalVoteWeight() == 5,
                  "Day victory waits for the voted-out Hunter to shoot the last wolf");
    checks.expect(day.killAt(0).cause == "village vote" && day.killAt(0).targetId == 1
                  && day.killAt(1).cause == "Hunter shot" && day.killAt(1).targetId == 2
                  && day.killAt(1).phase == DAY && day.killAt(1).turnNumber == 2,
                  "Day tally and retaliation retain correct order, attribution, and phase");
}

void testWeightedAndCumulativeVotes(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(ALPHA_WOLF_ROLE, "Alpha");
    world.createPlayer(VILLAGER_ROLE, "First voter");
    world.createPlayer(VILLAGER_ROLE, "Second voter");
    world.createPlayer(WITCH_ROLE, "Witch");
    world.addManualEvent(HEAL_EVENT, 4, 2);
    world.runTurn();
    checks.expect(!world.isFinished() && world.killCount() == 0
                  && world.getTotalBallots() == 0, "Early protection leaves an ongoing game without phantom statistics");
    world.addManualEvent(VOTE_EVENT, 1, 2);
    world.addManualEvent(VOTE_EVENT, 2, 3);
    world.addManualEvent(VOTE_EVENT, 3, 2);
    world.addManualEvent(VOTE_EVENT, 4, 3);
    world.addManualEvent(VOTE_EVENT, 3, 2);
    world.runTurn();
    checks.expect(world.getTotalBallots() == 4 && world.getTotalVoteWeight() == 5
                  && world.ballotCount() == 4, "Accepted ballots counted separately from AlphaWolf's weight");
    checks.expect(world.voteTotal(2) == 3 && world.voteTotal(3) == 2,
                  "Manual overrides and repeated events produce one ballot per voter");
    const KillRecord vote = world.killAt(0);
    checks.expect(vote.sourceId == 0 && vote.sourceName == "Village vote" && vote.targetId == 2
                  && vote.targetName == "First voter" && vote.cause == "village vote"
                  && vote.turnNumber == 2 && vote.phase == DAY, "Vote elimination attributed to collective village");
    checks.expect(world.getCompletedRounds() == 1 && !world.isFinished(), "One complete round recorded");
    checks.expect(world.runTurn() && world.getResult() == WOLF_WIN && world.killCount() == 2,
                  "Following Night attack reaches wolf parity");
    checks.expect(world.ballotCount() == 0 && world.getTotalBallots() == 4
                  && world.getTotalVoteWeight() == 5, "Night clears current ballots but preserves cumulative totals");
    checks.expect(world.getCompletedTurns() == 3 && world.getCompletedRounds() == 1,
                  "Night victory reports complete rounds without rounding up");
    world.createPlayer(VILLAGER_ROLE, "New voter");
    checks.expect(!world.isFinished() && world.killCount() == 2 && world.getTotalBallots() == 4,
                  "Administrative reopen retains prior factual statistics");
    world.runTurn();
    checks.expect(!world.isFinished() && world.getTotalBallots() == 7
                  && world.getTotalVoteWeight() == 9 && world.killCount() == 2,
                  "Tied next Day adds real ballots without an elimination");
    checks.expect(world.getCompletedRounds() == 2, "Reopened simulation retains and advances round history");

    World committed(output);
    committed.createPlayer(WITCH_ROLE);
    committed.createPlayer(VILLAGER_ROLE);
    committed.createPlayer(VILLAGER_ROLE);
    committed.createPlayer(WEREWOLF_ROLE);
    committed.runTurn();
    committed.enqueueEvent(new KillEvent(4, 2));
    committed.enqueueEvent(new VoteEvent(2, 4));
    committed.runTurn();
    checks.expect(committed.getTotalBallots() == 3 && committed.getTotalVoteWeight() == 3,
                  "Committed dead-source ballot counts while ballots for dead targets do not");
    checks.expect(committed.killCount() == 2 && committed.killAt(0).targetId == 2
                  && committed.killAt(1).targetId == 1, "Direct committed attack and later tally both logged");
}

void testHistoryRetentionAndIgnoredActions(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(WITCH_ROLE, "Original Witch");
    world.createPlayer(VILLAGER_ROLE, "Original Victim");
    world.createPlayer(WEREWOLF_ROLE, "Original Wolf");
    world.requestPotion(1, 1, true);
    world.enqueueEvent(new KillEvent(3, 1));
    world.enqueueEvent(new KillEvent(3, 2));
    world.enqueueEvent(new KillEvent(3, 2));
    world.processEvents();
    checks.expect(world.killCount() == 1 && world.killAt(0).turnNumber == 0,
                  "Protected, dead-target, and duplicate attacks excluded from history");
    world.modifyPlayer(2, "Renamed Victim", false, false);
    world.modifyPlayer(3, "Renamed Wolf", true, false);
    world.removePlayer(2);
    world.removePlayer(3);
    checks.expect(world.killAt(0).sourceName == "Original Wolf"
                  && world.killAt(0).targetName == "Original Victim"
                  && world.killAt(0).sourceId == 3 && world.killAt(0).targetId == 2,
                  "Historical names and IDs survive renaming and deletion");
    world.undo();
    world.undo();
    checks.expect(world.killCount() == 1, "Administrative delete/undo never rewrites factual kills");
    world.modifyPlayer(1, "Administrative death", false, false);
    int copy = world.duplicatePlayer(2);
    world.removePlayer(copy);
    checks.expect(world.killCount() == 1, "Administrative death, duplication, and deletion are not gameplay kills");
    world.describeStatistics();
    checks.expect(output.str().find("Original Wolf [3] -> Original Victim [2] by attack") != std::string::npos,
                  "Report uses saved historical names rather than current objects");
    int rejected = 0;
    try { world.killAt(-1); } catch (const std::out_of_range&) { ++rejected; }
    try { world.killAt(1); } catch (const std::out_of_range&) { ++rejected; }
    checks.expect(rejected == 2, "Populated history validates indices");
    KillRecord copied = world.killAt(0);
    copied.targetName = "Changed returned copy";
    checks.expect(world.killAt(0).targetName == "Original Victim", "History accessor returns an independent value");

    World deleted(output);
    deleted.createPlayer(WITCH_ROLE);
    deleted.createPlayer(VILLAGER_ROLE);
    deleted.createPlayer(WEREWOLF_ROLE);
    deleted.requestPotion(1, 2, false);
    deleted.removePlayer(2);
    deleted.processEvents();
    deleted.enqueueEvent(new KillEvent(3, 1));
    deleted.removePlayer(3);
    deleted.processEvents();
    checks.expect(deleted.killCount() == 0 && deleted.getTotalBallots() == 0,
                  "Deleted source/target references produce no phantom statistics");
}

void testInterruptedTurnAndReporting(Checks& checks) {
    std::ostringstream output;
    World world(output);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(VILLAGER_ROLE);
    world.createPlayer(WEREWOLF_ROLE);
    world.enqueueEvent(new KillEvent(3, 2));
    world.enqueueEvent(new FailingEvent());
    bool failed = false;
    try { world.runTurn(); } catch (const std::runtime_error&) { failed = true; }
    checks.expect(failed && world.getCompletedTurns() == 0 && world.getResult() == GAME_ONGOING,
                  "Interrupted turn does not claim completion or declare a result");
    checks.expect(world.killCount() == 1 && !world.findPlayer(2)->isAlive()
                  && world.pendingEventCount() == 0, "Actual effects/statistics persist under non-transactional exception cleanup");
    checks.expect(world.runTurn() && world.getResult() == WOLF_WIN && world.killCount() == 2,
                  "Recovery records only the additional real death");
    const int turns = world.getCompletedTurns(), kills = world.killCount();
    const long long ballots = world.getTotalBallots();
    const GameResult result = world.getResult();
    world.describeStatistics();
    world.describeStatistics();
    checks.expect(world.getCompletedTurns() == turns && world.killCount() == kills
                  && world.getTotalBallots() == ballots && world.getResult() == result,
                  "Repeated statistics views do not modify simulation state");

    std::ostringstream consoleOutput;
    std::istringstream input("14\n4\n5\n1000\n0\n");
    Console console(world, input, consoleOutput);
    console.run();
    checks.expect(world.getCompletedTurns() == turns && consoleOutput.str().find("Completed 0 turn(s).") != std::string::npos,
                  "Console exposes statistics and safely refuses post-game batches");
}

void testStress(Checks& checks) {
    const RoleKind roles[] = {WITCH_ROLE, SEER_ROLE, VILLAGER_ROLE, HUNTER_ROLE, WEREWOLF_ROLE, ALPHA_WOLF_ROLE};
    for (int repetition = 0; repetition < 20; ++repetition) {
        std::ostringstream output;
        World world(output);
        for (int i = 0; i < 12; ++i) { world.createPlayer(roles[i % 6], std::string(150, 'n')); }
        const int completed = world.runTurns(30);
        checks.expect(completed > 0 && world.isFinished() && world.pendingEventCount() == 0,
                      "Mixed-role games reach a fully resolved result");
        checks.expect(world.killCount() <= 12 && world.getTotalVoteWeight() >= world.getTotalBallots(),
                      "Stress histories contain only actual deaths and valid weights");
        world.describeStatistics();
        int id = world.createPlayer(VILLAGER_ROLE, "Reopen");
        world.modifyPlayer(id, "Snapshot", true, false);
        world.removePlayer(id);
    }
}

int main() {
    Checks checks;
    std::cout << "Running M9 results and statistics tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before, after, difference;
    _CrtMemCheckpoint(&before);
#endif
    testInitialAndTerminalState(checks);
    testVillageWinAndDraw(checks);
    testWeightedAndCumulativeVotes(checks);
    testHistoryRetentionAndIgnoredActions(checks);
    testInterruptedTurnAndReporting(checks);
    testStress(checks);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding histories, events, or command snapshots");
    if (leaked) { _CrtMemDumpStatistics(&difference); }
#endif
    std::cout << checks.total << " checks, " << checks.failures << " failures.\n";
    return checks.failures == 0 ? 0 : 1;
}
