#include "Event.h"
#include "World.h"

#include <ostream>
#include <stdexcept>

Event::Event(int sourceId, int targetId) : sourceId(sourceId), targetId(targetId) {
    if (sourceId <= 0 || targetId <= 0) {
        throw std::invalid_argument("Event source and target IDs must be positive.");
    }
}
Event::~Event() { }
int Event::getSourceId() const { return sourceId; }
int Event::getTargetId() const { return targetId; }
void Event::describePair(std::ostream& output, const char* name) const {
    output << name << " source " << sourceId << " -> target " << targetId << '\n';
}

KillEvent::KillEvent(int sourceId, int targetId, bool poison)
    : Event(sourceId, targetId), poison(poison) { }

void KillEvent::apply(World& world) {
    Player* source = world.findPlayer(getSourceId());
    Player* target = world.findPlayer(getTargetId());
    if (source == nullptr || target == nullptr || !target->isAlive()) {
        return;
    }
    if (poison && !source->spendPotion(getTargetId(), false)) {
        world.reportEvent("Poison ignored: no available poison potion.");
        return;
    }
    world.eliminate(getSourceId(), getTargetId(), poison ? "poison" : "attack", true);
}

void KillEvent::describe(std::ostream& output) const {
    describePair(output, poison ? "PoisonEvent" : "KillEvent");
}

HealEvent::HealEvent(int sourceId, int targetId) : Event(sourceId, targetId) { }
void HealEvent::apply(World& world) {
    Player* source = world.findPlayer(getSourceId());
    Player* target = world.findPlayer(getTargetId());
    if (source == nullptr || target == nullptr || !target->isAlive()) {
        return;
    }
    if (!source->spendPotion(getTargetId(), true)) {
        world.reportEvent("Heal ignored: no available healing potion.");
        return;
    }
    target->setProtected(true);
    world.reportEvent("Player " + std::to_string(getTargetId()) + " is protected.");
}
void HealEvent::describe(std::ostream& output) const { describePair(output, "HealEvent"); }

VoteEvent::VoteEvent(int sourceId, int targetId) : Event(sourceId, targetId) { }
void VoteEvent::apply(World& world) { world.recordVote(getSourceId(), getTargetId()); }
void VoteEvent::describe(std::ostream& output) const { describePair(output, "VoteEvent"); }

RevealEvent::RevealEvent(int sourceId, int targetId) : Event(sourceId, targetId) { }
void RevealEvent::apply(World& world) {
    Player* source = world.findPlayer(getSourceId());
    const Player* target = world.findPlayer(getTargetId());
    if (source == nullptr || target == nullptr || !target->isAlive()) {
        return;
    }
    if (source->receiveInspection(getTargetId(), target->getRole())) {
        world.reportEvent("Seer " + std::to_string(getSourceId()) + " learned that player "
                          + std::to_string(getTargetId()) + " is " + roleName(target->getRole()) + ".");
    }
}
void RevealEvent::describe(std::ostream& output) const { describePair(output, "RevealEvent"); }

ShootEvent::ShootEvent(int sourceId, int targetId) : Event(sourceId, targetId) { }
void ShootEvent::apply(World& world) {

    if (world.findPlayer(getSourceId()) != nullptr) {
        world.eliminate(getSourceId(), getTargetId(), "Hunter shot", true);
    }
}
void ShootEvent::describe(std::ostream& output) const { describePair(output, "ShootEvent"); }
