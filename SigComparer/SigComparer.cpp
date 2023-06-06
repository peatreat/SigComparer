#include "cmd_line.h"
#include "sig.h"

void progress_bar(atomic<float>* progress, atomic<bool>& progress_done) {
    const auto barWidth = 70;

    show_cursor(false);

    float prog = (progress[0] + progress[1]) * 0.5f;

    while (prog != 1.f) {
        this_thread::sleep_for(chrono::milliseconds(1));

        prog = (progress[0] + progress[1]) * 0.5f;

        if (!prog) continue;

        cout << "[";
        int pos = barWidth * prog;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) cout << "=";
            else if (i == pos) cout << ">";
            else cout << " ";
        }
        cout << "] " << int(prog * 100.f) << " %\r";

        cout.flush();
    }

    cout << std::string(barWidth, ' ') << "        \r";
    
    progress_done = true;
}

int main(int argc, char** argv)
{
    srand(time(0));

    atomic<float> progresses[MAX_FILE_COUNT] = {};

    try {
        flags_data_t input = handle_command_line(argc, argv);

        if (!input.files[0] || !input.files[1])
            throw runtime_error("ERR: You must enter " + to_string(MAX_FILE_COUNT) + " file names for comparison");

        atomic<bool> progress_done(false);

        if (input.mode == 0) {            
            thread pthread(progress_bar, progresses, ref(progress_done));
            pthread.detach();
        }

        compare_data_t result = sig_compare({ input.files, progresses }, input.mode, input.sig_size);

        while (input.mode == 0 && !progress_done)
            this_thread::sleep_for(chrono::milliseconds(100));

        show_cursor(true);

        float match_percent = (float(result.matches) / result.total_sigs) * 100.f;

        const char* bytes_sigs_str = (input.mode == 0 ? "bytes" : "sigs");
        cout << "The " << MAX_FILE_COUNT << " files are " << fixed << setprecision(2) << match_percent << "% similar. " << result.matches << " " << bytes_sigs_str << " found out of " << result.total_sigs << " " << bytes_sigs_str << " between the " << MAX_FILE_COUNT << " files." << endl;

        if (input.output) {
            string out_filename = "found_sigs_" + string(input.files[!result.biggest_file]) + ".txt";
            ofstream output(out_filename, ios::out);

            if (output.is_open()) {
                auto comp = [&](pair<size_t, pair<unordered_set<size_t>, uint32_t>>& a, pair<size_t, pair<unordered_set<size_t>, uint32_t>>& b)-> bool {
                    return input.sort < 2 ? (input.sort == 0 ? a.second.second > b.second.second : a.second.second < b.second.second) : (input.sort == 2 ? *a.second.first.begin() > *b.second.first.begin() : *a.second.first.begin() < *b.second.first.begin()); //or use paramA in some way
                };

                sort(result.sigs.begin(), result.sigs.end(), comp);

                for (auto& it : result.sigs) {
                    output << "Sig at offset 0x" << hex << setfill('0') << setw(8) << *it.second.first.begin() << " with a size of " << dec << it.second.second << " bytes" << endl;
                }

                cout << "Wrote " << result.sigs.size() << " sigs to " << out_filename << endl;

                output.close();
            }
            else {
                cout << "ERR: Failed to write sigs to file" << endl;
            }
        }
    }
    catch (runtime_error& e) {
        cout << e.what() << endl;
        return 0;
    }

    return 0;
}