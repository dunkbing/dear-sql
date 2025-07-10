#include "tabs/tab.hpp"
#include "database/query_executor.hpp"
#include "database/database.hpp"
#include "application.hpp"
#include "imgui.h"
#include <cstring>
#include <iostream>

// Base Tab class
Tab::Tab(const std::string &name, TabType type)
    : name(name), type(type)
{
}

// SQLEditorTab implementation
SQLEditorTab::SQLEditorTab(const std::string &name)
    : Tab(name, TabType::SQL_EDITOR)
{
}

void SQLEditorTab::render()
{
    auto &app = Application::getInstance();

    ImGui::Text("SQL Editor");
    ImGui::Separator();

    // SQL input
    ImGui::InputTextMultiline("##SQL", sqlBuffer, sizeof(sqlBuffer),
                              ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.3f));
    sqlQuery = sqlBuffer;

    if (ImGui::Button("Execute Query"))
    {
        int selectedDb = app.getSelectedDatabase();
        auto &databases = app.getDatabases();

        if (selectedDb >= 0 && selectedDb < (int)databases.size())
        {
            auto &db = databases[selectedDb];
            if (db->connect())
            {
                queryResult = QueryExecutor::executeQuery(db->getConnection(), sqlQuery);
                strncpy(resultBuffer, queryResult.c_str(), sizeof(resultBuffer) - 1);
                resultBuffer[sizeof(resultBuffer) - 1] = '\0';
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        memset(sqlBuffer, 0, sizeof(sqlBuffer));
        sqlQuery.clear();
    }

    ImGui::Separator();
    ImGui::Text("Results:");

    // Results display
    ImGui::InputTextMultiline("##Results", resultBuffer, sizeof(resultBuffer),
                              ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
}

// TableViewerTab implementation
TableViewerTab::TableViewerTab(const std::string &name, const std::string &databasePath, const std::string &tableName)
    : Tab(name, TabType::TABLE_VIEWER), databasePath(databasePath), tableName(tableName)
{
    loadData();
}

void TableViewerTab::render()
{
    ImGui::Text("Table: %s", tableName.c_str());
    ImGui::Separator();

    // Pagination controls
    int totalPages = (totalRows + rowsPerPage - 1) / rowsPerPage;

    if (ImGui::Button("<<") && currentPage > 0)
    {
        firstPage();
    }
    ImGui::SameLine();

    if (ImGui::Button("<") && currentPage > 0)
    {
        previousPage();
    }
    ImGui::SameLine();

    ImGui::Text("Page %d of %d (%d rows total)", currentPage + 1, totalPages, totalRows);
    ImGui::SameLine();

    if (ImGui::Button(">") && currentPage < totalPages - 1)
    {
        nextPage();
    }
    ImGui::SameLine();

    if (ImGui::Button(">>") && currentPage < totalPages - 1)
    {
        lastPage();
    }

    ImGui::Separator();

    // Table display
    if (!columnNames.empty() && !tableData.empty())
    {
        if (ImGui::BeginTable("TableData", columnNames.size(),
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
        {
            // Headers
            for (const auto &colName : columnNames)
            {
                ImGui::TableSetupColumn(colName.c_str());
            }
            ImGui::TableHeadersRow();

            // Data rows
            for (const auto &row : tableData)
            {
                ImGui::TableNextRow();
                for (size_t i = 0; i < row.size() && i < columnNames.size(); i++)
                {
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", row[i].c_str());
                }
            }

            ImGui::EndTable();
        }
    }
    else
    {
        ImGui::Text("No data to display");
    }
}

void TableViewerTab::loadData()
{
    auto &app = Application::getInstance();
    auto &databases = app.getDatabases();

    // Find database
    Database *db = nullptr;
    for (auto &database : databases)
    {
        if (database->getPath() == databasePath && database->isConnected())
        {
            db = database.get();
            break;
        }
    }

    if (!db)
        return;

    // Get total row count
    totalRows = QueryExecutor::getRowCount(db->getConnection(), tableName);

    // Get column names
    columnNames = QueryExecutor::getColumnNames(db->getConnection(), tableName);

    // Get data with pagination
    int offset = currentPage * rowsPerPage;
    tableData = QueryExecutor::getTableData(db->getConnection(), tableName, rowsPerPage, offset);
}

void TableViewerTab::nextPage()
{
    int totalPages = (totalRows + rowsPerPage - 1) / rowsPerPage;
    if (currentPage < totalPages - 1)
    {
        currentPage++;
        loadData();
    }
}

void TableViewerTab::previousPage()
{
    if (currentPage > 0)
    {
        currentPage--;
        loadData();
    }
}

void TableViewerTab::firstPage()
{
    currentPage = 0;
    loadData();
}

void TableViewerTab::lastPage()
{
    int totalPages = (totalRows + rowsPerPage - 1) / rowsPerPage;
    currentPage = totalPages - 1;
    loadData();
}
