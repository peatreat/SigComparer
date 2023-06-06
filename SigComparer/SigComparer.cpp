#include "cmd_line.h"
#include "sig.h"

int main(int argc, char** argv)
{
    int sig_size = 16;
    int mode = 1;
    char* files[MAX_FILE_COUNT] = {nullptr, nullptr};

    int total_sigs = 0;
    int matches = 0;

    try {
        handle_command_line(argc, argv, sig_size, mode, files);

        if (!files[0] || !files[1])
            throw runtime_error("ERR: You must enter " + to_string(MAX_FILE_COUNT) + " file names for comparison");

        matches = sig_compare(files, mode, sig_size, total_sigs);
    }
    catch (runtime_error& e) {
        cout << e.what() << endl;
        return 0;
    }

    float match_percent = (float(matches) / total_sigs) * 100.f;

    cout << "The " << MAX_FILE_COUNT << " files are " << fixed << setprecision(2) << match_percent << "% similar. " << matches << " sigs found out of " << total_sigs << " sigs between the " << MAX_FILE_COUNT << " files." << endl;

    return 0;
}