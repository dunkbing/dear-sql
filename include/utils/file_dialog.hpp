#pragma once

#include "database/db_interface.hpp"
#include <memory>
#include <string>

class DatabaseInterface;

class FileDialog {
public:
    FileDialog() = default;
    ~FileDialog() = default;

    // Initialize/cleanup NFD
    static bool initialize();
    static void cleanup();

    // File operations only
    std::shared_ptr<DatabaseInterface> openSQLiteFile();

private:
    static bool isInitialized;
};
