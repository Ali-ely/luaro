#include "Stack.h"
#include "Queue.h"

#include <iostream>
#include <string>

int main() {
    Stack<std::string> history(2);
    history.push("Create Alice");
    history.push("Modify Alice");
    history.push("Duplicate Alice");
    std::cout << "Stack removes the most recent item first:\n";
    std::string action;
    while (history.pop(action)) {
        std::cout << "  " << action << '\n';
    }
    if (!history.pop(action)) {
        std::cout << "  Empty stack: pop returned false.\n";
    }

    Queue<std::string> pending(2);
    pending.enqueue("Heal player 1");
    pending.enqueue("Inspect player 5");
    pending.enqueue("Attack player 1");
    std::cout << "\nView the queue without removing items:\n";
    for (int i = 0; i < pending.size(); ++i) {
        std::cout << "  " << pending.at(i) << '\n';
    }
    std::cout << "Items remaining after viewing: " << pending.size() << '\n';
    std::cout << "\nQueue removes the oldest item first:\n";
    while (pending.dequeue(action)) {
        std::cout << "  " << action << '\n';
    }
    if (!pending.peek(action)) {
        std::cout << "  Empty queue: peek returned false.\n";
    }
    std::cout << "\nThese are sample strings; game events arrive in M4.\n";
    return 0;
}
