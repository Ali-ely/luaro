#ifndef WORLD_H
#define WORLD_H

#include "Player.h"

#include <iosfwd>
#include <string>
#include <vector>

enum Phase { NIGHT, DAY };

// Owns the village's players and runs phase-dependent actions polymorphically.
// The output stream is borrowed and must outlive this World.
class World {
private:
    std::vector<Player*> players;
    int nextId;
    Phase phase;
    int completedTurns;
    bool runningTurn;
    std::ostream& output;

public:
    World();
    explicit World(std::ostream& output);
    ~World();

    // Copying owning pointers would cause double deletion; World is not copyable.
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;

    int createPlayer(RoleKind role);
    int createPlayer(RoleKind role, const std::string& name);
    bool removePlayer(int id);
    Player* findPlayer(int id);
    const Player* findPlayer(int id) const;
    const Player* playerAt(int index) const;
    int playerCount() const;
    int livingPlayerCount() const;
    void describePlayers() const;

    Phase getPhase() const;
    int getCompletedTurns() const;
    int getCompletedRounds() const;
    bool runTurn();
    int runTurns(int numberOfTurns);
    void reportAction(const Player& player, const std::string& action);
};

#endif
