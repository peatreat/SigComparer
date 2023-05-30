#include <iostream>
#include <string>
#include <filesystem>

using namespace std;

const int MAX_FILE_COUNT = 2;

enum class flag_e {
    HELP,
    SIG_SIZE,
    MODE,
    ERR
};

flag_e get_flag(char* flag) {
    if (flag == "-h" || flag == "-help") return flag_e::HELP;
    if (flag == "-s" || flag == "-size") return flag_e::SIG_SIZE;
    if (flag == "-m" || flag == "-mode") return flag_e::MODE;
    return flag_e::ERR;
}

int handle_flag(flag_e& flag, char** argv, int& i) {
    switch (flag) {
    case flag_e::HELP: {
        cout << "-s [Scan Size of each Sig (Default: 8 bytes)]"
            << endl << "-m [Mode {0 for normal, 1 for async} (Default: 0)]"
            << endl << "Usage: sigcomparer.exe [-m 0:1] [-s 1-64] file1.ext file2.ext";
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

    for (int i = 0; i < argc; i++) {
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

int sig_compare(char** files, int& mode, int& sig_size, int& matches, int& total_sigs) {
    char* smallest_file = nullptr;
    uintmax_t smallest_sz = 0;
    for (int i = 0; i < MAX_FILE_COUNT; i++) {
        uintmax_t cur_sz = filesystem::file_size(files[i]);

        if (!smallest_file || cur_sz < smallest_sz) {
            smallest_file = files[i];
            smallest_sz = cur_sz;
        }
    }

    if (sig_size > smallest_sz)
        throw std::runtime_error("ERR: Signature size " + to_string(sig_size) + " is bigger than smallest file size (" + to_string(smallest_sz) + " bytes)");

    total_sigs = 1 + (smallest_sz - sig_size);

    switch (mode) {
    case 0: { // normal
        break;
    }
    case 1: { // async
        break;
    }
    default: {
        throw std::runtime_error("ERR: Unrecognized mode selected " + to_string(mode));
    }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int sig_size = 8;
    int mode = 0;
    char* files[MAX_FILE_COUNT] = {nullptr, nullptr};

    auto end_program = []() {
        std::getchar();
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
        sig_compare(files, mode, sig_size, matches, total_sigs);
    }
    catch (std::runtime_error& e) {
        cout << e.what() << endl;
        return end_program();
    }

    float match_percent = (float(matches) / total_sigs) * 100.f;

    cout << "The " << MAX_FILE_COUNT << " are " << match_percent << setprecision(2) << "% simmilar. " << matches << " sigs found out of " << total_sigs << " sigs between the " << MAX_FILE_COUNT << " files." << endl;

    return end_program();
}