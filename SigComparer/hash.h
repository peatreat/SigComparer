#pragma once
#include <iostream>
#include <queue>
#include <iomanip>

class hasher {
public:
    hasher(int capacity) {
        this->hash = 0;
        this->capacity = capacity;
    }

    void add(uint8_t byte) {
        if (bytes.size() == capacity)
            bytes.pop_front();

        bytes.push_back(byte);

        std::hash<string> hasher;
        this->hash = hasher(string(bytes.begin(), bytes.end()));
    }

    const size_t& get() {
        return hash;
    }

    const size_t size() {
        return bytes.size();
    }

    void print() {
        for (uint8_t& byte : bytes) {
            cout << hex << setfill('0') << setw(2) << int(byte) << " ";
        }
        cout << endl;
    }
private:
    size_t hash;
    int capacity;
    deque<uint8_t> bytes;
};