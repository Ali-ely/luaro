#ifndef TEAMS_H
#define TEAMS_H

#include "Player.h"

// Abstract middle level for all roles that win with the village.
class VillageTeam : public Player {
protected:
    VillageTeam(int id, const std::string& name);
    VillageTeam(const VillageTeam& other);
    VillageTeam& operator=(const VillageTeam& other);

public:
    const char* getTeamName() const override;
    bool isWolf() const override;
};

// Abstract middle level for wolves; shares team identity and night priority.
class WolfTeam : public Player {
protected:
    WolfTeam(int id, const std::string& name);
    WolfTeam(const WolfTeam& other);
    WolfTeam& operator=(const WolfTeam& other);

public:
    const char* getTeamName() const override;
    bool isWolf() const override;
    int nightOrder() const override;
};

#endif
