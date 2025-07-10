#include "ui/database_sidebar.hpp"
#include "application.hpp"
#include "database/database.hpp"
#include "imgui.h"
#include "tabs/tab_manager.hpp"
#include "utils/file_dialog.hpp"
#include <iostream>

void DatabaseSidebar::render() {
    auto &app = Application::getInstance();

    ImGui::Begin("Databases");

    if (ImGui::Button("Open Database", ImVec2(-1, 0))) {
        auto db = app.getFileDialog()->openDatabaseDialog();
        if (db) {
            // Try to connect and load tables
            if (db->connect()) {
                db->refreshTables();
                std::cout << "Adding database to list. Tables loaded: " << db->getTables().size()
                          << std::endl;
                app.addDatabase(db);
            } else {
                std::cerr << "Failed to open database: " << db->getPath() << std::endl;
            }
        }
    }

    ImGui::Separator();

    auto &databases = app.getDatabases();
    for (size_t i = 0; i < databases.size(); i++) {
        renderDatabaseNode(i);
    }

    ImGui::End();
}

void DatabaseSidebar::renderDatabaseNode(size_t databaseIndex) {
    auto &app = Application::getInstance();
    auto &databases = app.getDatabases();
    auto &db = databases[databaseIndex];

    // Database node
    ImGuiTreeNodeFlags dbFlags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (app.getSelectedDatabase() == (int)databaseIndex) {
        dbFlags |= ImGuiTreeNodeFlags_Selected;
    }

    bool dbOpen = ImGui::TreeNodeEx(db->getName().c_str(), dbFlags);

    if (ImGui::IsItemClicked()) {
        app.setSelectedDatabase(databaseIndex);
        app.setSelectedTable(-1);
    }

    // Load tables when the tree node is opened (expanded) and tables haven't been loaded yet
    if (dbOpen && !db->areTablesLoaded()) {
        std::cout << "Database expanded and tables not loaded yet, attempting to load..."
                  << std::endl;
        if (!db->isConnected()) {
            std::cout << "Database not connected, attempting to connect..." << std::endl;
            db->connect();
        }
        if (db->isConnected()) {
            db->refreshTables();
        }
    }

    // Context menu for database
    handleDatabaseContextMenu(databaseIndex);

    if (dbOpen) {
        // Tables
        if (db->getTables().empty()) {
            ImGui::Text("  No tables found");
        } else {
            for (size_t j = 0; j < db->getTables().size(); j++) {
                renderTableNode(databaseIndex, j);
            }
        }
        ImGui::TreePop();
    }
}

void DatabaseSidebar::renderTableNode(size_t databaseIndex, size_t tableIndex) {
    auto &app = Application::getInstance();
    auto &databases = app.getDatabases();
    auto &db = databases[databaseIndex];
    auto &table = db->getTables()[tableIndex];

    ImGuiTreeNodeFlags tableFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (app.getSelectedDatabase() == (int)databaseIndex &&
        app.getSelectedTable() == (int)tableIndex) {
        tableFlags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::TreeNodeEx(table.name.c_str(), tableFlags);

    if (ImGui::IsItemClicked()) {
        app.setSelectedDatabase(databaseIndex);
        app.setSelectedTable(tableIndex);
    }

    // Double-click to open table viewer
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        app.getTabManager()->createTableViewerTab(db->getPath(), table.name);
    }

    // Context menu for table
    handleTableContextMenu(databaseIndex, tableIndex);
}

void DatabaseSidebar::handleDatabaseContextMenu(size_t databaseIndex) {
    auto &app = Application::getInstance();
    auto &databases = app.getDatabases();
    auto &db = databases[databaseIndex];

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Refresh")) {
            db->setTablesLoaded(false); // Reset flag to allow refresh
            db->refreshTables();
        }
        if (ImGui::MenuItem("New SQL Editor")) {
            app.getTabManager()->createSQLEditorTab();
        }
        if (ImGui::MenuItem("Disconnect")) {
            db->disconnect();
        }
        ImGui::EndPopup();
    }
}

void DatabaseSidebar::handleTableContextMenu(size_t databaseIndex, size_t tableIndex) {
    auto &app = Application::getInstance();
    auto &databases = app.getDatabases();
    auto &db = databases[databaseIndex];
    auto &table = db->getTables()[tableIndex];

    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("View Data")) {
            app.getTabManager()->createTableViewerTab(db->getPath(), table.name);
        }
        if (ImGui::MenuItem("Show Structure")) {
            // TODO: Show table structure in a tab
        }
        ImGui::EndPopup();
    }
}
