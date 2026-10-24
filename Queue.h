#ifndef QUEUE_H
#define QUEUE_H

#include <limits>
#include <stdexcept>

// A first-in, first-out container using a circular dynamic array.
// It owns its array, but never deletes objects stored as pointer values.
// T must support default construction, copying, and copy assignment.
template <typename T>
class Queue {
private:
    T* items;
    int count;
    int capacity;
    int first;

    int indexAt(int offset) const {

        int remaining = capacity - first;
        if (offset < remaining) {
            return first + offset;
        }
        return offset - remaining;
    }

    T* copyStorage(int newCapacity) const {
        T* copied = new T[newCapacity];
        try {
            for (int i = 0; i < count; ++i) {
                copied[i] = items[indexAt(i)];
            }
        } catch (...) {
            delete[] copied;
            throw;
        }
        return copied;
    }

    void grow() {
        if (capacity > std::numeric_limits<int>::max() / 2) {
            throw std::length_error("Queue cannot grow further.");
        }
        int newCapacity = capacity * 2;
        T* larger = copyStorage(newCapacity);
        delete[] items;
        items = larger;
        capacity = newCapacity;
        first = 0;
    }

public:
    explicit Queue(int initialCapacity = 4)
        : items(nullptr), count(0), capacity(initialCapacity), first(0) {
        if (initialCapacity <= 0) {
            throw std::invalid_argument("Queue capacity must be positive.");
        }
        items = new T[capacity];
    }

    Queue(const Queue& other)
        : items(other.copyStorage(other.capacity)), count(other.count),
          capacity(other.capacity), first(0) {
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            T* copied = other.copyStorage(other.capacity);
            delete[] items;
            items = copied;
            count = other.count;
            capacity = other.capacity;
            first = 0;
        }
        return *this;
    }

    ~Queue() {
        delete[] items;
    }

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    void enqueue(T value) {

        if (count == capacity) {
            grow();
        }
        items[indexAt(count)] = value;
        ++count;
    }

    bool peek(T& value) const {
        if (isEmpty()) {
            return false;
        }
        value = items[first];
        return true;
    }

    bool dequeue(T& value) {
        if (isEmpty()) {
            return false;
        }
        value = items[first];
        items[first] = T();
        --count;
        if (count == 0 || first == capacity - 1) {
            first = 0;
        } else {
            ++first;
        }
        return true;
    }

    const T& at(int index) const {
        if (index < 0 || index >= count) {
            throw std::out_of_range("Queue index is out of range.");
        }
        return items[indexAt(index)];
    }
};

#endif
