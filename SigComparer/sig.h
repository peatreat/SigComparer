#pragma once
#include "globals.h"
#include "hash.h"
#include "utils.h"

#include <future>
#include <string>
#include <fstream>

sig_data_t get_sigs(void* file, int sig_size, bool async, unordered_map<size_t, pair<unordered_set<size_t>, size_t>>* other = nullptr) {
    unordered_map<size_t, pair<unordered_set<size_t>, size_t>> sigs;

    char* filename = nullptr;    
    atomic<float>* progress = nullptr;

    if (async) {
        filename = static_cast<char*>(file);
    }
    else {
        const auto data = static_cast<pair<char*, atomic<float>&>*>(file);
        filename = data->first;
        progress = &data->second;
    }

    ifstream reader(filename, ios_base::in | ios_base::binary);

    if (!reader.is_open())
        throw runtime_error(string("ERR: Failed to open file ") + filename);

    uint8_t byte;

    hasher hash(sig_size);

    // get file size
    reader.seekg(0, reader.end);
    const auto end_pos = reader.tellg();
    reader.seekg(0, reader.beg);
    
    const auto beg_pos = reader.tellg();
    const auto sz = end_pos - beg_pos;

    unordered_set<size_t> start_hash_offsets;

    size_t start_hash_off = 0;
    size_t adjacent_hashes = 0;

    size_t bytes_found = 0;
    size_t skip_bytes = 0;

    for (size_t i = 0; i < sz; i++) {
        reader >> noskipws >> byte; // read 1 byte from file to variable "byte"

        hash.add(byte); // process byte with hashing object

        if (progress)
            *progress = (float)i / (sz - 1);

        if (skip_bytes) {
            skip_bytes--;
            continue;
        }

        if (hash.size() == sig_size) { // only start adding sigs after we have processed a minimum of sig_size bytes into the hash
            if (!other) {
                // if we are async then only add a 1 sig for each hash
                if (async && sigs.contains(hash.get()))
                    continue;

                // if no set of sigs was passed to compare with then generate a set of sigs for each hash
                auto& sig = sigs[hash.get()];
                sig.first.insert(i - (sig_size - 1));
                sig.second = sig_size;
            } else {
                if (other->contains(hash.get())) {
                    if (adjacent_hashes) {
                        /* 
                        *  cur hash exists and there are adjacent hashes behind this offset.
                        *  checking to see if this byte + adjacent bytes exists in "other" by checking if the "other" offsets for this hash contains
                        *  any of the offsets in "other" for the starting hash of these adjacent bytes
                        */
                        bool contained = false;
                        auto& cur_hash_offsets = other->at(hash.get()).first; // grab offsets in "other" for current hash

                        for (auto other_off = start_hash_offsets.begin(); other_off != start_hash_offsets.end();) {
                            /*
                            * other_off is going to be an offset in the file for the "other" sigs, which has the same bytes as the first sig_size bytes 
                            * from the start of this block of adjacent hashes.
                            * we are checking if there is an offset in the file for the "other" sigs that equals other_off + adjacent_hashes for the current hash.
                            * if we are able to check that then we know the current byte is adjacent to the previous ones within the other file.
                            * we do this by getting all the offsets in the other file for the current hash and checking each offset in start_hash_offsets if
                            * other_off + adjacent_hashes is contained in the current hash's offsets for the "other" file
                            */
                            if (!cur_hash_offsets.contains(*other_off + adjacent_hashes)) {
                                other_off = start_hash_offsets.erase(other_off);
                            }
                            else {
                                contained = true;
                                other_off++;
                            }
                        }

                        if (!contained) {
                            /*
                            *  it found out that the current hash is not adjacent to the previous ones.
                            *  it will now insert a sig for only the adjacent bytes previous to this one
                            */
                            sigs.insert({ start_hash_off, { { start_hash_off }, i - start_hash_off } });

                            bytes_found += i - start_hash_off;
                            skip_bytes += sig_size - 1; // skip enough bytes to get the hash of the first sig_size bytes after the previous adjacent block.
                            adjacent_hashes = 0; // reset adjacent_hashes so we know that the next hash found is the start of its own sig
                            continue;
                        }
                    }
                    else {
                        /*
                        *  current hash was found but it is the start of an adjacent block (if there are any more adjacent hashes futher on)
                        *  lets set our starting position (start_hash_off) of our new adjacent sig block
                        *  also going to grab all the offets in the "other" file that have the same hash as the current one and copy them to start_hash_offsets
                        */
                        start_hash_off = i - (sig_size - 1);
                        start_hash_offsets = other->at(hash.get()).first;
                    }

                    // increments adjacent_hashes when a hash is found
                    adjacent_hashes++;
                }
                else if (adjacent_hashes) {
                    /*
                    *  ran into a hash that is not in the other file. lets add the sig we have built up to this point to our map.
                    */
                    sigs.insert({ start_hash_off, { { start_hash_off }, i - start_hash_off } });

                    bytes_found += i - start_hash_off;
                    skip_bytes += sig_size - 1; // skip enough bytes to get the hash of the first sig_size bytes after the previous adjacent block.
                    adjacent_hashes = 0; // reset adjacent_hashes so we know that the next hash found is the start of its own sig
                }
            }
        }
    }

    if (other && adjacent_hashes) {
        /*
        *  this sig goes to the end of the file and was not handled in the for loop so we are going to handle it here
        */
        sigs.insert({ start_hash_off, { { start_hash_off }, sz - start_hash_off } });
        bytes_found += sz - start_hash_off;
    }

    reader.close();

    return { sigs, bytes_found };
}

