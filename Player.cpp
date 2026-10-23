#include "Player.h"

#include <ostream>
#include <stdexcept>

const char* roleName(RoleKind role) {
    switch (role) {
        case VILLAGER_ROLE: return "Villager";
        case SEER_ROLE: return "Seer";
        case WITCH_ROLE: return "Witch";
        case HUNTER_ROLE: return "Hunter";
        case WEREWOLF_ROLE: return "Werewolf";
        case ALPHA_WOLF_ROLE: return "AlphaWolf";
    }
    throw std::invalid_argument("Unknown role.");
}

Player::Player(int id, const std::string& name)
    : id(id), name(name), alive(true), protectedFlag(false) {
    if (id <= 0) {
        throw std::invalid_argument("Player IDs must be positive.");
    }
    setName(name);
}

Player::Player(const Player& other)
    : id(other.id), name(other.name), alive(other.alive),
      protectedFlag(other.protectedFlag) {
}

Player& Player::operator=(const Player& other) {
    if (this != &other) {
        // Copy the string first: if allocation fails, other fields stay unchanged.
        name = other.name;
        id = other.id;
        alive = other.alive;
        protectedFlag = other.protectedFlag;
    }
    return *this;
}

Player::~Player() {
}

int Player::getId() const { return id; }
const std::string& Player::getName() const { return name; }
bool Player::isAlive() const { return alive; }
bool Player::isProtected() const { return protectedFlag; }

void Player::setName(const std::string& newName) {
    if (newName.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("A player name cannot be blank.");
    }
    name = newName;
}

void Player::setAlive(bool value) {
    alive = value;
    if (!alive) {
        protectedFlag = false;
    }
}

void Player::setProtected(bool value) {
    if (value && !alive) {
        throw std::invalid_argument("A dead player cannot be protected.");
    }
    protectedFlag = value;
}

void Player::describeCommon(std::ostream& output) const {
    output << "ID " << id << " | " << name << " | " << roleName(getRole())
           << " | " << getTeamName() << " | " << (alive ? "alive" : "dead")
           << " | " << (protectedFlag ? "protected" : "unprotected");
}

int Player::nightOrder() const { return 2; }
int Player::voteWeight() const { return 1; }

void Player::onDeath(World&) {
    // Most players have no death reaction. Hunter overrides this hook.
}
