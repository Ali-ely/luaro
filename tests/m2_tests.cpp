#include "Stack.h"
#include "Queue.h"

#include <iostream>
#include <stdexcept>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

// Counts test outcomes without global mutable state.
class Checks {
private:
    int total;
    int failures;
public:
    Checks() : total(0), failures(0) {}
    void expect(bool passed, const char* message) {
        ++total;
        if (!passed) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }
    int getTotal() const { return total; }
    int getFailures() const { return failures; }
};

// A test value that can deliberately fail assignment. The budget is borrowed
// from a local variable, so the tests need no global counters or owned pointers.
class CopyProbe {
public:
    int value;
    int* budget;

    CopyProbe(int value = 0, int* budget = nullptr) : value(value), budget(budget) {}
    CopyProbe(const CopyProbe& other) : value(other.value), budget(other.budget) {}
    CopyProbe& operator=(const CopyProbe& other) {
        if (this != &other) {
            if (other.budget != nullptr) {
                if (*other.budget == 0) {
                    throw std::runtime_error("Deliberate test copy failure.");
                }
                --*other.budget;
            }
            value = other.value;
            budget = other.budget;
        }
        return *this;
    }
};

void testStackOrder(Checks& checks) {
    Stack<int> stack(1);
    int value = -99;
    checks.expect(stack.isEmpty() && stack.size() == 0, "New stack is empty");
    checks.expect(!stack.pop(value) && value == -99, "Empty pop preserves output");
    checks.expect(!stack.peek(value) && value == -99, "Empty stack peek preserves output");
    for (int i = 0; i < 100; ++i) {
        stack.push(i);
    }
    checks.expect(stack.size() == 100, "Stack grows repeatedly");
    const Stack<int>& view = stack;
    checks.expect(view.peek(value) && value == 99 && stack.size() == 100,
                  "Const stack peek does not remove an item");
    for (int i = 99; i >= 0; --i) {
        checks.expect(stack.pop(value) && value == i, "Stack uses last-in, first-out order");
    }
    checks.expect(stack.isEmpty() && !stack.pop(value), "Stack can be drained completely");
    stack.push(123);
    checks.expect(stack.pop(value) && value == 123, "Drained stack can be reused");
}

void testQueueOrderAndViewing(Checks& checks) {
    Queue<int> queue(3);
    int value = -99;
    checks.expect(queue.isEmpty() && queue.size() == 0, "New queue is empty");
    checks.expect(!queue.dequeue(value) && value == -99, "Empty dequeue preserves output");
    checks.expect(!queue.peek(value) && value == -99, "Empty queue peek preserves output");
    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    checks.expect(queue.dequeue(value) && value == 1, "Queue removes the oldest item");
    queue.enqueue(4);
    const Queue<int>& view = queue;
    for (int repetition = 0; repetition < 3; ++repetition) {
        checks.expect(view.at(0) == 2 && view.at(1) == 3 && view.at(2) == 4
                      && queue.size() == 3, "Viewing a wrapped queue changes nothing");
    }
    checks.expect(view.peek(value) && value == 2 && queue.size() == 3,
                  "Const queue peek does not remove an item");
    queue.enqueue(5);
    for (int i = 2; i <= 5; ++i) {
        checks.expect(queue.dequeue(value) && value == i, "Wrapped growth preserves FIFO order");
    }
    checks.expect(queue.isEmpty(), "Wrapped queue drains correctly");

    for (int i = 0; i < 100; ++i) {
        queue.enqueue(i);
    }
    for (int i = 0; i < 100; ++i) {
        checks.expect(queue.at(i) == i, "Large queue view stays in insertion order");
    }
    for (int i = 0; i < 100; ++i) {
        checks.expect(queue.dequeue(value) && value == i, "Queue grows repeatedly without reordering");
    }

    Queue<int> rotating(3);
    rotating.enqueue(0);
    rotating.enqueue(1);
    for (int i = 0; i < 300; ++i) {
        checks.expect(rotating.dequeue(value) && value == i, "Repeated queue wraparound is correct");
        rotating.enqueue(i + 2);
        checks.expect(rotating.size() == 2 && rotating.at(0) == i + 1
                      && rotating.at(1) == i + 2, "Wraparound preserves size and view");
    }
    checks.expect(rotating.dequeue(value) && value == 300, "Drain after repeated wraparound");
    checks.expect(rotating.dequeue(value) && value == 301 && rotating.isEmpty(),
                  "Final rotating queue item is correct");
    rotating.enqueue(456);
    checks.expect(rotating.dequeue(value) && value == 456, "Drained queue can be reused");
}

