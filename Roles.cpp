#include "Roles.h"
#include "World.h"

#include <limits>
#include <ostream>
#include <stdexcept>

namespace {
// These basic templates are used only with int and the simple records above.
// All record fields are numbers, booleans, or enums, so copying a field cannot throw.
template <typename T>
T* copyArray(const T* source, int count) {
    if (count == 0) {
        return nullptr;
    }
    T* result = new T[count];
    for (int i = 0; i < count; ++i) {
        result[i] = source[i];
    }
    return result;
}

template <typename T>
void appendToArray(T*& entries, int& count, const T& value) {
    if (count == std::numeric_limits<int>::max()) {
        throw std::length_error("The role history is full.");
    }
    // Allocate and copy before deleting the old array.
    T* larger = new T[count + 1];
    for (int i = 0; i < count; ++i) {
        larger[i] = entries[i];
    }
    larger[count] = value;
    delete[] entries;
    entries = larger;
    ++count;
}

void requirePlayerId(int id) {
    if (id <= 0) {
        throw std::invalid_argument("Target IDs must be positive.");
    }
}

void requireIndex(int index, int count) {
    if (index < 0 || index >= count) {
        throw std::out_of_range("Role history index is out of range.");
    }
}
}

Villager::Villager(int id, const std::string& name)
    : VillageTeam(id, name), suspicions(nullptr), count(0) {
}

Villager::Villager(const Villager& other)
    : VillageTeam(other), suspicions(copyArray(other.suspicions, other.count)),
      count(other.count) {
}

Villager& Villager::operator=(const Villager& other) {
    if (this != &other) {
        SuspicionEntry* copied = copyArray(other.suspicions, other.count);
        try {
            VillageTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] suspicions;
        suspicions = copied;
        count = other.count;
    }
    return *this;
}

Villager::~Villager() {
    delete[] suspicions;
}

RoleKind Villager::getRole() const { return VILLAGER_ROLE; }

Villager* Villager::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new Villager(*this);
}

void Villager::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | suspicions: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << suspicions[i].playerId << ":" << suspicions[i].score << "]";
    }
    output << '\n';
}

void Villager::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        world.reportAction(*this, "Villager observes the village for suspicious activity.");
        for (int i = 0; i < world.playerCount(); ++i) {
            const Player* player = world.playerAt(i);
            if (!player->isAlive() || player->getId() == getId()) { continue; }
            bool known = false;
            for (int j = 0; j < count; ++j) {
                if (suspicions[j].playerId == player->getId()) { known = true; }
            }
            if (!known) { setSuspicion(player->getId(), 0); }
        }
    } else {
        world.reportAction(*this, "Villager weighs suspicion scores for a day vote.");
        int target = world.lowestLivingTarget(getId());
        int highest = 0;
        for (int i = 0; i < count; ++i) {
            const Player* player = world.findPlayer(suspicions[i].playerId);
            if (player != nullptr && player->isAlive() && player->getId() != getId()
                && (suspicions[i].score > highest
                    || (suspicions[i].score == highest && player->getId() < target))) {
                target = player->getId();
                highest = suspicions[i].score;
            }
        }
        world.queueVote(getId(), target);
    }
}

int Villager::getSuspicionCount() const { return count; }

SuspicionEntry Villager::getSuspicion(int index) const {
    requireIndex(index, count);
    return suspicions[index];
}

Seer::Seer(int id, const std::string& name)
    : VillageTeam(id, name), inspectionResults(nullptr), count(0) {
}

Seer::Seer(const Seer& other)
    : VillageTeam(other), inspectionResults(copyArray(other.inspectionResults, other.count)),
      count(other.count) {
}

Seer& Seer::operator=(const Seer& other) {
    if (this != &other) {
        InspectionResult* copied = copyArray(other.inspectionResults, other.count);
        try {
            VillageTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] inspectionResults;
        inspectionResults = copied;
        count = other.count;
    }
    return *this;
}

Seer::~Seer() {
    delete[] inspectionResults;
}

RoleKind Seer::getRole() const { return SEER_ROLE; }

Seer* Seer::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new Seer(*this);
}

void Seer::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | inspections: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << inspectionResults[i].playerId << ":" << roleName(inspectionResults[i].role) << "]";
    }
    output << '\n';
}

void Seer::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        world.reportAction(*this, "Seer prepares a secret role inspection.");
        int target = 0;
        for (int i = 0; i < world.playerCount(); ++i) {
            const Player* player = world.playerAt(i);
            if (!player->isAlive() || player->getId() == getId()) { continue; }
            bool known = false;
            for (int j = 0; j < count; ++j) {
                if (inspectionResults[j].playerId == player->getId()) { known = true; }
            }
            if (!known && (target == 0 || player->getId() < target)) { target = player->getId(); }
        }
        if (target != 0) { world.enqueueEvent(new RevealEvent(getId(), target)); }
    } else {
        world.reportAction(*this, "Seer consults inspection results for a day vote.");
        int knownWolf = 0;
        for (int i = 0; i < count; ++i) {
            const Player* player = world.findPlayer(inspectionResults[i].playerId);
            if (inspectionResults[i].wolf && player != nullptr && player->isAlive()
                && player->getId() != getId() && (knownWolf == 0 || player->getId() < knownWolf)) {
                knownWolf = player->getId();
            }
        }
        world.queueVote(getId(), knownWolf == 0 ? world.lowestLivingTarget(getId()) : knownWolf);
    }
}

