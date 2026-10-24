#ifndef WORLD_H
#define WORLD_H

#include "Player.h"
#include "Queue.h"
#include "Event.h"
#include "Stack.h"

#include <iosfwd>
#include <string>
#include <vector>

enum Phase { NIGHT, DAY };
enum GameResult { GAME_ONGOING, VILLAGE_WIN, WOLF_WIN, GAME_DRAW };

class Command;
class CreateCmd;
class DuplicateCmd;
class DeleteCmd;
class ModifyCmd;

// A valid ballot; IDs stay safe even if players later die or are removed.
struct Ballot {
    int voterId;
    int targetId;
    int weight;
};

// Historical names and IDs survive later renaming or administrative deletion.
struct KillRecord {
    int sourceId;
    int targetId;
    std::string sourceName;
    std::string targetName;
    std::string cause;
    Phase phase;
    int turnNumber;
};

// Owns the village's players and runs phase-dependent actions polymorphically.
// The output stream is borrowed and must outlive this World.
// Player lookups are borrowed too: removal and modification undo can invalidate
// those pointers. Keep IDs across edits, as queued events do.
class World {
    friend class CreateCmd;
    friend class DuplicateCmd;
    friend class DeleteCmd;
    friend class ModifyCmd;
private:
    std::vector<Player*> players;
    int nextId;
    Phase phase;
    int completedTurns;
    GameResult result;
    long long totalBallots;
    long long totalVoteWeight;
    bool runningTurn;
    bool resolvingEvents;
    Queue<Event*> events;
    Stack<Command*> history;
    std::vector<Ballot> ballots;
    std::vector<KillRecord> kills;
    int packTargetId;
    std::ostream& output;

    void discardPendingEvents();
    void tallyVotes();
    void clearProtection();
    void checkVictory();
    void discardHistory();
    void remember(Command* command);
    void eraseForUndo(int id);
    void insertForUndo(Player* snapshot, int position);
    void replaceForUndo(Player* snapshot, int position);

public:
    World();
    explicit World(std::ostream& output);
    ~World();

    World(const World& other) = delete;
    World& operator=(const World& other) = delete;

    int createPlayer(RoleKind role);
    int createPlayer(RoleKind role, const std::string& name);
    int createPlayer(RoleKind role, const std::string& name, bool alive, bool protectedFlag);
    bool removePlayer(int id);
    int duplicatePlayer(int id);
    bool modifyPlayer(int id, const std::string& name, bool alive, bool protectedFlag);
    bool undo();
    int undoCount() const;
    void clearUndoHistory();
    Player* findPlayer(int id);
    const Player* findPlayer(int id) const;
    const Player* playerAt(int index) const;
    int playerCount() const;
    int livingPlayerCount() const;
    void describePlayers() const;

    Phase getPhase() const;
    int getCompletedTurns() const;
    int getCompletedRounds() const;
    GameResult getResult() const;
    bool isFinished() const;
    const char* resultText() const;
    long long getTotalBallots() const;
    long long getTotalVoteWeight() const;
    int killCount() const;
    KillRecord killAt(int index) const;
    void describeStatistics() const;
    bool runTurn();
    int runTurns(int numberOfTurns);
    void reportAction(const Player& player, const std::string& action);
    void reportEvent(const std::string& message);

    void enqueueEvent(Event* event);
    bool addManualEvent(EventKind kind, int sourceId, int targetId);
    bool requestInspection(int sourceId, int targetId);
    bool requestPotion(int sourceId, int targetId, bool healing);
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
