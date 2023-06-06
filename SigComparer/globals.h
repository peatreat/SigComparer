#pragma once
#include <windows.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

const int MAX_FILE_COUNT = 2;

enum class flag_e {
    HELP,
    SIG_SIZE,
    MODE,
    SORT,
    OUTPUT,
    ERR
};

struct flags_data_t {
    char* files[MAX_FILE_COUNT];
    int sig_size;
    int mode;
    int output;
    int sort;
};

struct compare_data_t {
    int biggest_file;
    size_t matches;
    size_t total_sigs;
    vector<pair<size_t, pair<unordered_set<size_t>, uint32_t>>> sigs;
};

struct sig_data_t {
    unordered_map<size_t, pair<unordered_set<size_t>, uint32_t>> sigs;
    size_t bytes_found;
};