compare_data_t sig_compare(pair<char**, atomic<float>*> files, int mode, int sig_size) {
    int biggest_file = -1;
    uintmax_t biggest_sz = 0;

    for (int i = 0; i < MAX_FILE_COUNT; i++) {
        if (!file_exists(files.first[i]))
            throw runtime_error(string("ERR: File does not exist ") + files.first[i]);

        uintmax_t cur_sz = get_file_size(files.first[i]);

        if (biggest_file == -1 || cur_sz > biggest_sz) {
            biggest_file = i;
            biggest_sz = cur_sz;
        }
    }

    if (sig_size > biggest_sz)
        throw runtime_error("ERR: Signature size " + to_string(sig_size) + " is bigger than biggest file size (" + to_string(biggest_sz) + " bytes)");

    switch (mode) {
    case 0: {
        pair<char*, atomic<float>&> big_file_data = { files.first[biggest_file], files.second[biggest_file] };
        pair<char*, atomic<float>&> small_file_data = { files.first[!biggest_file], files.second[!biggest_file] };

        auto big_sigs = get_sigs(&big_file_data, sig_size, false);
        auto small_sigs = get_sigs(&small_file_data, sig_size, false, &big_sigs.sigs);

        big_file_data.second = 1.f;
        small_file_data.second = 1.f;

        return { biggest_file, small_sigs.bytes_found, biggest_sz, vector<pair<size_t, pair<unordered_set<size_t>, size_t>>>(small_sigs.sigs.begin(), small_sigs.sigs.end()) };
    }
    case 1: { // async
        vector<future<sig_data_t>> futures;

        futures.push_back(async(launch::async, get_sigs, files.first[0], sig_size, true, nullptr));
        futures.push_back(async(launch::async, get_sigs, files.first[1], sig_size, true, nullptr));

        vector<sig_data_t> results;

        for (int i = 0; i < futures.size(); i++) {
            results.push_back(futures[i].get());
        }

        auto& big_sigs = results[biggest_file].sigs;
        auto& small_sigs = results[!biggest_file].sigs;
        
        vector<pair<size_t, pair<unordered_set<size_t>, size_t>>> sigs;

        for (auto it = big_sigs.begin(); it != big_sigs.end(); it++) {
            auto pos = small_sigs.find(it->first);
            if (pos != small_sigs.end()) {
                sigs.push_back({ it->first, (*pos).second });
            }
        }

        return { biggest_file, sigs.size(), big_sigs.size(), sigs };
    }
    default: {
        throw runtime_error("ERR: Unrecognized mode selected " + to_string(mode));
    }
    }

    return {};
}