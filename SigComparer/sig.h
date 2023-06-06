#pragma once
#include "globals.h"

#include <future>
#include <filesystem>
#include <queue>
#include <fstream>
#include <vector>
#include <unordered_set>

unordered_set<uint32_t> get_sigs(char* file, int sig_size) {
    unordered_set<uint32_t> sigs;
    ifstream reader(file, ios_base::binary);

    if (!reader.is_open())
        throw runtime_error(string("ERR: Failed to open file ") + file);

    uint8_t c;
    uint32_t hash;

    queue<uint8_t> hash_bytes;

    // get file size
    reader.seekg(0, reader.end);
    const auto end_pos = reader.tellg();
    reader.seekg(0, reader.beg);
    const auto sz = end_pos - reader.tellg();

    for (int i = 0; i < sz; i++) {
        reader >> c; // read 1 byte from file to variable "c"

        hash_bytes.push(c); // keep track of last n bytes where n = sig_size

        // hashing: initialize with first byte value else xor hash with next byte
        hash = (i == 0) ? c : (hash ^ c);

        // shift hash 1 bit to the left
        hash = (hash << 1) | hash >> 31;

        if (hash_bytes.size() == sig_size) {
            // add hash to unordered set
            sigs.insert(hash);

            // remove first byte from hash and add new byte to hash
            hash = (hash << 32 - sig_size) | (hash >> sig_size);
            hash = hash ^ hash_bytes.front();
            hash = (hash << sig_size) | (hash >> 32 - sig_size);

            // get rid of the first byte, it's no longer needed
            hash_bytes.pop();
        }
    }

    reader.close();

    return sigs;
}

int sig_compare(char** files, int& mode, int& sig_size, int& total_sigs) {
    int matches = 0;

    int biggest_file = -1;
    uintmax_t biggest_sz = 0;

    for (int i = 0; i < MAX_FILE_COUNT; i++) {
        if (!filesystem::exists(files[i]))
            throw runtime_error(string("ERR: File does not exist ") + files[i]);

        uintmax_t cur_sz = filesystem::file_size(files[i]);

        if (biggest_file == -1 || cur_sz > biggest_sz) {
            biggest_file = i;
            biggest_sz = cur_sz;
        }
    }

    if (sig_size > biggest_sz)
        throw runtime_error("ERR: Signature size " + to_string(sig_size) + " is bigger than biggest file size (" + to_string(biggest_sz) + " bytes)");

    auto check_sigs = [&](unordered_set<uint32_t>& big_sigs, unordered_set<uint32_t>& small_sigs) {
        for (unordered_set<uint32_t>::iterator it = big_sigs.begin(); it != big_sigs.end(); it++) {
            if (small_sigs.contains(*it))
                matches++;
        }

        total_sigs = big_sigs.size();
    };

    switch (mode) {
    case 0: {
        unordered_set<uint32_t> big_sigs = get_sigs(files[biggest_file], sig_size);
        unordered_set<uint32_t> small_sigs = get_sigs(files[!biggest_file], sig_size);

        check_sigs(big_sigs, small_sigs);

        break;
    }
    case 1: { // async
        vector<future<unordered_set<uint32_t>>> futures;
        futures.push_back(async(launch::async, get_sigs, files[0], sig_size));
        futures.push_back(async(launch::async, get_sigs, files[1], sig_size));

        vector<unordered_set<uint32_t>> results;

        for (int i = 0; i < futures.size(); i++) {
            results.push_back(futures[i].get());
        }

        unordered_set<uint32_t>& big_sigs = results[biggest_file];
        unordered_set<uint32_t>& small_sigs = results[!biggest_file];

        check_sigs(big_sigs, small_sigs);

        break;
    }
    default: {
        throw runtime_error("ERR: Unrecognized mode selected " + to_string(mode));
    }
    }

    return matches;
}