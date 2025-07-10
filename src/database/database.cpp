#include "database/database.hpp"
#include <iostream>

Database::Database(const std::string &name, const std::string &path) : name(name), path(path) {}

Database::~Database() {
    disconnect();
}

bool Database::connect() {
    if (connected && connection) {
        return true;
    }

    int rc = sqlite3_open(path.c_str(), &connection);
    if (rc != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(connection) << std::endl;
        return false;
    }

    std::cout << "Successfully connected to database: " << path << std::endl;
    connected = true;
    return true;
}

void Database::disconnect() {
    if (connection) {
        sqlite3_close(connection);
        connection = nullptr;
    }
    connected = false;
}

void Database::refreshTables() {
    std::cout << "Refreshing tables for database: " << name << std::endl;
    if (!connect()) {
        std::cout << "Failed to connect to database" << std::endl;
        tablesLoaded = true; // Mark as loaded even if failed to prevent retry
        return;
    }

    tables.clear();
    std::vector<std::string> tableNames = getTableNames();
    std::cout << "Found " << tableNames.size() << " tables" << std::endl;

    for (const auto &tableName : tableNames) {
        std::cout << "Adding table: " << tableName << std::endl;
        Table table;
        table.name = tableName;
        table.columns = getTableColumns(tableName);
        tables.push_back(table);
    }
    std::cout << "Finished refreshing tables. Total tables: " << tables.size() << std::endl;
    tablesLoaded = true; // Mark as loaded to prevent infinite refresh
}

std::vector<std::string> Database::getTableNames() {
    std::vector<std::string> tableNames;
    const char *sql =
        "SELECT name FROM sqlite_master WHERE type IN ('table', 'view') ORDER BY name;";
    sqlite3_stmt *stmt;

    std::cout << "Executing query to get table names..." << std::endl;
    int rc = sqlite3_prepare_v2(connection, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *tableName = (const char *)sqlite3_column_text(stmt, 0);
            if (tableName) {
                std::cout << "Found table: " << tableName << std::endl;
                tableNames.push_back(std::string(tableName));
            }
        }
    } else {
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(connection) << std::endl;
    }
    sqlite3_finalize(stmt);
    std::cout << "Query completed. Found " << tableNames.size() << " tables." << std::endl;
    return tableNames;
}

std::vector<Column> Database::getTableColumns(const std::string &tableName) {
    std::vector<Column> columns;
    std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(connection, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
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
