#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include "themes.hpp"
#include <imgui_internal.h>

static bool dark_theme = true;

enum class TabType
{
    SQL_EDITOR,
    TABLE_VIEWER
};

struct Column
{
    std::string name;
    std::string type;
    bool isPrimaryKey = false;
    bool isNotNull = false;
};

struct Table
{
    std::string name;
    std::vector<Column> columns;
    bool expanded = false;
};

struct Database
{
    std::string name;
    std::string path;
    sqlite3 *connection = nullptr;
    std::vector<Table> tables;
    bool expanded = false;
    bool connected = false;
    bool tablesLoaded = false; // Flag to prevent infinite refresh
};

struct Tab
{
    std::string name;
    TabType type;
    bool open = true;
    bool shouldFocus = false; // Flag to indicate this tab should be focused

    // SQL Editor specific
    std::string sqlQuery;
    std::string queryResult;

    // Table Viewer specific
    std::string databasePath;
    std::string tableName;
    std::vector<std::vector<std::string>> tableData;
    std::vector<std::string> columnNames;
    int currentPage = 0;
    int rowsPerPage = 100;
    int totalRows = 0;

    Tab(const std::string &tabName, TabType tabType) : name(tabName), type(tabType) {}
};

static std::vector<std::shared_ptr<Database>> databases;
static std::vector<std::shared_ptr<Tab>> tabs;
static int selectedDatabase = -1;
static int selectedTable = -1;
static char sqlBuffer[4096] = "";
static char queryResultBuffer[16384] = "";
static bool dockingLayoutInitialized = false;
static std::shared_ptr<Tab> activeTab = nullptr;

// SQLite helper functions
bool connectToDatabase(Database &db)
{
    if (db.connected && db.connection)
    {
        return true;
    }

    // std::cout << "Attempting to connect to database: " << db.path << std::endl;
    int rc = sqlite3_open(db.path.c_str(), &db.connection);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db.connection) << std::endl;
        return false;
    }

    std::cout << "Successfully connected to database: " << db.path << std::endl;
    db.connected = true;
    return true;
}

void disconnectDatabase(Database &db)
{
    if (db.connection)
    {
        sqlite3_close(db.connection);
        db.connection = nullptr;
    }
    db.connected = false;
}

std::vector<std::string> getTableNames(sqlite3 *db)
{
    std::vector<std::string> tables;
    const char *sql = "SELECT name FROM sqlite_master WHERE type IN ('table', 'view') ORDER BY name;";
    sqlite3_stmt *stmt;

    std::cout << "Executing query to get table names..." << std::endl;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *tableName = (const char *)sqlite3_column_text(stmt, 0);
            if (tableName)
            {
                std::cout << "Found table: " << tableName << std::endl;
                tables.push_back(std::string(tableName));
            }
        }
    }
    else
    {
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(db) << std::endl;
    }
    sqlite3_finalize(stmt);
    std::cout << "Query completed. Found " << tables.size() << " tables." << std::endl;
    return tables;
}

std::vector<Column> getTableColumns(sqlite3 *db, const std::string &tableName)
{
    std::vector<Column> columns;
    std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            Column col;
            col.name = (const char *)sqlite3_column_text(stmt, 1);
            col.type = (const char *)sqlite3_column_text(stmt, 2);
            col.isNotNull = sqlite3_column_int(stmt, 3) == 1;
            col.isPrimaryKey = sqlite3_column_int(stmt, 5) == 1;
            columns.push_back(col);
        }
    }
    sqlite3_finalize(stmt);
    return columns;
}

void refreshDatabaseTables(Database &db)
{
    std::cout << "Refreshing tables for database: " << db.name << std::endl;
    if (!connectToDatabase(db))
    {
        std::cout << "Failed to connect to database" << std::endl;
        db.tablesLoaded = true; // Mark as loaded even if failed to prevent retry
        return;
    }

    db.tables.clear();
    std::vector<std::string> tableNames = getTableNames(db.connection);
    std::cout << "Found " << tableNames.size() << " tables" << std::endl;

    for (const auto &tableName : tableNames)
    {
        std::cout << "Adding table: " << tableName << std::endl;
        Table table;
        table.name = tableName;
        table.columns = getTableColumns(db.connection, tableName);
        db.tables.push_back(table);
    }
    std::cout << "Finished refreshing tables. Total tables: " << db.tables.size() << std::endl;
    db.tablesLoaded = true; // Mark as loaded to prevent infinite refresh
}

