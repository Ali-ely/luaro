#ifndef PLAYER_H
#define PLAYER_H

#include <iosfwd>
#include <string>

class World;

// Role labels are stored in inspection records, not used to dispatch turns.
enum RoleKind {
    VILLAGER_ROLE, SEER_ROLE, WITCH_ROLE, HUNTER_ROLE,
    WEREWOLF_ROLE, ALPHA_WOLF_ROLE
};

const char* roleName(RoleKind role);

// Abstract owner of the state shared by every player. World will assign IDs.
class Player {
    friend class World; // Only World may assign a new identity to a duplicate.
private:
    int id;
    std::string name;
    bool alive;
    bool protectedFlag;

protected:
    Player(int id, const std::string& name);
    Player(const Player& other);
    // Protected assignment prevents assigning only the base part through Player*.
    Player& operator=(const Player& other);
    void describeCommon(std::ostream& output) const;

public:
    virtual ~Player();

    int getId() const;
    const std::string& getName() const;
    bool isAlive() const;
    bool isProtected() const;
    void setName(const std::string& newName);
    void setAlive(bool value);
    void setProtected(bool value);

    virtual const char* getTeamName() const = 0;
    virtual bool isWolf() const = 0;
    virtual RoleKind getRole() const = 0;
    virtual Player* clone() const = 0;
    virtual void describe(std::ostream& output) const = 0;
    virtual void act(World& world) = 0;
    virtual int nightOrder() const;
    virtual int voteWeight() const;
    virtual void onDeath(World& world);
    // Event delivery uses virtual hooks, without downcasting the recipient.
    virtual bool receiveInspection(int targetId, RoleKind role);
    virtual bool spendPotion(int targetId, bool healing);
};

#endif
