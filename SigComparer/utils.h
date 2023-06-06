#pragma once
#include <fstream>
#include <Windows.h>

inline bool file_exists(const string& name) {
    FILE* file = nullptr;
    if (!fopen_s(&file, name.c_str(), "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

const size_t get_file_size(char* filename) {
    ifstream reader(filename, ios::in | ios::binary);

    if (!reader.is_open()) return 0;

    reader.seekg(0, reader.end);
    const auto end_pos = reader.tellg();
    reader.seekg(0, reader.beg);
    const auto sz = end_pos - reader.tellg();

    reader.close();
    return sz;
}

void show_cursor(bool showFlag)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag; // set the cursor visibility
    SetConsoleCursorInfo(out, &cursorInfo);
}