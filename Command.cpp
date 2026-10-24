#include "Command.h"
#include "World.h"

Command::~Command() { }

CreateCmd::CreateCmd(int id) : id(id) { }
void CreateCmd::undo(World& world) { world.eraseForUndo(id); }

DuplicateCmd::DuplicateCmd(int id) : id(id) { }
void DuplicateCmd::undo(World& world) { world.eraseForUndo(id); }

DeleteCmd::DeleteCmd(const Player& player, int position)
    : snapshot(player.clone()), position(position) { }
DeleteCmd::~DeleteCmd() { delete snapshot; }
void DeleteCmd::undo(World& world) {
    world.insertForUndo(snapshot, position);
    snapshot = nullptr; // Ownership transfers only after insertion succeeds.
}

ModifyCmd::ModifyCmd(const Player& player, int position)
    : snapshot(player.clone()), position(position) { }
ModifyCmd::~ModifyCmd() { delete snapshot; }
void ModifyCmd::undo(World& world) {
    world.replaceForUndo(snapshot, position);
    snapshot = nullptr;
}