std::string executeQuery(sqlite3 *db, const std::string &query)
{
    std::stringstream result;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
    {
        return "Error: " + std::string(sqlite3_errmsg(db));
    }

    // Get column count and names
    int columnCount = sqlite3_column_count(stmt);
    if (columnCount > 0)
    {
        // Headers
        for (int i = 0; i < columnCount; i++)
        {
            result << sqlite3_column_name(stmt, i);
            if (i < columnCount - 1)
                result << " | ";
        }
        result << "\n";

        // Separator
        for (int i = 0; i < columnCount; i++)
        {
            result << "----------";
            if (i < columnCount - 1)
                result << "-+-";
        }
        result << "\n";
    }

    // Data rows
    int rowCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && rowCount < 1000)
    { // Limit to 1000 rows
        for (int i = 0; i < columnCount; i++)
        {
            const char *text = (const char *)sqlite3_column_text(stmt, i);
            result << (text ? text : "NULL");
            if (i < columnCount - 1)
                result << " | ";
        }
        result << "\n";
        rowCount++;
    }

    if (rowCount == 0 && columnCount == 0)
    {
        result << "Query executed successfully. Rows affected: " << sqlite3_changes(db);
    }
    else if (rowCount == 1000)
    {
        result << "\n... (showing first 1000 rows)";
    }

    sqlite3_finalize(stmt);
    return result.str();
}

void loadTableData(Tab &tab)
{
    if (tab.type != TabType::TABLE_VIEWER)
        return;

    // Find database
    Database *db = nullptr;
    for (auto &database : databases)
    {
        if (database->path == tab.databasePath && database->connected)
        {
            db = database.get();
            break;
        }
    }

    if (!db)
        return;

    // Get total row count
    std::string countSql = "SELECT COUNT(*) FROM " + tab.tableName;
    sqlite3_stmt *countStmt;
    if (sqlite3_prepare_v2(db->connection, countSql.c_str(), -1, &countStmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step(countStmt) == SQLITE_ROW)
        {
            tab.totalRows = sqlite3_column_int(countStmt, 0);
        }
    }
    sqlite3_finalize(countStmt);

    // Get column names
    tab.columnNames.clear();
    std::string pragmaSql = "PRAGMA table_info(" + tab.tableName + ");";
    sqlite3_stmt *pragmaStmt;
    if (sqlite3_prepare_v2(db->connection, pragmaSql.c_str(), -1, &pragmaStmt, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(pragmaStmt) == SQLITE_ROW)
        {
            tab.columnNames.push_back((const char *)sqlite3_column_text(pragmaStmt, 1));
        }
    }
    sqlite3_finalize(pragmaStmt);

    // Get data with pagination
    int offset = tab.currentPage * tab.rowsPerPage;
    std::string dataSql = "SELECT * FROM " + tab.tableName + " LIMIT " +
                          std::to_string(tab.rowsPerPage) + " OFFSET " + std::to_string(offset);

    tab.tableData.clear();
    sqlite3_stmt *dataStmt;
    if (sqlite3_prepare_v2(db->connection, dataSql.c_str(), -1, &dataStmt, NULL) == SQLITE_OK)
    {
        int columnCount = sqlite3_column_count(dataStmt);
        while (sqlite3_step(dataStmt) == SQLITE_ROW)
        {
            std::vector<std::string> row;
            for (int i = 0; i < columnCount; i++)
            {
                const char *text = (const char *)sqlite3_column_text(dataStmt, i);
                row.push_back(text ? text : "NULL");
            }
            tab.tableData.push_back(row);
        }
    }
    sqlite3_finalize(dataStmt);
}

