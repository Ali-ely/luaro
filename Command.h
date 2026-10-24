#ifndef COMMAND_H
#define COMMAND_H

#include "Player.h"

class World;

// One completed player edit; World owns and deletes commands in its undo stack.
class Command {
public:
    Command() = default;
    virtual ~Command();
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    virtual void undo(World& world) = 0;
};

// Undo creation by removing the new player; IDs are never recycled.
class CreateCmd : public Command {
private:
    int id;
public:
    explicit CreateCmd(int id);
    void undo(World& world) override;
};

// Undo duplication by removing only the newly assigned ID.
class DuplicateCmd : public Command {
private:
    int id;
public:
    explicit DuplicateCmd(int id);
    void undo(World& world) override;
};

// Owns an exact clone and its original position until restoration or destruction.
class DeleteCmd : public Command {
private:
    Player* snapshot;
    int position;
public:
    DeleteCmd(const Player& player, int position);
    ~DeleteCmd() override;
    void undo(World& world) override;
};

// Owns the entire pre-edit state, including concrete role data and identity.
class ModifyCmd : public Command {
private:
    Player* snapshot;
    int position;
public:
    ModifyCmd(const Player& player, int position);
    ~ModifyCmd() override;
    void undo(World& world) override;
};

#endif