int Seer::getInspectionCount() const { return count; }

InspectionResult Seer::getInspection(int index) const {
    requireIndex(index, count);
    return inspectionResults[index];
}

Witch::Witch(int id, const std::string& name)
    : VillageTeam(id, name), potionLog(nullptr), count(0) {
}

Witch::Witch(const Witch& other)
    : VillageTeam(other), potionLog(copyArray(other.potionLog, other.count)),
      count(other.count) {
}

Witch& Witch::operator=(const Witch& other) {
    if (this != &other) {
        PotionUse* copied = copyArray(other.potionLog, other.count);
        try {
            VillageTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] potionLog;
        potionLog = copied;
        count = other.count;
    }
    return *this;
}

Witch::~Witch() {
    delete[] potionLog;
}

RoleKind Witch::getRole() const { return WITCH_ROLE; }

Witch* Witch::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new Witch(*this);
}

void Witch::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | potions used: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << potionLog[i].targetId << ":" << (potionLog[i].healing ? "heal" : "poison") << "]";
    }
    output << '\n';
}

void Witch::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        std::string action = "Witch checks night potions: healing ";
        action += hasHealingPotion() ? "available" : "spent";
        action += ", poison ";
        action += hasPoisonPotion() ? "available." : "spent.";
        world.reportAction(*this, action);
        if (hasHealingPotion()) { world.enqueueEvent(new HealEvent(getId(), getId())); }
    } else {
        world.reportAction(*this, "Witch prepares a normal day vote and saves potions for night.");
        world.queueVote(getId(), world.lowestLivingTarget(getId()));
    }
}

int Witch::getPotionUseCount() const { return count; }

PotionUse Witch::getPotionUse(int index) const {
    requireIndex(index, count);
    return potionLog[index];
}

Hunter::Hunter(int id, const std::string& name)
    : VillageTeam(id, name), pastShots(nullptr), count(0) {
}

Hunter::Hunter(const Hunter& other)
    : VillageTeam(other), pastShots(copyArray(other.pastShots, other.count)),
      count(other.count) {
}

Hunter& Hunter::operator=(const Hunter& other) {
    if (this != &other) {
        int* copied = copyArray(other.pastShots, other.count);
        try {
            VillageTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] pastShots;
        pastShots = copied;
        count = other.count;
    }
    return *this;
}

Hunter::~Hunter() {
    delete[] pastShots;
}

RoleKind Hunter::getRole() const { return HUNTER_ROLE; }

Hunter* Hunter::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new Hunter(*this);
}

void Hunter::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | past shots: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << pastShots[i] << "]";
    }
    output << '\n';
}

void Hunter::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        world.reportAction(*this, "Hunter stands watch and prepares retaliation.");
    } else {
        world.reportAction(*this, "Hunter prepares a day vote and keeps a final shot ready.");
        world.queueVote(getId(), world.lowestLivingTarget(getId()));
    }
}

int Hunter::getShotCount() const { return count; }

int Hunter::getShot(int index) const {
    requireIndex(index, count);
    return pastShots[index];
}

Werewolf::Werewolf(int id, const std::string& name)
    : WolfTeam(id, name), pastTargets(nullptr), count(0) {
}

Werewolf::Werewolf(const Werewolf& other)
    : WolfTeam(other), pastTargets(copyArray(other.pastTargets, other.count)),
      count(other.count) {
}

Werewolf& Werewolf::operator=(const Werewolf& other) {
    if (this != &other) {
        int* copied = copyArray(other.pastTargets, other.count);
        try {
            WolfTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] pastTargets;
        pastTargets = copied;
        count = other.count;
    }
    return *this;
}

Werewolf::~Werewolf() {
    delete[] pastTargets;
}

RoleKind Werewolf::getRole() const { return WEREWOLF_ROLE; }

Werewolf* Werewolf::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new Werewolf(*this);
}

void Werewolf::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | past targets: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << pastTargets[i] << "]";
    }
    output << '\n';
}

void Werewolf::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        world.reportAction(*this, "Werewolf prepares to attack the pack's chosen victim.");
        int target = world.packVictim();
        if (target != 0) {
            recordTarget(target);
            world.enqueueEvent(new KillEvent(getId(), target));
        }
    } else {
        world.reportAction(*this, "Werewolf plans a day vote against the village.");
        world.queueVote(getId(), world.lowestLivingTarget(getId(), true));
    }
}

int Werewolf::getTargetCount() const { return count; }

int Werewolf::getTarget(int index) const {
    requireIndex(index, count);
    return pastTargets[index];
}

AlphaWolf::AlphaWolf(int id, const std::string& name)
    : WolfTeam(id, name), packMembers(nullptr), count(0) {
}

