#pragma once
#include "globals.h"
#include <string>
#include <array>

flag_e get_flag(char* flag) {
    if (!strcmp(flag, "-h") || !strcmp(flag, "-help")) return flag_e::HELP;
    if (!strcmp(flag, "-s") || !strcmp(flag, "-size")) return flag_e::SIG_SIZE;
    if (!strcmp(flag, "-m") || !strcmp(flag, "-mode")) return flag_e::MODE;
    if (!strcmp(flag, "-s") || !strcmp(flag, "-sort")) return flag_e::SORT;
    if (!strcmp(flag, "-o") || !strcmp(flag, "-out")) return flag_e::OUTPUT;
    return flag_e::ERR;
}

int handle_flag(flag_e& flag, char** argv, int& i) {
    switch (flag) {
    case flag_e::HELP: {
        cout << "-s [Scan Size of each Sig {1 to 64} (Default: 16 bytes)]"
            << endl << "-m [Mode {0: dynamic scan, 1: light scan} (Default: 0)]"
            << endl << "-out [Saves sigs to file {0: disabled, 1: enabled} (Default: 1)]"
            << endl << "-sort [Sorting of output file {0-1: size desc/asc, 2-3: offset desc/asc} (Default: 0)]"
            << endl << "Usage: sigcomparer.exe [-m 0:1] [-s 1-64] [-out 0:1] [-sort 0-3] file1.ext file2.ext"
            << endl;
        break;
    }

    case flag_e::SIG_SIZE:
    case flag_e::MODE:
    case flag_e::SORT:
    case flag_e::OUTPUT: {
        i++;
        return stoi(argv[i]);
    }

    case flag_e::ERR:
    default: {
        throw runtime_error(string("ERROR: Unknown flag ") + argv[i]);
    }
    }

    return 0;
}

flags_data_t handle_command_line(int argc, char** argv) {
    int file_count = 0;

    flags_data_t flags = { { nullptr, nullptr }, 16, 0, 1, 0 };

    auto handle_flag_num = [](int& set_value, int& flag_val, const array<int, 2>& range) {
        if (flag_val < range[0] || flag_val > range[1])
            throw runtime_error("ERR: Flag value out of bounds");

        set_value = flag_val;
    };

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            flag_e flag = get_flag(argv[i]);

            try {
                int flag_val = handle_flag(flag, argv, i);

                switch (flag) {
                case flag_e::SIG_SIZE: {
                    handle_flag_num(flags.sig_size, flag_val, { 1, 64 });
                    break;
                }
                case flag_e::MODE: {
                    handle_flag_num(flags.mode, flag_val, { 0, 1 });
                    break;
                }
                case flag_e::OUTPUT: {
                    handle_flag_num(flags.output, flag_val, { 0, 1 });
                    break;
                }
                case flag_e::SORT: {
                    handle_flag_num(flags.sort, flag_val, { 0, 3 });
                    break;
                }
                default:
                    break;
                }
            }
            catch (invalid_argument& e) {
                throw runtime_error(string("ERR: Failed to parse integer ") + argv[i]);
            }
            catch (out_of_range& e) {
                throw runtime_error(string("ERR: Failed to parse integer ") + argv[i]);
            }

            continue;
        }

        // if no flag then it is a file input
        if (file_count == MAX_FILE_COUNT)
            throw runtime_error("ERR: Max File Limit Exceeded (" + to_string(MAX_FILE_COUNT) + ')');

        flags.files[file_count] = argv[i];
        file_count++;
    }

    return flags;
}