#ifndef STACK_H
#define STACK_H

#include <limits>
#include <stdexcept>

// A last-in, first-out container that owns a dynamic array of T values.
// Pointer values are borrowed: this class never deletes the pointed-to objects.
// T must support default construction, copying, and copy assignment.
template <typename T>
class Stack {
private:
    T* items;
    int count;
    int capacity;

    T* copyStorage(int newCapacity) const {
        T* copied = new T[newCapacity];
        try {
            for (int i = 0; i < count; ++i) {
                copied[i] = items[i];
            }
        } catch (...) {
            delete[] copied;
            throw;
        }
        return copied;
    }

    void grow() {
        if (capacity > std::numeric_limits<int>::max() / 2) {
            throw std::length_error("Stack cannot grow further.");
        }
        int newCapacity = capacity * 2;
        T* larger = copyStorage(newCapacity);
        delete[] items;
        items = larger;
        capacity = newCapacity;
    }

public:
    explicit Stack(int initialCapacity = 4)
        : items(nullptr), count(0), capacity(initialCapacity) {
        if (initialCapacity <= 0) {
            throw std::invalid_argument("Stack capacity must be positive.");
        }
        items = new T[capacity];
    }

    Stack(const Stack& other)
        : items(other.copyStorage(other.capacity)), count(other.count),
          capacity(other.capacity) {
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            T* copied = other.copyStorage(other.capacity);
            delete[] items;
            items = copied;
            count = other.count;
            capacity = other.capacity;
        }
        return *this;
    }

    ~Stack() {
        delete[] items;
    }

    int size() const { return count; }
    bool isEmpty() const { return count == 0; }

    void push(T value) {
        if (count == capacity) {
            grow();
        }
        items[count] = value;
        ++count;
    }

    bool peek(T& value) const {
        if (isEmpty()) {
            return false;
        }
        value = items[count - 1];
        return true;
    }

    bool pop(T& value) {
        if (isEmpty()) {
            return false;
        }
        value = items[count - 1];
        // Release resources held by a removed value, such as a string's storage.
        // For pointers this only clears the slot; the caller owns the pointee.
        items[count - 1] = T();
        --count;
        return true;
    }
};

#endif
