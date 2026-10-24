#include "World.h"
#include "Roles.h"
#include "Command.h"

#include <iostream>
#include <limits>
#include <stdexcept>

World::World()
    : nextId(1), phase(NIGHT), completedTurns(0), runningTurn(false),
      resolvingEvents(false), packTargetId(-1), output(std::cout) {
}

World::World(std::ostream& output)
    : nextId(1), phase(NIGHT), completedTurns(0), runningTurn(false),
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
    // Role selection is confined to construction, never the normal turn loop.
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
    // Zero is an exhausted-ID marker, never a player ID. Removed IDs are not reused.
    if (nextId == std::numeric_limits<int>::max()) {
        nextId = 0;
    } else {
        ++nextId;
    }
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
    copy->id = nextId; // clone itself preserves identity for future undo snapshots.
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
    // If string validation/allocation fails, remove only this attempted edit.
    try { player->setName(name); }
    catch (...) {
        Command* command = nullptr;
        history.pop(command);
        delete command;
        throw;
    }
    player->setAlive(alive);
    player->setProtected(protectedFlag);
    return true;
}

bool World::removePlayer(int id) {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("Players cannot be removed during a turn.");
    }
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            remember(new DeleteCmd(*players[i], i));
            // Administrative removal is not a gameplay death: do not call onDeath.
            delete players[i];
            players.erase(players.begin() + i);
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
    // Keep the command on the stack if output or restoration fails.
    output << "Undo last player edit.\n";
    command->undo(*this);
    history.pop(command);
    delete command;
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
    // The positive int ID range also bounds the maximum number of players.
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

bool World::runTurn() {
    if (runningTurn || resolvingEvents) {
        throw std::logic_error("A turn is already running.");
    }
    if (livingPlayerCount() == 0) {
        output << "No living players; the turn did not advance.\n";
        return false;
    }
    if (completedTurns == std::numeric_limits<int>::max()) {
        throw std::overflow_error("The turn counter is full.");
    }

    discardHistory(); // Gameplay cannot be undone as a player-management edit.
    runningTurn = true;
    try {
        output << "Turn " << completedTurns + 1 << " - "
               << (phase == NIGHT ? "Night" : "Day") << '\n';
        if (phase == NIGHT) {
            ballots.clear();
            packTargetId = -1;
            // Current roles use priorities 0, 1, and 2. Keep insertion order within
            // each group; do not reorder the owning vector or inspect role types.
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
            // Tally elimination may enqueue a Hunter shot, including chain reactions.
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
    // This low-level helper can also apply gameplay outside runTurn.
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
    // These are manual-menu validations, not role dispatch in a normal turn.
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
    if (phase != DAY || voter == nullptr || target == nullptr || !target->isAlive()
        || voterId == targetId) {
        return false;
    }
    for (int i = 0; i < ballotCount(); ++i) {
        if (ballots[i].voterId == voterId) {
            return false; // First valid ballot wins, including a manual override.
        }
    }
    Ballot ballot = {voterId, targetId, voter->voteWeight()};
    ballots.push_back(ballot);
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
    if (target == nullptr || !target->isAlive()) {
        return false;
    }
    if (respectProtection && target->isProtected()) {
        reportEvent("Player " + std::to_string(targetId) + " survived " + cause + " (protected).");
        return false;
    }
    target->setAlive(false); // Set this before the hook so repeated attacks cannot repeat it.
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