// Helper function to find existing table viewer tab
std::shared_ptr<Tab> findTableTab(const std::string &databasePath, const std::string &tableName)
{
    for (auto &tab : tabs)
    {
        if (tab->type == TabType::TABLE_VIEWER &&
            tab->databasePath == databasePath &&
            tab->tableName == tableName)
        {
            return tab;
        }
    }
    return nullptr;
}

// Helper function to open or focus table tab
void openTableTab(const std::string &databasePath, const std::string &tableName)
{
    // Check if tab already exists
    auto existingTab = findTableTab(databasePath, tableName);
    if (existingTab)
    {
        // Tab already exists, mark it to be focused
        existingTab->shouldFocus = true;
        std::cout << "Table " << tableName << " is already open, focusing existing tab" << std::endl;
        return;
    }

    // Create new tab
    auto tab = std::make_shared<Tab>(tableName, TabType::TABLE_VIEWER);
    tab->databasePath = databasePath;
    tab->tableName = tableName;
    tab->shouldFocus = true; // Mark new tab to be focused
    loadTableData(*tab);
    tabs.push_back(tab);
    std::cout << "Created new tab for table: " << tableName << std::endl;
}

void setupDefaultDockingLayout(ImGuiID dockspace_id)
{
    if (dockingLayoutInitialized)
        return;

    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    // Split the dockspace into left and right
    ImGuiID dock_left, dock_right;
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, &dock_left, &dock_right);

    // Dock windows to specific nodes
    ImGui::DockBuilderDockWindow("Databases", dock_left);
    ImGui::DockBuilderDockWindow("Content", dock_right);

    ImGui::DockBuilderFinish(dockspace_id);
    dockingLayoutInitialized = true;
}

void openDatabaseFileDialog()
{
    nfdchar_t *outPath;
    nfdfilteritem_t filterItem[2] = {{"SQLite Database", "db,sqlite,sqlite3"}, {"All Files", "*"}};

    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, NULL);
    if (result == NFD_OKAY)
    {
        auto db = std::make_shared<Database>();
        // Extract filename from path for display name
        std::string path(outPath);
        size_t lastSlash = path.find_last_of("/\\");
        db->name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        db->path = path;

        // Try to connect and load tables
        if (connectToDatabase(*db))
        {
            refreshDatabaseTables(*db);
            std::cout << "Adding database to list. Tables loaded: " << db->tables.size() << std::endl;
            databases.push_back(db);
        }
        else
        {
            std::cerr << "Failed to open database: " << path << std::endl;
        }

        // Free the path memory
        NFD_FreePath(outPath);
    }
    else if (result == NFD_CANCEL)
    {
        // User cancelled, do nothing
    }
    else
    {
        std::cerr << "File dialog error: " << NFD_GetError() << std::endl;
    }
}

