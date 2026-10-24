#include "World.h"
#include "Roles.h"
#include "Command.h"

#include <iostream>
#include <limits>
#include <stdexcept>

World::World()
    : nextId(1), phase(NIGHT), completedTurns(0), result(GAME_ONGOING),
      totalBallots(0), totalVoteWeight(0), runningTurn(false),
      resolvingEvents(false), packTargetId(-1), output(std::cout) {
}

World::World(std::ostream& output)
    : nextId(1), phase(NIGHT), completedTurns(0), result(GAME_ONGOING),
      totalBallots(0), totalVoteWeight(0), runningTurn(false),
      resolvingEvents(false), packTargetId(-1), output(output) {
}

World::~World() {
    discardHistory();
    discardPendingEvents();
    for (int i = 0; i < playerCount(); ++i) {
        delete players[i];
    }
}

int World::createPlayer(RoleKind role) {
    return createPlayer(role, roleName(role));
}

int World::createPlayer(RoleKind role, const std::string& name) {
    return createPlayer(role, name, true, false);
}

int World::createPlayer(RoleKind role, const std::string& name, bool alive, bool protectedFlag) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Players cannot be created during a turn.");
    }
    if (nextId == 0) {
        throw std::overflow_error("No player IDs remain.");
    }
    if (!alive && protectedFlag) {
        throw std::invalid_argument("A dead player cannot be protected.");
    }

    Player* player = nullptr;

    switch (role) {
        case VILLAGER_ROLE: player = new Villager(nextId, name); break;
        case SEER_ROLE: player = new Seer(nextId, name); break;
        case WITCH_ROLE: player = new Witch(nextId, name); break;
        case HUNTER_ROLE: player = new Hunter(nextId, name); break;
        case WEREWOLF_ROLE: player = new Werewolf(nextId, name); break;
        case ALPHA_WOLF_ROLE: player = new AlphaWolf(nextId, name); break;
        default: throw std::invalid_argument("Unknown role.");
    }
    try {
        player->setAlive(alive);
        player->setProtected(protectedFlag);
        remember(new CreateCmd(nextId));
    } catch (...) {
        delete player;
        throw;
    }
    try { players.push_back(player); }
    catch (...) {
        Command* command = nullptr;
        history.pop(command);
        delete command;
        delete player;
        throw;
    }

    int assignedId = nextId;

    if (nextId == std::numeric_limits<int>::max()) {
        nextId = 0;
    } else {
        ++nextId;
    }
    result = GAME_ONGOING;
    return assignedId;
}

int World::duplicatePlayer(int id) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Players cannot be duplicated during a turn.");
    }
    const Player* source = findPlayer(id);
    if (source == nullptr) { return 0; }
    if (nextId == 0) { throw std::overflow_error("No player IDs remain."); }
    Player* copy = source->clone();
    copy->id = nextId;
    try { remember(new DuplicateCmd(nextId)); }
    catch (...) { delete copy; throw; }
    try { players.push_back(copy); }
    catch (...) {
        Command* command = nullptr;
        history.pop(command);
        delete command;
        delete copy;
        throw;
    }
    const int assignedId = nextId;
    nextId = nextId == std::numeric_limits<int>::max() ? 0 : nextId + 1;
    result = GAME_ONGOING;
    return assignedId;
}

bool World::modifyPlayer(int id, const std::string& name, bool alive, bool protectedFlag) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Players cannot be modified during a turn.");
    }
    Player* player = findPlayer(id);
    if (player == nullptr) { return false; }
    if (!alive && protectedFlag) {
        throw std::invalid_argument("A dead player cannot be protected.");
    }
    int position = 0;
    while (players[position] != player) { ++position; }
    remember(new ModifyCmd(*player, position));

    try { player->setName(name); }
    catch (...) {
        Command* command = nullptr;
        history.pop(command);
        delete command;
        throw;
    }
    player->setAlive(alive);
    player->setProtected(protectedFlag);
    result = GAME_ONGOING;
    return true;
}

bool World::removePlayer(int id) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Players cannot be removed during a turn.");
    }
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            remember(new DeleteCmd(*players[i], i));

            delete players[i];
            players.erase(players.begin() + i);
            result = GAME_ONGOING;
            return true;
        }
    }
    return false;
}

void World::remember(Command* command) {
    try { history.push(command); }
    catch (...) { delete command; throw; }
}

void World::discardHistory() {
    Command* command = nullptr;
    while (history.pop(command)) { delete command; }
}

void World::clearUndoHistory() {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Undo history cannot be cleared during a turn.");
    }
    discardHistory();
}

int World::undoCount() const { return history.size(); }