void testStackCopies(Checks& checks) {
    const std::string first(200, 'a');
    const std::string second(200, 'b');
    Stack<std::string> original(2);
    original.push(first);
    original.push(second);
    Stack<std::string> copy(original);
    Stack<std::string> assigned(1);
    assigned.push("old");
    checks.expect(&(assigned = original) == &assigned, "Stack assignment returns itself");
    original.push("new");
    std::string value;
    checks.expect(copy.pop(value) && value == second && original.size() == 3,
                  "Stack copy is independent when source grows");
    checks.expect(assigned.pop(value) && value == second && copy.size() == 1,
                  "Stack assignment uses independent storage");
    copy = copy;
    checks.expect(copy.peek(value) && value == first, "Stack handles populated self-assignment");
    Stack<std::string> empty;
    Stack<std::string> emptyCopy(empty);
    checks.expect(emptyCopy.isEmpty(), "Empty stack can be copied");
    assigned = empty;
    assigned = assigned;
    checks.expect(assigned.isEmpty(), "Empty stack assignment clears old items");

    Stack<std::string>* survivor = nullptr;
    {
        Stack<std::string> temporary(1);
        temporary.push(first);
        survivor = new Stack<std::string>(temporary);
        assigned = temporary;
    }
    checks.expect(survivor->pop(value) && value == first, "Stack copy survives source destruction");
    delete survivor;
    checks.expect(assigned.pop(value) && value == first, "Stack assignment survives source destruction");
    original = empty;
    checks.expect(original.isEmpty(), "A grown stack can be assigned empty");
}

void testQueueCopies(Checks& checks) {
    const std::string first(200, 'a');
    const std::string second(200, 'b');
    const std::string third(200, 'c');
    std::string value;
    Queue<std::string> original(3);
    original.enqueue("discard");
    original.enqueue(first);
    original.enqueue(second);
    original.dequeue(value);
    original.enqueue(third);
    Queue<std::string> copy(original);
    Queue<std::string> assigned(1);
    assigned.enqueue("old");
    checks.expect(&(assigned = original) == &assigned, "Queue assignment returns itself");
    original.enqueue("new");
    checks.expect(copy.at(0) == first && copy.at(1) == second && copy.at(2) == third,
                  "Copy constructor preserves wrapped logical order");
    checks.expect(assigned.at(0) == first && assigned.at(1) == second && assigned.at(2) == third,
                  "Assignment preserves wrapped logical order");
    checks.expect(copy.dequeue(value) && value == first && original.size() == 4,
                  "Queue copy is independent after source growth");
    checks.expect(assigned.dequeue(value) && value == first && copy.size() == 2,
                  "Queue assignment uses independent storage");
    copy = copy;
    checks.expect(copy.size() == 2 && copy.at(0) == second && copy.at(1) == third,
                  "Queue handles populated self-assignment");
    Queue<std::string> empty;
    Queue<std::string> emptyCopy(empty);
    checks.expect(emptyCopy.isEmpty(), "Empty queue can be copied");
    assigned = empty;
    assigned = assigned;
    checks.expect(assigned.isEmpty(), "Empty queue assignment clears old items");

    Queue<std::string>* survivor = nullptr;
    {
        Queue<std::string> temporary(1);
        temporary.enqueue(first);
        survivor = new Queue<std::string>(temporary);
        assigned = temporary;
    }
    checks.expect(survivor->dequeue(value) && value == first, "Queue copy survives source destruction");
    delete survivor;
    checks.expect(assigned.dequeue(value) && value == first, "Queue assignment survives source destruction");

    Queue<std::string> alias(1);
    alias.enqueue(first);
    alias.enqueue(alias.at(0));
    checks.expect(alias.size() == 2 && alias.at(0) == first && alias.at(1) == first,
                  "Enqueueing a viewed item is safe across growth");
    original = empty;
    checks.expect(original.isEmpty(), "A grown queue can be assigned empty");
}

