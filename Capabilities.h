#ifndef CAPABILITIES_H
#define CAPABILITIES_H

class World;

// Optional inspection action, selected explicitly by the administrator.
class Inspector {
public:
    virtual ~Inspector();
    virtual bool inspect(World& world, int targetId) = 0;
};

// Optional potion action; healing=true protects, false requests poison.
class PotionUser {
public:
    virtual ~PotionUser();
    virtual bool usePotion(World& world, int targetId, bool healing) = 0;
};

#endif