bool World::undo() {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Undo is only available between turns.");
    }
    Command* command = nullptr;
    if (!history.peek(command)) {
        output << "Nothing to undo.\n";
        return false;
    }

    output << "Undo last player edit.\n";
    command->undo(*this);
    history.pop(command);
    delete command;
    result = GAME_ONGOING;
    return true;
}

void World::eraseForUndo(int id) {
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            delete players[i];
            players.erase(players.begin() + i);
            return;
        }
    }
    throw std::logic_error("Undo player is missing.");
}

void World::insertForUndo(Player* snapshot, int position) {
    if (snapshot == nullptr || position < 0 || position > playerCount()
        || findPlayer(snapshot->getId()) != nullptr) {
        throw std::logic_error("Cannot restore the saved player position or ID.");
    }
    players.insert(players.begin() + position, snapshot);
}

void World::replaceForUndo(Player* snapshot, int position) {
    if (snapshot == nullptr || position < 0 || position >= playerCount()
        || players[position]->getId() != snapshot->getId()) {
        throw std::logic_error("Cannot restore the saved player state.");
    }
    delete players[position];
    players[position] = snapshot;
}

Player* World::findPlayer(int id) {
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            return players[i];
        }
    }
    return nullptr;
}

const Player* World::findPlayer(int id) const {
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            return players[i];
        }
    }
    return nullptr;
}

const Player* World::playerAt(int index) const {
    if (index < 0 || index >= playerCount()) {
        throw std::out_of_range("Player index is out of range.");
    }
    return players[index];
}

int World::playerCount() const {

    return static_cast<int>(players.size());
}

int World::livingPlayerCount() const {
    int living = 0;
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->isAlive()) {
            ++living;
        }
    }
    return living;
}

void World::describePlayers() const {
    if (players.empty()) {
        output << "The world has no players.\n";
    }
    for (int i = 0; i < playerCount(); ++i) {
        players[i]->describe(output);
    }
}

Phase World::getPhase() const { return phase; }
int World::getCompletedTurns() const { return completedTurns; }
int World::getCompletedRounds() const { return completedTurns / 2; }
GameResult World::getResult() const { return result; }
bool World::isFinished() const { return result != GAME_ONGOING; }

const char* World::resultText() const {
    switch (result) {
        case VILLAGE_WIN: return "Villagers win";
        case WOLF_WIN: return "Wolves win";
        case GAME_DRAW: return "Draw (no survivors)";
        default: return "In progress";
    }
}

long long World::getTotalBallots() const { return totalBallots; }
long long World::getTotalVoteWeight() const { return totalVoteWeight; }
int World::killCount() const { return static_cast<int>(kills.size()); }

KillRecord World::killAt(int index) const {
    if (index < 0 || index >= killCount()) { throw std::out_of_range("Kill index is out of range."); }
    return kills[index];
}

void World::checkVictory() {
    int wolves = 0, village = 0;
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->isAlive()) {
            if (players[i]->isWolf()) { ++wolves; }
            else { ++village; }
        }
    }
    if (wolves == 0 && village == 0) { result = GAME_DRAW; }
    else if (wolves == 0) { result = VILLAGE_WIN; }
    else if (wolves >= village) { result = WOLF_WIN; }
    else { result = GAME_ONGOING; }
}

void World::describeStatistics() const {
    output << "\nSimulation statistics\nResult: " << resultText()
           << "\nTurns completed: " << completedTurns
           << "\nRounds completed: " << getCompletedRounds()
           << "\nBallots cast: " << totalBallots
           << "\nWeighted vote total: " << totalVoteWeight
           << "\nEliminations: " << killCount() << '\n';
    for (int i = 0; i < killCount(); ++i) {
        const KillRecord& record = kills[i];
        output << "  ";
        if (record.turnNumber == 0) { output << "Between turns"; }
        else { output << "Turn " << record.turnNumber; }
        output << " (" << (record.phase == NIGHT ? "Night" : "Day") << "): "
               << record.sourceName << " [" << record.sourceId << "] -> "
               << record.targetName << " [" << record.targetId << "] by " << record.cause << '\n';
    }
}

