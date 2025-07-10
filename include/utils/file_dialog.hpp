#pragma once

#include <string>
#include <memory>

class Database;

class FileDialog
{
public:
    FileDialog() = default;
    ~FileDialog() = default;

    // Initialize/cleanup NFD
    static bool initialize();
    static void cleanup();

    // Database file selection
    std::shared_ptr<Database> openDatabaseDialog();

private:
    static bool isInitialized;
};
