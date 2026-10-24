#ifndef ROLES_H
#define ROLES_H

#include "Teams.h"
#include "Capabilities.h"

// One suspicion score associated with a stable player ID.
struct SuspicionEntry {
    int playerId;
    int score;
};

// An exact role learned by a Seer, associated with a stable player ID.
struct InspectionResult {
    int playerId;
    RoleKind role;
    bool wolf;
};

// One consumed potion. The log also determines which potions remain available.
struct PotionUse {
    int targetId;
    bool healing;
};

// Ordinary village role: votes using its own suspicion array rather than secret roles.
// It belongs to VillageTeam and deep-copies its independently owned array.
class Villager : public VillageTeam {
private:
    SuspicionEntry* suspicions;
    int count;

public:
    explicit Villager(int id, const std::string& name = "Villager");
    Villager(const Villager& other);
    Villager& operator=(const Villager& other);
    ~Villager() override;

    RoleKind getRole() const override;
    Villager* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getSuspicionCount() const;
    SuspicionEntry getSuspicion(int index) const;
    void setSuspicion(int playerId, int score);
};

// Village investigator with an owned inspection history. Inspector is a separate
// capability so the optional user action is not offered by every Player.
class Seer : public VillageTeam, public Inspector {
private:
    InspectionResult* inspectionResults;
    int count;

public:
    explicit Seer(int id, const std::string& name = "Seer");
    Seer(const Seer& other);
    Seer& operator=(const Seer& other);
    ~Seer() override;

    RoleKind getRole() const override;
    Seer* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getInspectionCount() const;
    InspectionResult getInspection(int index) const;
    void recordInspection(int playerId, RoleKind role);
    bool receiveInspection(int targetId, RoleKind role) override;
    bool inspect(World& world, int targetId) override;
    int nightOrder() const override;
};

// Village potion user; its owned usage log determines remaining single-use stock.
// PotionUser adds the optional action without adding it to other village roles.
class Witch : public VillageTeam, public PotionUser {
private:
    PotionUse* potionLog;
    int count;

public:
    explicit Witch(int id, const std::string& name = "Witch");
    Witch(const Witch& other);
    Witch& operator=(const Witch& other);
    ~Witch() override;

    RoleKind getRole() const override;
    Witch* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getPotionUseCount() const;
    PotionUse getPotionUse(int index) const;
    bool hasHealingPotion() const;
    bool hasPoisonPotion() const;
    bool recordPotionUse(int targetId, bool healing);
    bool spendPotion(int targetId, bool healing) override;
    bool usePotion(World& world, int targetId, bool healing) override;
    int nightOrder() const override;
};

// Village defender: a virtual death reaction queues retaliation. Its owned shot
// history is independent of other Hunters and preserved by snapshots.
class Hunter : public VillageTeam {
private:
    int* pastShots;
    int count;

public:
    explicit Hunter(int id, const std::string& name = "Hunter");
    Hunter(const Hunter& other);
    Hunter& operator=(const Hunter& other);
    ~Hunter() override;

    RoleKind getRole() const override;
    Hunter* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getShotCount() const;
    int getShot(int index) const;
    void recordShot(int targetId);
    void onDeath(World& world) override;
};

// Standard WolfTeam attacker; records pack targets in its own deep-copied array.
class Werewolf : public WolfTeam {
private:
    int* pastTargets;
    int count;

public:
    explicit Werewolf(int id, const std::string& name = "Werewolf");
    Werewolf(const Werewolf& other);
    Werewolf& operator=(const Werewolf& other);
    ~Werewolf() override;

    RoleKind getRole() const override;
    Werewolf* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getTargetCount() const;
    int getTarget(int index) const;
    void recordTarget(int targetId);
};

// WolfTeam coordinator with an owned pack-members array and double-weight vote.
// Virtual voting weight distinguishes it without a role check in the tally.
class AlphaWolf : public WolfTeam {
private:
    int* packMembers;
    int count;

public:
    explicit AlphaWolf(int id, const std::string& name = "AlphaWolf");
    AlphaWolf(const AlphaWolf& other);
    AlphaWolf& operator=(const AlphaWolf& other);
    ~AlphaWolf() override;

    RoleKind getRole() const override;
    AlphaWolf* clone() const override;
    void describe(std::ostream& output) const override;
    void act(World& world) override;
    int getPackMemberCount() const;
    int getPackMember(int index) const;
    void addPackMember(int playerId);
    int voteWeight() const override;
};

#endif
