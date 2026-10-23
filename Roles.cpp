#include "Roles.h"

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

void Villager::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Observe at night; vote using suspicion during the day.
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

void Seer::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Inspect at night; use inspection knowledge when voting during the day.
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

void Witch::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Use available potions at night; cast a normal vote during the day.
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

void Hunter::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Prepare retaliation at night; vote during the day.
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

void Werewolf::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Attack the pack victim at night; vote against the village during the day.
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

void AlphaWolf::act(World&) {
    // M3/M4 will connect this role's action to World and events.
    // Attack with the pack at night; cast a double-weight vote during the day.
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
    InspectionResult result = {playerId, role};
    appendToArray(inspectionResults, count, result);
}

int Seer::nightOrder() const { return 0; }

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

void Hunter::recordShot(int targetId) {
    requirePlayerId(targetId);
    appendToArray(pastShots, count, targetId);
}

void Hunter::onDeath(World&) {
    // M4 will enqueue one ShootEvent here; M1 only establishes the virtual hook.
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
