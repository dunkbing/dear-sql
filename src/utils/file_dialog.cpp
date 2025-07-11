#include "utils/file_dialog.hpp"
#include "database/sqlite.hpp"
#include <iostream>
#include <nfd.h>

bool FileDialog::isInitialized = false;

bool FileDialog::initialize() {
    if (!isInitialized) {
        NFD_Init();
        isInitialized = true;
    }
    return true;
}

void FileDialog::cleanup() {
    if (isInitialized) {
        NFD_Quit();
        isInitialized = false;
    }
}

std::shared_ptr<DatabaseInterface> FileDialog::openSQLiteFile() {
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[2] = {{"SQLite Database", "db,sqlite,sqlite3"}, {"All Files", "*"}};

    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, nullptr);
    if (result == NFD_OKAY) {
        std::string path(outPath);
        size_t lastSlash = path.find_last_of("/\\");
        std::string name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

        auto db = std::make_shared<SQLiteDatabase>(name, path);
        NFD_FreePath(outPath);
        return db;
    } else if (result == NFD_CANCEL) {
        return nullptr;
    } else {
        std::cerr << "File dialog error: " << NFD_GetError() << std::endl;
        return nullptr;
    }
}
