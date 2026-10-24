#ifndef CONSOLE_H
#define CONSOLE_H

#include "World.h"

#include <iosfwd>
#include <string>

// Borrows World and streams; line-based input keeps every prompt recoverable.
class Console {
private:
    World& world;
    std::istream& input;
    std::ostream& output;

    bool readInteger(const std::string& prompt, int minimum, int maximum, int& value);
    bool readName(std::string& name);
    bool readValues(std::string& name, bool& alive, bool& protection);
    bool createPlayer();
    bool managePlayer(int choice);
    bool addEvent();
    bool specialAction(int choice);
    bool perform(int choice);
    void showMenu() const;

public:
    Console(World& world, std::istream& input, std::ostream& output);
    void run();
};

#endif