void renderDatabaseSidebar()
{
    ImGui::Begin("Databases");

    if (ImGui::Button("Open Database", ImVec2(-1, 0)))
    {
        openDatabaseFileDialog();
    }

    ImGui::Separator();

    for (size_t i = 0; i < databases.size(); i++)
    {
        auto &db = databases[i];

        // Database node
        ImGuiTreeNodeFlags dbFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selectedDatabase == (int)i)
        {
            dbFlags |= ImGuiTreeNodeFlags_Selected;
        }

        bool dbOpen = ImGui::TreeNodeEx(db->name.c_str(), dbFlags);

        if (ImGui::IsItemClicked())
        {
            selectedDatabase = i;
            selectedTable = -1;
        }

        // Load tables when the tree node is opened (expanded) and tables haven't been loaded yet
        if (dbOpen && !db->tablesLoaded)
        {
            std::cout << "Database expanded and tables not loaded yet, attempting to load..." << std::endl;
            if (!db->connected)
            {
                std::cout << "Database not connected, attempting to connect..." << std::endl;
                connectToDatabase(*db);
            }
            if (db->connected)
            {
                refreshDatabaseTables(*db);
            }
        }

        // Context menu for database
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Refresh"))
            {
                db->tablesLoaded = false; // Reset flag to allow refresh
                refreshDatabaseTables(*db);
            }
            if (ImGui::MenuItem("New SQL Editor"))
            {
                auto tab = std::make_shared<Tab>("SQL Editor " + std::to_string(tabs.size() + 1), TabType::SQL_EDITOR);
                tabs.push_back(tab);
            }
            if (ImGui::MenuItem("Disconnect"))
            {
                disconnectDatabase(*db);
            }
            ImGui::EndPopup();
        }

        if (dbOpen)
        {
            // Tables
            if (db->tables.empty())
            {
                ImGui::Text("  No tables found");
            }
            else
            {
                for (size_t j = 0; j < db->tables.size(); j++)
                {
                    auto &table = db->tables[j];

                    ImGuiTreeNodeFlags tableFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if (selectedDatabase == (int)i && selectedTable == (int)j)
                    {
                        tableFlags |= ImGuiTreeNodeFlags_Selected;
                    }

                    ImGui::TreeNodeEx(table.name.c_str(), tableFlags);

                    if (ImGui::IsItemClicked())
                    {
                        selectedDatabase = i;
                        selectedTable = j;
                    }

                    // Double-click to open table viewer
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    {
                        openTableTab(db->path, table.name);
                    }

                    // Context menu for table
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("View Data"))
                        {
                            openTableTab(db->path, table.name);
                        }
                        if (ImGui::MenuItem("Show Structure"))
                        {
                            // TODO: Show table structure in a tab
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void renderSQLEditor(Tab &tab)
{
    ImGui::Text("SQL Editor");
    ImGui::Separator();

    // SQL input
    ImGui::InputTextMultiline("##SQL", sqlBuffer, sizeof(sqlBuffer),
                              ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.3f));
    tab.sqlQuery = sqlBuffer;

    if (ImGui::Button("Execute Query"))
    {
        if (selectedDatabase >= 0 && selectedDatabase < (int)databases.size())
        {
            auto &db = databases[selectedDatabase];
            if (connectToDatabase(*db))
            {
                tab.queryResult = executeQuery(db->connection, tab.sqlQuery);
                strncpy(queryResultBuffer, tab.queryResult.c_str(), sizeof(queryResultBuffer) - 1);
                queryResultBuffer[sizeof(queryResultBuffer) - 1] = '\0';
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        memset(sqlBuffer, 0, sizeof(sqlBuffer));
        tab.sqlQuery.clear();
    }

    ImGui::Separator();
    ImGui::Text("Results:");

    // Results display
    ImGui::InputTextMultiline("##Results", queryResultBuffer, sizeof(queryResultBuffer),
                              ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
}

void renderTableViewer(Tab &tab)
{
    ImGui::Text("Table: %s", tab.tableName.c_str());
    ImGui::Separator();

    // Pagination controls
    int totalPages = (tab.totalRows + tab.rowsPerPage - 1) / tab.rowsPerPage;

    if (ImGui::Button("<<") && tab.currentPage > 0)
    {
        tab.currentPage = 0;
        loadTableData(tab);
    }
    ImGui::SameLine();

    if (ImGui::Button("<") && tab.currentPage > 0)
    {
        tab.currentPage--;
        loadTableData(tab);
    }
    ImGui::SameLine();

    ImGui::Text("Page %d of %d (%d rows total)", tab.currentPage + 1, totalPages, tab.totalRows);
    ImGui::SameLine();

    if (ImGui::Button(">") && tab.currentPage < totalPages - 1)
    {
        tab.currentPage++;
        loadTableData(tab);
    }
    ImGui::SameLine();

    if (ImGui::Button(">>") && tab.currentPage < totalPages - 1)
    {
        tab.currentPage = totalPages - 1;
        loadTableData(tab);
    }

    ImGui::Separator();

    // Table display
    if (!tab.columnNames.empty() && !tab.tableData.empty())
    {
        if (ImGui::BeginTable("TableData", tab.columnNames.size(),
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY))
        {

            // Headers
            for (const auto &colName : tab.columnNames)
            {
                ImGui::TableSetupColumn(colName.c_str());
            }
            ImGui::TableHeadersRow();

            // Data rows
            for (const auto &row : tab.tableData)
            {
                ImGui::TableNextRow();
                for (size_t i = 0; i < row.size() && i < tab.columnNames.size(); i++)
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

void ToggleButton(const char *str_id, bool *v)
{
    ImVec4 *colors = ImGui::GetStyle().Colors;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
        *v = !*v;
    ImGuiContext &gg = *GImGui;
    float ANIM_SPEED = 0.085f;
    if (gg.LastActiveId == gg.CurrentWindow->GetID(str_id))
        float t_anim = ImSaturate(gg.LastActiveIdTimer / ANIM_SPEED);
    if (ImGui::IsItemHovered())
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height),
                                 ImGui::GetColorU32(*v ? colors[ImGuiCol_ButtonActive] : ImVec4(0.78f, 0.78f, 0.78f, 1.0f)), height * 0.5f);
    else
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height),
                                 ImGui::GetColorU32(*v ? colors[ImGuiCol_Button] : ImVec4(0.85f, 0.85f, 0.85f, 1.0f)), height * 0.50f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius),
                               radius - 1.5f, IM_COL32(255, 255, 255, 255));
}

int main()
{
    std::cout << "Starting Database Client..." << std::endl;

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Database Client", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::cout << "GLFW window created successfully" << std::endl;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Configure fonts with Japanese support
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false;

    // Try to load fonts from assets folder first, then fallback to system fonts
    std::vector<std::string> fontPaths =  {
// System fonts as fallback
#ifdef __APPLE__
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial Unicode MS.ttf"
#elif defined(_WIN32)
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/arial.ttf"
#else
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf"
#endif
    };

    bool fontLoaded = false;

    // Try to load the first available font
    for (const auto &fontPath : fontPaths)
    {
        std::ifstream fontFile(fontPath);
        if (fontFile.good())
        {
            fontFile.close();

            // Load the font with comprehensive Unicode ranges
            const ImWchar *ranges = nullptr;

            // Choose appropriate glyph ranges based on font name
            if (fontPath.find("CJK") != std::string::npos || fontPath.find("Han") != std::string::npos)
            {
                ranges = io.Fonts->GetGlyphRangesJapanese(); // Full CJK support
            }
            else if (fontPath.find("Cyrillic") != std::string::npos || fontPath.find("NotoSans-Regular") != std::string::npos)
            {
                ranges = io.Fonts->GetGlyphRangesCyrillic(); // Cyrillic + Latin
            }
            else if (fontPath.find("unifont") != std::string::npos || fontPath.find("Arabic") != std::string::npos)
            {
                ranges = nullptr; // Load all available glyphs for maximum coverage
            }
            else
            {
                // For general fonts, use Japanese ranges for broad coverage (includes Latin + CJK)
                ranges = io.Fonts->GetGlyphRangesJapanese();
            }

            ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &fontConfig, ranges);

            if (font != nullptr)
            {
                if (fontPath.find("assets/fonts/") == 0)
                {
                    std::cout << "✓ Successfully loaded custom font: " << fontPath << std::endl;
                }
                else
                {
                    std::cout << "✓ Successfully loaded system font: " << fontPath << std::endl;
                }
                fontLoaded = true;
                break;
            }
        }
    }

    // Fallback: Use default font with Japanese ranges
    if (!fontLoaded)
    {
        std::cout << "⚠ No custom or system fonts found, using default font with Japanese ranges" << std::endl;
        fontConfig.MergeMode = false;
        io.Fonts->AddFontDefault(&fontConfig);

        // Add Japanese characters to the default font
        ImFontConfig japaneseConfig;
        japaneseConfig.MergeMode = true;
        japaneseConfig.GlyphOffset = ImVec2(0.0f, 0.0f);

        // Try to find any available font and merge Japanese characters
        for (const auto &fontPath : fontPaths)
        {
            std::ifstream fontFile(fontPath);
            if (fontFile.good())
            {
                fontFile.close();
                // Use appropriate ranges for the fallback font too
                const ImWchar *fallbackRanges = io.Fonts->GetGlyphRangesJapanese(); // Good default for broad coverage
                io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &japaneseConfig, fallbackRanges);
                if (fontPath.find("assets/fonts/") == 0)
                {
                    std::cout << "✓ Merged Japanese characters from custom font: " << fontPath << std::endl;
                }
                else
                {
                    std::cout << "✓ Merged Japanese characters from system font: " << fontPath << std::endl;
                }
                break;
            }
        }
    }

    // Build the font atlas
    io.Fonts->Build();

    ImGui::StyleColorsDark();
    Theme::ApplyNativeTheme(dark_theme ? Theme::NATIVE_DARK : Theme::NATIVE_LIGHT);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::cout << "ImGui initialized" << std::endl;

    // Initialize Native File Dialog
    NFD_Init();

    glClearColor(
        dark_theme ? 0.110f : 0.957f,
        dark_theme ? 0.110f : 0.957f,
        dark_theme ? 0.137f : 0.957f,
        0.98f);

    // No sample database - users can open their own databases

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DockSpace setup
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("DockSpace Demo", nullptr, window_flags);

        // Menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open Database", "Ctrl+O"))
                {
                    openDatabaseFileDialog();
                }
                if (ImGui::MenuItem("New SQL Editor", "Ctrl+N"))
                {
                    auto tab = std::make_shared<Tab>("SQL Editor " + std::to_string(tabs.size() + 1), TabType::SQL_EDITOR);
                    tab->shouldFocus = true; // Focus the new SQL editor tab
                    tabs.push_back(tab);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    glfwSetWindowShouldClose(window, true);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Refresh All"))
                {
                    for (auto &db : databases)
                    {
                        if (db->connected)
                        {
                            refreshDatabaseTables(*db);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            // Push theme toggle to the right side of the menu bar
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 100);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Dark");
            ImGui::SameLine();
            ToggleButton("##ThemeToggle", &dark_theme);
            if (ImGui::IsItemClicked())
            {
                Theme::ApplyNativeTheme(dark_theme ? Theme::NATIVE_DARK : Theme::NATIVE_LIGHT);
            }

            ImGui::EndMenuBar();
        }
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        // Setup default docking layout
        setupDefaultDockingLayout(dockspace_id);

        // Database sidebar
        renderDatabaseSidebar();

        // Main content area
        ImGui::Begin("Content");
        if (tabs.empty())
        {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2);
            float buttonWidth = 200;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) / 2);
            if (ImGui::Button("Create First SQL Editor", ImVec2(buttonWidth, 0)))
            {
                auto tab = std::make_shared<Tab>("SQL Editor 1", TabType::SQL_EDITOR);
                tab->shouldFocus = true; // Focus the first SQL editor tab
                tabs.push_back(tab);
            }
        }
        else
        {
            if (ImGui::BeginTabBar("ContentTabs"))
            {
                for (auto it = tabs.begin(); it != tabs.end();)
                {
                    auto &tab = *it;

                    // Handle tab focusing
                    ImGuiTabItemFlags tabFlags = ImGuiTabItemFlags_None;
                    if (tab->shouldFocus)
                    {
                        tabFlags |= ImGuiTabItemFlags_SetSelected;
                        tab->shouldFocus = false; // Reset flag after use
                    }

                    if (tab->open && ImGui::BeginTabItem(tab->name.c_str(), &tab->open, tabFlags))
                    {
                        switch (tab->type)
                        {
                        case TabType::SQL_EDITOR:
                            renderSQLEditor(*tab);
                            break;
                        case TabType::TABLE_VIEWER:
                            renderTableViewer(*tab);
                            break;
                        }
                        ImGui::EndTabItem();
                    }

                    if (!tab->open)
                    {
                        it = tabs.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // End DockSpace
        ImGui::End();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    for (auto &db : databases)
    {
        disconnectDatabase(*db);
    }

    // Cleanup Native File Dialog
    NFD_Quit();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
