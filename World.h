#ifndef WORLD_H
#define WORLD_H

#include "Player.h"
#include "Queue.h"
#include "Event.h"

#include <iosfwd>
#include <string>
#include <vector>

enum Phase { NIGHT, DAY };

// A valid ballot; IDs stay safe even if players later die or are removed.
struct Ballot {
    int voterId;
    int targetId;
    int weight;
};

// Owns the village's players and runs phase-dependent actions polymorphically.
// The output stream is borrowed and must outlive this World.
class World {
private:
    std::vector<Player*> players;
    int nextId;
    Phase phase;
    int completedTurns;
    bool runningTurn;
    bool resolvingEvents;
    Queue<Event*> events;
    std::vector<Ballot> ballots;
    int packTargetId;
    std::ostream& output;

    void discardPendingEvents();
    void tallyVotes();
    void clearProtection();

public:
    World();
    explicit World(std::ostream& output);
    ~World();

    // Copying owning pointers would cause double deletion; World is not copyable.
    World(const World& other) = delete;
    World& operator=(const World& other) = delete;

    int createPlayer(RoleKind role);
    int createPlayer(RoleKind role, const std::string& name);
    int createPlayer(RoleKind role, const std::string& name, bool alive, bool protectedFlag);
    bool removePlayer(int id);
    int duplicatePlayer(int id); // Returns 0 for an unknown ID.
    bool modifyPlayer(int id, const std::string& name, bool alive, bool protectedFlag);
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
    void reportEvent(const std::string& message);

    // Ownership transfers on entry, including when enqueueing throws.
    void enqueueEvent(Event* event);
    bool addManualEvent(EventKind kind, int sourceId, int targetId);
    int pendingEventCount() const;
    void viewPendingEvents() const;
    void processEvents();
    int lowestLivingTarget(int excludedId, bool villageOnly = false) const;
    int packVictim();
    void queueVote(int voterId, int targetId);
    bool recordVote(int voterId, int targetId);
    int ballotCount() const;
    long long voteTotal(int targetId) const;
    bool eliminate(int sourceId, int targetId, const std::string& cause, bool respectProtection);
};

#endif
