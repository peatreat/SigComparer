#pragma once
#include "globals.h"

flag_e get_flag(char* flag) {
    if (!strcmp(flag, "-h") || !strcmp(flag, "-help")) return flag_e::HELP;
    if (!strcmp(flag, "-s") || !strcmp(flag, "-size")) return flag_e::SIG_SIZE;
    if (!strcmp(flag, "-m") || !strcmp(flag, "-mode")) return flag_e::MODE;
    return flag_e::ERR;
}

int handle_flag(flag_e& flag, char** argv, int& i) {
    switch (flag) {
    case flag_e::HELP: {
        cout << "-s [Scan Size of each Sig (Default: 16 bytes)]"
            << endl << "-m [Mode {0 for normal, 1 for async} (Default: 1)]"
            << endl << "Usage: sigcomparer.exe [-m 0:1] [-s 1-64] file1.ext file2.ext"
            << endl;
        break;
    }

    case flag_e::SIG_SIZE: {
        i++;
        return stoi(argv[i]);
    }

    case flag_e::MODE: {
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

void handle_command_line(int argc, char** argv, int& sig_size, int& mode, char** files) {
    int file_count = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            flag_e flag = get_flag(argv[i]);

            try {
                int flag_val = handle_flag(flag, argv, i);

                switch (flag) {
                case flag_e::SIG_SIZE: {
                    if (flag_val < 0 || flag_val > 64)
                        throw runtime_error("ERR: Signature size must be between 1-64 inclusive");

                    sig_size = flag_val;
                    break;
                }
                case flag_e::MODE: {
                    if (flag_val < 0 || flag_val > 1)
                        throw runtime_error("ERR: Mode must only be 0 or 1");

                    mode = flag_val;
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

        files[file_count] = argv[i];
        file_count++;
    }
}