bool World::runTurn() {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("A turn is already running.");
    }
    if (isFinished()) {
        output << "Game already finished: " << resultText() << ". Make a player edit to continue.\n";
        return false;
    }
    if (livingPlayerCount() == 0) {
        output << "No living players; the turn did not advance.\n";
        return false;
    }
    if (completedTurns == std::numeric_limits<int>::max()) {
        throw std::overflow_error("The turn counter is full.");
    }

    discardHistory();
    runningTurn = true;
    try {
        output << "Turn " << completedTurns + 1 << " - "
               << (phase == NIGHT ? "Night" : "Day") << '\n';
        if (phase == NIGHT) {
            ballots.clear();
            packTargetId = -1;

            for (int priority = 0; priority <= 2; ++priority) {
                for (int i = 0; i < playerCount(); ++i) {
                    Player* player = players[i];
                    if (player->isAlive() && player->nightOrder() == priority) {
                        player->act(*this);
                    }
                }
            }
        } else {
            for (int i = 0; i < playerCount(); ++i) {
                Player* player = players[i];
                if (player->isAlive()) {
                    player->act(*this);
                }
            }
        }
        processEvents();
        if (phase == DAY) {
            tallyVotes();

            processEvents();
        } else {
            clearProtection();
        }
    } catch (...) {
        discardPendingEvents();
        runningTurn = false;
        throw;
    }
    ++completedTurns;
    phase = phase == NIGHT ? DAY : NIGHT;
    runningTurn = false;
    checkVictory();
    if (isFinished()) { describeStatistics(); }
    return true;
}

int World::runTurns(int numberOfTurns) {
    if (numberOfTurns < 0) {
        throw std::invalid_argument("The number of turns cannot be negative.");
    }
    if (numberOfTurns > std::numeric_limits<int>::max() - completedTurns) {
        throw std::overflow_error("The requested turns exceed the counter limit.");
    }
    int completed = 0;
    for (int i = 0; i < numberOfTurns; ++i) {
        if (!runTurn()) {
            break;
        }
        ++completed;
        if (isFinished()) { break; }
    }
    return completed;
}

void World::reportAction(const Player& player, const std::string& action) {
    output << "  [" << player.getId() << "] " << player.getName() << ": " << action << '\n';
}

void World::reportEvent(const std::string& message) {
    output << "  Event: " << message << '\n';
}

void World::enqueueEvent(Event* event) {
    if (event == nullptr) {
        throw std::invalid_argument("Cannot enqueue a null event.");
    }
    if (isFinished()) {
        delete event;
        throw std::logic_error("Game finished. Make a player edit before adding events.");
    }
    try {
        events.enqueue(event);
    } catch (...) {
        delete event;
        throw;
    }
}

void World::discardPendingEvents() {
    Event* event = nullptr;
    while (events.dequeue(event)) {
        delete event;
    }
}

int World::pendingEventCount() const { return events.size(); }

void World::viewPendingEvents() const {
    if (events.isEmpty()) {
        output << "No pending events.\n";
    }
    for (int i = 0; i < events.size(); ++i) {
        output << i + 1 << ". ";
        events.at(i)->describe(output);
    }
}

void World::processEvents() {
    if (resolvingEvents) {
        throw std::logic_error("Events are already being resolved.");
    }

    if (!events.isEmpty()) { discardHistory(); }
    resolvingEvents = true;
    Event* event = nullptr;
    try {
        while (events.dequeue(event)) {
            try {
                output << "  Resolve: ";
                event->describe(output);
                event->apply(*this);
            } catch (...) {
                delete event;
                event = nullptr;
                throw;
            }
            delete event;
            event = nullptr;
        }
    } catch (...) {
        discardPendingEvents();
        resolvingEvents = false;
        throw;
    }
    resolvingEvents = false;
}

bool World::addManualEvent(EventKind kind, int sourceId, int targetId) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Manual events can only be added between turns.");
    }
    if (isFinished()) {
        reportEvent("Game finished. Make a player edit before adding events.");
        return false;
    }
    Player* source = findPlayer(sourceId);
    Player* target = findPlayer(targetId);
    if (source == nullptr || target == nullptr || !source->isAlive() || !target->isAlive()) {
        reportEvent("Not queued: select existing, living source and target players.");
        return false;
    }
    if (sourceId == targetId && kind != HEAL_EVENT) {
        reportEvent("Not queued: only healing can target its source.");
        return false;
    }

    switch (kind) {
        case KILL_EVENT:
            if (phase == NIGHT) { enqueueEvent(new KillEvent(sourceId, targetId)); return true; }
            break;
        case HEAL_EVENT:
            if (phase == NIGHT && source->getRole() == WITCH_ROLE) {
                enqueueEvent(new HealEvent(sourceId, targetId)); return true;
            }
            break;
        case POISON_EVENT:
            if (phase == NIGHT && source->getRole() == WITCH_ROLE) {
                enqueueEvent(new KillEvent(sourceId, targetId, true)); return true;
            }
            break;
        case REVEAL_EVENT:
            if (phase == NIGHT && source->getRole() == SEER_ROLE) {
                enqueueEvent(new RevealEvent(sourceId, targetId)); return true;
            }
            break;
        case VOTE_EVENT:
            if (phase == DAY) { enqueueEvent(new VoteEvent(sourceId, targetId)); return true; }
            break;
        default: break;
    }
    reportEvent("Not queued: event kind, phase, or source role is invalid.");
    return false;
}

