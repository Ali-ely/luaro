#ifndef EVENT_H
#define EVENT_H

#include <iosfwd>

class World;

enum EventKind { KILL_EVENT, HEAL_EVENT, VOTE_EVENT, REVEAL_EVENT, POISON_EVENT };

// An owned, polymorphic action containing IDs rather than borrowed player pointers.
class Event {
private:
    int sourceId;
    int targetId;
protected:
    Event(int sourceId, int targetId);
    void describePair(std::ostream& output, const char* name) const;
public:
    virtual ~Event();
    int getSourceId() const;
    int getTargetId() const;
    virtual void apply(World& world) = 0;
    virtual void describe(std::ostream& output) const = 0;
};

// Resolves a normal attack or consumes a Witch's poison before attacking.
class KillEvent : public Event {
private:
    bool poison;
public:
    KillEvent(int sourceId, int targetId, bool poison = false);
    void apply(World& world) override;
    void describe(std::ostream& output) const override;
};

// Consumes healing and protects a living target; it never resurrects a player.
class HealEvent : public Event {
public:
    HealEvent(int sourceId, int targetId);
    void apply(World& world) override;
    void describe(std::ostream& output) const override;
};

// Registers one weighted ballot for the end-of-day tally.
class VoteEvent : public Event {
public:
    VoteEvent(int sourceId, int targetId);
    void apply(World& world) override;
    void describe(std::ostream& output) const override;
};

// Delivers the inspected role to the source's virtual inspection handler.
class RevealEvent : public Event {
public:
    RevealEvent(int sourceId, int targetId);
    void apply(World& world) override;
    void describe(std::ostream& output) const override;
};

// A Hunter's committed retaliation can resolve after that Hunter has died.
class ShootEvent : public Event {
public:
    ShootEvent(int sourceId, int targetId);
    void apply(World& world) override;
    void describe(std::ostream& output) const override;
};

#endif