void testInvalidInputsAndPointers(Checks& checks) {
    for (int capacity = -1; capacity <= 0; ++capacity) {
        bool rejected = false;
        try {
            Stack<int> invalid(capacity);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        checks.expect(rejected, "Stack rejects nonpositive capacity");
        rejected = false;
        try {
            Queue<int> invalid(capacity);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        checks.expect(rejected, "Queue rejects nonpositive capacity");
    }

    Queue<int> queue;
    const int invalidIndices[] = {-1, 0, 1};
    for (int i = 0; i < 3; ++i) {
        bool rejected = false;
        try {
            queue.at(invalidIndices[i]);
        } catch (const std::out_of_range&) {
            rejected = true;
        }
        checks.expect(rejected, "Empty queue rejects invalid view index");
    }
    queue.enqueue(10);
    bool rejected = false;
    try {
        queue.at(queue.size());
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    checks.expect(rejected && queue.at(0) == 10, "Past-end view leaves queue unchanged");

    int ownedElsewhere = 7;
    {
        Stack<int*> stack(1);
        Queue<int*> pointers(1);
        stack.push(&ownedElsewhere);
        pointers.enqueue(&ownedElsewhere);
        pointers.enqueue(nullptr);
        Stack<int*> stackCopy(stack);
        Queue<int*> queueCopy(pointers);
        int* result = nullptr;
        checks.expect(stack.pop(result) && result == &ownedElsewhere, "Stack transfers pointer value");
        checks.expect(pointers.dequeue(result) && result == &ownedElsewhere,
                      "Queue transfers pointer value");
        checks.expect(pointers.dequeue(result) && result == nullptr,
                      "Null pointer is a valid queued item");
        checks.expect(stackCopy.peek(result) && result == &ownedElsewhere
                      && queueCopy.at(0) == &ownedElsewhere, "Pointer copies retain their identity");

    }
    checks.expect(ownedElsewhere == 7, "Containers do not delete borrowed pointer targets");
}

void testStackCopyFailures(Checks& checks) {
    int budget = 100;
    Stack<CopyProbe> source(2);
    source.push(CopyProbe(10, &budget));
    source.push(CopyProbe(20, &budget));
    bool rejected = false;
    budget = 1;
    try {
        Stack<CopyProbe> failed(source);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 2, "Stack: failed copy construction preserves source");

    Stack<CopyProbe> destination(1);
    destination.push(CopyProbe(99));
    budget = 1;
    rejected = false;
    try {
        destination = source;
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CopyProbe result;
    checks.expect(rejected && destination.size() == 1 && destination.peek(result)
                  && result.value == 99, "Stack: failed assignment preserves destination");

    budget = 1;
    rejected = false;
    try {
        source.push(CopyProbe(30, &budget));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 2, "Stack: failed growth preserves items");
    budget = 100;
    checks.expect(source.peek(result) && result.value == 20,
                  "Stack: failed growth preserves order");
    source.push(CopyProbe(30, &budget));

    budget = 0;
    rejected = false;
    try {
        source.push(CopyProbe(40, &budget));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 3, "Stack: failed insertion does not change size");
    rejected = false;
    try {
        source.pop(result);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 3, "Stack: failed output copy does not remove item");
    budget = 100;
    const int expected[] = {30, 20, 10};
    for (int i = 0; i < 3; ++i) {
        checks.expect(source.pop(result) && result.value == expected[i],
                      "Stack: container remains usable after copy failures");
    }
}

void testQueueCopyFailures(Checks& checks) {
    int budget = 100;
    Queue<CopyProbe> source(2);
    source.enqueue(CopyProbe(10, &budget));
    source.enqueue(CopyProbe(20, &budget));
    bool rejected = false;
    budget = 1;
    try {
        Queue<CopyProbe> failed(source);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 2, "Queue: failed copy construction preserves source");

    Queue<CopyProbe> destination(1);
    destination.enqueue(CopyProbe(99));
    budget = 1;
    rejected = false;
    try {
        destination = source;
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    CopyProbe result;
    checks.expect(rejected && destination.size() == 1 && destination.peek(result)
                  && result.value == 99, "Queue: failed assignment preserves destination");

    budget = 1;
    rejected = false;
    try {
        source.enqueue(CopyProbe(30, &budget));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 2, "Queue: failed growth preserves items");
    budget = 100;
    checks.expect(source.peek(result) && result.value == 10,
                  "Queue: failed growth preserves order");
    source.enqueue(CopyProbe(30, &budget));

    budget = 0;
    rejected = false;
    try {
        source.enqueue(CopyProbe(40, &budget));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 3, "Queue: failed insertion does not change size");
    rejected = false;
    try {
        source.dequeue(result);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    checks.expect(rejected && source.size() == 3, "Queue: failed output copy does not remove item");
    budget = 100;
    const int expected[] = {10, 20, 30};
    for (int i = 0; i < 3; ++i) {
        checks.expect(source.dequeue(result) && result.value == expected[i],
                      "Queue: container remains usable after copy failures");
    }
}

void runTests(Checks& checks) {
    testStackOrder(checks);
    testQueueOrderAndViewing(checks);
    testStackCopies(checks);
    testQueueCopies(checks);
    testInvalidInputsAndPointers(checks);
    testStackCopyFailures(checks);
    testQueueCopyFailures(checks);
}

int main() {
    Checks checks;
    std::cout << "Running M2 Stack and Queue tests...\n";
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemState before;
    _CrtMemState after;
    _CrtMemState difference;
    _CrtMemCheckpoint(&before);
#endif

    runTests(checks);

#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtMemCheckpoint(&after);
    const bool leaked = _CrtMemDifference(&difference, &before, &after) != 0;
    checks.expect(!leaked, "No outstanding allocations, including failed copies");
    if (leaked) {
        _CrtMemDumpStatistics(&difference);
    }
#endif

    std::cout << checks.getTotal() << " checks, " << checks.getFailures() << " failures.\n";
    return checks.getFailures() == 0 ? 0 : 1;
}
