#ifndef ROLES_H
#define ROLES_H

#include "Teams.h"

// One suspicion score associated with a stable player ID.
struct SuspicionEntry {
    int playerId;
    int score;
};

// An exact role learned by a Seer, associated with a stable player ID.
struct InspectionResult {
    int playerId;
    RoleKind role;
};

// One consumed potion. The log also determines which potions remain available.
struct PotionUse {
    int targetId;
    bool healing;
};

// Villager owns its suspicions array and copies it independently.
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

// Seer owns its inspections array and copies it independently.
class Seer : public VillageTeam {
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
    int nightOrder() const override;
};

// Witch owns its potions used array and copies it independently.
class Witch : public VillageTeam {
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
    int nightOrder() const override;
};

// Hunter owns its past shots array and copies it independently.
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

// Werewolf owns its past targets array and copies it independently.
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

// AlphaWolf owns its pack members array and copies it independently.
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
