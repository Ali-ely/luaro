#include "Teams.h"

VillageTeam::VillageTeam(int id, const std::string& name) : Player(id, name) {
}

VillageTeam::VillageTeam(const VillageTeam& other) : Player(other) {
}

VillageTeam& VillageTeam::operator=(const VillageTeam& other) {
    Player::operator=(other);
    return *this;
}

const char* VillageTeam::getTeamName() const { return "Village"; }
bool VillageTeam::isWolf() const { return false; }

WolfTeam::WolfTeam(int id, const std::string& name) : Player(id, name) {
}

WolfTeam::WolfTeam(const WolfTeam& other) : Player(other) {
}

WolfTeam& WolfTeam::operator=(const WolfTeam& other) {
    Player::operator=(other);
    return *this;
}

const char* WolfTeam::getTeamName() const { return "Wolves"; }
bool WolfTeam::isWolf() const { return true; }
int WolfTeam::nightOrder() const { return 1; }