bool World::requestInspection(int sourceId, int targetId) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Special actions are only available between turns.");
    }
    if (isFinished()) {
        reportEvent("Game finished. Make a player edit before requesting special actions.");
        return false;
    }
    Player* source = findPlayer(sourceId);
    if (source == nullptr) {
        reportEvent("Unknown player ID.");
        return false;
    }

    Inspector* inspector = dynamic_cast<Inspector*>(source);
    if (inspector == nullptr) {
        reportEvent("This player cannot inspect.");
        return false;
    }
    return inspector->inspect(*this, targetId);
}

bool World::requestPotion(int sourceId, int targetId, bool healing) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Special actions are only available between turns.");
    }
    if (isFinished()) {
        reportEvent("Game finished. Make a player edit before requesting special actions.");
        return false;
    }
    Player* source = findPlayer(sourceId);
    if (source == nullptr) {
        reportEvent("Unknown player ID.");
        return false;
    }
    PotionUser* potionUser = dynamic_cast<PotionUser*>(source);
    if (potionUser == nullptr) {
        reportEvent("This player cannot use potions.");
        return false;
    }
    return potionUser->usePotion(*this, targetId, healing);
}

int World::lowestLivingTarget(int excludedId, bool villageOnly) const {
    int chosen = 0;
    for (int i = 0; i < playerCount(); ++i) {
        const Player* player = players[i];
        if (player->isAlive() && player->getId() != excludedId
            && (!villageOnly || !player->isWolf())
            && (chosen == 0 || player->getId() < chosen)) {
            chosen = player->getId();
        }
    }
    return chosen;
}

int World::packVictim() {
    if (packTargetId == -1) {
        packTargetId = lowestLivingTarget(0, true);
    }
    return packTargetId;
}

void World::queueVote(int voterId, int targetId) {
    if (targetId != 0) {
        enqueueEvent(new VoteEvent(voterId, targetId));
    }
}

bool World::recordVote(int voterId, int targetId) {
    const Player* voter = findPlayer(voterId);
    const Player* target = findPlayer(targetId);
    if (isFinished() || phase != DAY || voter == nullptr || target == nullptr || !target->isAlive()
        || voterId == targetId) {
        return false;
    }
    for (int i = 0; i < ballotCount(); ++i) {
        if (ballots[i].voterId == voterId) {
            return false;
        }
    }
    Ballot ballot = {voterId, targetId, voter->voteWeight()};
    if (totalBallots == std::numeric_limits<long long>::max()
        || totalVoteWeight > std::numeric_limits<long long>::max() - ballot.weight) {
        throw std::overflow_error("The cumulative vote counter is full.");
    }
    ballots.push_back(ballot);
    ++totalBallots;
    totalVoteWeight += ballot.weight;
    return true;
}

int World::ballotCount() const { return static_cast<int>(ballots.size()); }

long long World::voteTotal(int targetId) const {
    long long total = 0;
    for (int i = 0; i < ballotCount(); ++i) {
        if (ballots[i].targetId == targetId) {
            total += ballots[i].weight;
        }
    }
    return total;
}

void World::tallyVotes() {
    int targetId = 0;
    long long highest = 0;
    bool tied = false;
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->isAlive()) {
            long long total = voteTotal(players[i]->getId());
            if (total > highest) {
                highest = total;
                targetId = players[i]->getId();
                tied = false;
            } else if (total > 0 && total == highest) {
                tied = true;
            }
        }
    }
    if (targetId == 0 || tied) {
        reportEvent("No day elimination: no valid votes or a tied highest total.");
    } else {
        eliminate(0, targetId, "village vote", false);
    }
}

bool World::eliminate(int sourceId, int targetId, const std::string& cause, bool respectProtection) {
    Player* target = findPlayer(targetId);
    if (isFinished() || target == nullptr || !target->isAlive()) {
        return false;
    }
    if (respectProtection && target->isProtected()) {
        reportEvent("Player " + std::to_string(targetId) + " survived " + cause + " (protected).");
        return false;
    }
    if (killCount() == std::numeric_limits<int>::max()) {
        throw std::length_error("The elimination history is full.");
    }
    const Player* source = findPlayer(sourceId);
    const std::string sourceName = sourceId == 0 ? "Village vote"
        : (source == nullptr ? "Unknown source" : source->getName());
    KillRecord record = {sourceId, targetId, sourceName, target->getName(), cause,
                         phase, runningTurn ? completedTurns + 1 : 0};
    kills.push_back(record);
    target->setAlive(false);
    target->onDeath(*this);
    reportEvent("Player " + std::to_string(targetId) + " died by " + cause
                + " (source " + std::to_string(sourceId) + ").");
    return true;
}

void World::clearProtection() {
    for (int i = 0; i < playerCount(); ++i) {
        players[i]->setProtected(false);
    }
}
