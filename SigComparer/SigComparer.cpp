#include <Windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <fstream>
#include <queue>

using namespace std;

const int MAX_FILE_COUNT = 2;

enum class flag_e {
    HELP,
    SIG_SIZE,
    MODE,
    ERR
};

flag_e get_flag(char* flag) {
    if (!strcmp(flag, "-h") || !strcmp(flag, "-help")) return flag_e::HELP;
    if (!strcmp(flag, "-s") || !strcmp(flag, "-size")) return flag_e::SIG_SIZE;
    if (!strcmp(flag, "-m") || !strcmp(flag, "-mode")) return flag_e::MODE;
    return flag_e::ERR;
}

int handle_flag(flag_e& flag, char** argv, int& i) {
    switch (flag) {
    case flag_e::HELP: {
        cout << "-s [Scan Size of each Sig (Default: 8 bytes)]"
            << endl << "-m [Mode {0 for normal, 1 for async} (Default: 0)]"
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
        throw std::runtime_error(std::string("ERROR: Unknown flag ") + argv[i]);
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
                        throw std::runtime_error("ERR: Signature size must be between 1-64 inclusive");

                    sig_size = flag_val;
                    break;
                }
                case flag_e::MODE: {
                    if (flag_val < 0 || flag_val > 1)
                        throw std::runtime_error("ERR: Mode must only be 0 or 1");

                    mode = flag_val;
                    break;
                }
                default:
                    break;
                }
            }
            catch (std::invalid_argument& e) {
                throw std::runtime_error(string("ERR: Failed to parse integer ") + argv[i]);
            }
            catch (std::out_of_range& e) {
                throw std::runtime_error(string("ERR: Failed to parse integer ") + argv[i]);
            }

            continue;
        }

        // if no flag then it is a file input
        if (file_count == MAX_FILE_COUNT)
            throw std::runtime_error("ERR: Max File Limit Exceeded (" + to_string(MAX_FILE_COUNT) + ')');

        files[file_count] = argv[i];
        file_count++;
    }
}

int sig_compare(char** files, int& mode, int& sig_size, int& total_sigs) {
    Sleep(5000);
    int matches = 0;

    int smallest_file = -1;
    uintmax_t smallest_sz = 0;

    for (int i = 0; i < MAX_FILE_COUNT; i++) {
        if (!filesystem::exists(files[i]))
            throw runtime_error(string("ERR: File does not exist ") + files[i]);

        uintmax_t cur_sz = filesystem::file_size(files[i]);

        if (smallest_file == -1 || cur_sz < smallest_sz) {
            smallest_file = i;
            smallest_sz = cur_sz;
        }
    }

    if (sig_size > smallest_sz)
        throw std::runtime_error("ERR: Signature size " + to_string(sig_size) + " is bigger than smallest file size (" + to_string(smallest_sz) + " bytes)");

    total_sigs = 1 + (smallest_sz - sig_size);
    cout << "total sigs: " << total_sigs << endl;

    switch (mode) {
    case 0: {
        unordered_map<uint32_t, bool> sigs;
        sigs.reserve(total_sigs);

        ifstream reader(files[smallest_file], ios_base::binary);
        
        uint8_t c;
        uint32_t hash;
        
        queue<uint8_t> hash_bytes;

        for (int i = 0; i < smallest_sz; i++) {
            reader >> c;

            hash_bytes.push(c);
            
            hash = (i == 0) ? c : (hash ^ c);
            hash = (hash << 1) | hash >> 31;

            if (hash_bytes.size() == sig_size) {
                sigs.insert({hash, false});

                /*
                cout << " added hash: " << hex << setfill('0') << setw(8) << (hash) << " | ";
                queue<uint8_t> copy_queue = hash_bytes;

                while (!copy_queue.empty()) {
                    cout << hex << setfill('0') << setw(2) << (int)copy_queue.front() << ' ';
                    copy_queue.pop();
                }

                cout << endl;
                */

                // remove first byte from hash and add new byte to hash
                hash = (hash << 32 - sig_size) | (hash >> sig_size);
                hash = hash ^ hash_bytes.front();
                hash = (hash << sig_size) | (hash >> 32 - sig_size);

                hash_bytes.pop();
            }
        }

        reader.close();
        hash_bytes = queue<uint8_t>();

        cout << sigs.size() << " unique sigs found in " << total_sigs << " total sigs" << endl;

        for (int i = 0; i < MAX_FILE_COUNT; i++) {
            if (i == smallest_file) continue;

            reader.open(files[i], ios_base::binary);

            if (!reader.is_open())
                throw runtime_error(string("ERR: Failed to open file ") + files[i]);

            for (int j = 0; j < filesystem::file_size(files[i]); j++) {
                reader >> c;

                hash_bytes.push(c);

                hash = (j == 0) ? c : (hash ^ c);
                hash = (hash << 1) | hash >> 31;

                if (hash_bytes.size() == sig_size) {
                    if (sigs.contains(hash) && !sigs[hash]) {
                        matches++;
                        sigs[hash] = true;
                    }

                    // remove first byte from hash and add new byte to hash
                    hash = (hash << 32 - sig_size) | (hash >> sig_size);
                    hash = hash ^ hash_bytes.front();
                    hash = (hash << sig_size) | (hash >> 32 - sig_size);

                    hash_bytes.pop();
                }
            }

            reader.close();
        }
        
        break;
    }
    case 1: { // async
        break;
    }
    default: {
        throw std::runtime_error("ERR: Unrecognized mode selected " + to_string(mode));
    }
    }

    return matches;
}

int main(int argc, char** argv)
{
    int sig_size = 8;
    int mode = 0;
    char* files[MAX_FILE_COUNT] = {nullptr, nullptr};

    auto end_program = []() {
        system("PAUSE");
        return 0;
    };

    try {
        handle_command_line(argc, argv, sig_size, mode, files);
    }
    catch (runtime_error& e) {
        cout << e.what() << endl;
        return end_program();
    }

    if (!files[0] || !files[1]) {
        cout << "ERR: You must enter " << MAX_FILE_COUNT << " file names for comparison" << endl;
        return end_program();
    }

    int total_sigs = 0;
    int matches = 0;

    try {
        matches = sig_compare(files, mode, sig_size, total_sigs);
        cout << "matches: " << matches << endl;
    }
    catch (std::runtime_error& e) {
        cout << e.what() << endl;
        return end_program();
    }

    float match_percent = (float(matches) / total_sigs) * 100.f;

    cout << "The " << MAX_FILE_COUNT << " are " << match_percent << setprecision(2) << "% simmilar. " << matches << " sigs found out of " << total_sigs << " sigs between the " << MAX_FILE_COUNT << " files." << endl;

    return end_program();
}