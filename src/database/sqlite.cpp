#include "database/sqlite.hpp"
#include <iostream>
#include <sstream>
#include <utility>

SQLiteDatabase::SQLiteDatabase(std::string name, std::string path)
    : name(std::move(name)), path(std::move(path)) {}

SQLiteDatabase::~SQLiteDatabase() {
    SQLiteDatabase::disconnect();
}

bool SQLiteDatabase::connect() {
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

void SQLiteDatabase::disconnect() {
    if (connection) {
        sqlite3_close(connection);
        connection = nullptr;
    }
    connected = false;
}

bool SQLiteDatabase::isConnected() const {
    return connected;
}

const std::string &SQLiteDatabase::getName() const {
    return name;
}

const std::string &SQLiteDatabase::getConnectionString() const {
    return path;
}

const std::string &SQLiteDatabase::getPath() const {
    return path;
}

DatabaseType SQLiteDatabase::getType() const {
    return DatabaseType::SQLITE;
}

void SQLiteDatabase::refreshTables() {
    std::cout << "Refreshing tables for database: " << name << std::endl;
    if (!connect()) {
        std::cout << "Failed to connect to database" << std::endl;
        tablesLoaded = true;
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
    tablesLoaded = true;
}

const std::vector<Table> &SQLiteDatabase::getTables() const {
    return tables;
}

std::vector<Table> &SQLiteDatabase::getTables() {
    return tables;
}

bool SQLiteDatabase::areTablesLoaded() const {
    return tablesLoaded;
}

void SQLiteDatabase::setTablesLoaded(bool loaded) {
    tablesLoaded = loaded;
}

std::string SQLiteDatabase::executeQuery(const std::string &query) {
    if (!connect()) {
        return "Error: Failed to connect to database";
    }

    std::stringstream result;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(connection, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "Error: " + std::string(sqlite3_errmsg(connection));
    }

    int columnCount = sqlite3_column_count(stmt);
    if (columnCount > 0) {
        for (int i = 0; i < columnCount; i++) {
            result << sqlite3_column_name(stmt, i);
            if (i < columnCount - 1)
                result << " | ";
        }
        result << "\n";

        for (int i = 0; i < columnCount; i++) {
            result << "----------";
            if (i < columnCount - 1)
                result << "-+-";
        }
        result << "\n";
    }

    int rowCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && rowCount < 1000) {
        for (int i = 0; i < columnCount; i++) {
            const char *text = (const char *)sqlite3_column_text(stmt, i);
            result << (text ? text : "NULL");
            if (i < columnCount - 1)
                result << " | ";
        }
        result << "\n";
        rowCount++;
    }

    if (rowCount == 0 && columnCount == 0) {
        result << "Query executed successfully. Rows affected: " << sqlite3_changes(connection);
    } else if (rowCount == 1000) {
        result << "\n... (showing first 1000 rows)";
    }

    sqlite3_finalize(stmt);
    return result.str();
}

std::vector<std::vector<std::string>>
SQLiteDatabase::getTableData(const std::string &tableName, const int limit, const int offset) {
    std::vector<std::vector<std::string>> data;
    if (!connect()) {
        return data;
    }

    std::string sql = "SELECT * FROM " + tableName + " LIMIT " + std::to_string(limit) +
                      " OFFSET " + std::to_string(offset);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(connection, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        int columnCount = sqlite3_column_count(stmt);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::vector<std::string> row;
            for (int i = 0; i < columnCount; i++) {
                auto text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
                row.emplace_back(text ? text : "NULL");
            }
            data.push_back(row);
        }
    }
    sqlite3_finalize(stmt);
    return data;
}

std::vector<std::string> SQLiteDatabase::getColumnNames(const std::string &tableName) {
    std::vector<std::string> columnNames;
    if (!connect()) {
        return columnNames;
    }

    const std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(connection, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            columnNames.emplace_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        }
    }
    sqlite3_finalize(stmt);
    return columnNames;
}

int SQLiteDatabase::getRowCount(const std::string &tableName) {
    if (!connect()) {
        return 0;
    }

    const std::string sql = "SELECT COUNT(*) FROM " + tableName;
    sqlite3_stmt *stmt;
    int count = 0;

    if (sqlite3_prepare_v2(connection, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return count;
}

bool SQLiteDatabase::isExpanded() const {
    return expanded;
}

void SQLiteDatabase::setExpanded(bool exp) {
    expanded = exp;
}

void *SQLiteDatabase::getConnection() const {
    return connection;
}

std::vector<std::string> SQLiteDatabase::getTableNames() {
    std::vector<std::string> tableNames;
    const char *sql =
        "SELECT name FROM sqlite_master WHERE type IN ('table', 'view') ORDER BY name;";
    sqlite3_stmt *stmt;

    std::cout << "Executing query to get table names..." << std::endl;
    int rc = sqlite3_prepare_v2(connection, sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            auto tableName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if (tableName) {
                std::cout << "Found table: " << tableName << std::endl;
                tableNames.emplace_back(tableName);
            }
        }
    } else {
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(connection) << std::endl;
    }
    sqlite3_finalize(stmt);
    std::cout << "Query completed. Found " << tableNames.size() << " tables." << std::endl;
    return tableNames;
}

std::vector<Column> SQLiteDatabase::getTableColumns(const std::string &tableName) {
    std::vector<Column> columns;
    std::string sql = "PRAGMA table_info(" + tableName + ");";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(connection, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Column col;
            col.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            col.type = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            col.isNotNull = sqlite3_column_int(stmt, 3) == 1;
            col.isPrimaryKey = sqlite3_column_int(stmt, 5) == 1;
            columns.push_back(col);
        }
    }
    sqlite3_finalize(stmt);
    return columns;
}