AlphaWolf::AlphaWolf(const AlphaWolf& other)
    : WolfTeam(other), packMembers(copyArray(other.packMembers, other.count)),
      count(other.count) {
}

AlphaWolf& AlphaWolf::operator=(const AlphaWolf& other) {
    if (this != &other) {
        int* copied = copyArray(other.packMembers, other.count);
        try {
            WolfTeam::operator=(other);
        } catch (...) {
            // Base assignment copies a string, which might fail to allocate.
            delete[] copied;
            throw;
        }
        delete[] packMembers;
        packMembers = copied;
        count = other.count;
    }
    return *this;
}

AlphaWolf::~AlphaWolf() {
    delete[] packMembers;
}

RoleKind AlphaWolf::getRole() const { return ALPHA_WOLF_ROLE; }

AlphaWolf* AlphaWolf::clone() const {
    // The copy constructor preserves the ID; duplication assigns a new ID later.
    return new AlphaWolf(*this);
}

void AlphaWolf::describe(std::ostream& output) const {
    describeCommon(output);
    output << " | pack members: " << count;
    for (int i = 0; i < count; ++i) {
        output << " [" << packMembers[i] << "]";
    }
    output << '\n';
}

void AlphaWolf::act(World& world) {
    if (!isAlive()) { return; }
    if (world.getPhase() == NIGHT) {
        world.reportAction(*this, "AlphaWolf coordinates the pack's night attack.");
        int memberCount = 0;
        for (int i = 0; i < world.playerCount(); ++i) {
            const Player* player = world.playerAt(i);
            if (player->isAlive() && player->isWolf() && player->getId() != getId()) { ++memberCount; }
        }
        int* members = memberCount == 0 ? nullptr : new int[memberCount];
        int position = 0;
        for (int i = 0; i < world.playerCount(); ++i) {
            const Player* player = world.playerAt(i);
            if (player->isAlive() && player->isWolf() && player->getId() != getId()) {
                members[position++] = player->getId();
            }
        }
        delete[] packMembers;
        packMembers = members;
        count = memberCount;
        int target = world.packVictim();
        if (target != 0) { world.enqueueEvent(new KillEvent(getId(), target)); }
    } else {
        world.reportAction(*this, "AlphaWolf prepares a double-weight day vote.");
        world.queueVote(getId(), world.lowestLivingTarget(getId(), true));
    }
}

int AlphaWolf::getPackMemberCount() const { return count; }

int AlphaWolf::getPackMember(int index) const {
    requireIndex(index, count);
    return packMembers[index];
}

void Villager::setSuspicion(int playerId, int score) {
    requirePlayerId(playerId);
    if (score < 0) {
        throw std::invalid_argument("Suspicion cannot be negative.");
    }
    for (int i = 0; i < count; ++i) {
        if (suspicions[i].playerId == playerId) {
            suspicions[i].score = score;
            return;
        }
    }
    SuspicionEntry entry = {playerId, score};
    appendToArray(suspicions, count, entry);
}

void Seer::recordInspection(int playerId, RoleKind role) {
    requirePlayerId(playerId);
    roleName(role); // Validate the role before storing it.
    InspectionResult result = {playerId, role, role == WEREWOLF_ROLE || role == ALPHA_WOLF_ROLE};
    appendToArray(inspectionResults, count, result);
}

int Seer::nightOrder() const { return 0; }

bool Seer::receiveInspection(int targetId, RoleKind role) {
    recordInspection(targetId, role);
    return true;
}

bool Witch::hasHealingPotion() const {
    for (int i = 0; i < count; ++i) {
        if (potionLog[i].healing) {
            return false;
        }
    }
    return true;
}

bool Witch::hasPoisonPotion() const {
    for (int i = 0; i < count; ++i) {
        if (!potionLog[i].healing) {
            return false;
        }
    }
    return true;
}

bool Witch::recordPotionUse(int targetId, bool healing) {
    requirePlayerId(targetId);
    if ((healing && !hasHealingPotion()) || (!healing && !hasPoisonPotion())) {
        return false;
    }
    PotionUse use = {targetId, healing};
    appendToArray(potionLog, count, use);
    return true;
}

int Witch::nightOrder() const { return 0; }

bool Witch::spendPotion(int targetId, bool healing) { return recordPotionUse(targetId, healing); }

void Hunter::recordShot(int targetId) {
    requirePlayerId(targetId);
    appendToArray(pastShots, count, targetId);
}

void Hunter::onDeath(World& world) {
    if (isAlive()) { return; }
    int target = world.lowestLivingTarget(getId());
    if (target != 0) {
        recordShot(target);
        world.enqueueEvent(new ShootEvent(getId(), target));
    }
}

void Werewolf::recordTarget(int targetId) {
    requirePlayerId(targetId);
    appendToArray(pastTargets, count, targetId);
}

void AlphaWolf::addPackMember(int playerId) {
    requirePlayerId(playerId);
    for (int i = 0; i < count; ++i) {
        if (packMembers[i] == playerId) {
            return;
        }
    }
    appendToArray(packMembers, count, playerId);
}

int AlphaWolf::voteWeight() const { return 2; }
