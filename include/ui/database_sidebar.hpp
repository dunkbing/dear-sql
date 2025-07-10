#pragma once

#include "imgui.h"

class DatabaseSidebar
{
public:
    DatabaseSidebar() = default;
    ~DatabaseSidebar() = default;

    void render();

private:
    void renderDatabaseNode(size_t databaseIndex);
    void renderTableNode(size_t databaseIndex, size_t tableIndex);
    void handleDatabaseContextMenu(size_t databaseIndex);
    void handleTableContextMenu(size_t databaseIndex, size_t tableIndex);
};
