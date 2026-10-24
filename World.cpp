#include "World.h"
#include "Roles.h"

#include <iostream>
#include <limits>
#include <stdexcept>

World::World()
    : nextId(1), phase(NIGHT), completedTurns(0), runningTurn(false), output(std::cout) {
}

World::World(std::ostream& output)
    : nextId(1), phase(NIGHT), completedTurns(0), runningTurn(false), output(output) {
}

World::~World() {
    for (int i = 0; i < playerCount(); ++i) {
        delete players[i];
    }
}

int World::createPlayer(RoleKind role) {
    return createPlayer(role, roleName(role));
}

int World::createPlayer(RoleKind role, const std::string& name) {
    if (runningTurn) {
        throw std::logic_error("Players cannot be created during a turn.");
    }
    if (nextId == 0) {
        throw std::overflow_error("No player IDs remain.");
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
        players.push_back(player);
    } catch (...) {
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

bool World::removePlayer(int id) {
    if (runningTurn) {
        throw std::logic_error("Players cannot be removed during a turn.");
    }
    for (int i = 0; i < playerCount(); ++i) {
        if (players[i]->getId() == id) {
            // Administrative removal is not a gameplay death: do not call onDeath.
            delete players[i];
            players.erase(players.begin() + i);
            return true;
        }
    }
    return false;
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
    if (runningTurn) {
        throw std::logic_error("A turn is already running.");
    }
    if (livingPlayerCount() == 0) {
        output << "No living players; the turn did not advance.\n";
        return false;
    }
    if (completedTurns == std::numeric_limits<int>::max()) {
        throw std::overflow_error("The turn counter is full.");
    }

    runningTurn = true;
    try {
        output << "Turn " << completedTurns + 1 << " - "
               << (phase == NIGHT ? "Night" : "Day") << '\n';
        if (phase == NIGHT) {
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
    } catch (...) {
        runningTurn = false;
        throw;
    }
    // M4 will resolve queued events here, before the phase changes.
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
