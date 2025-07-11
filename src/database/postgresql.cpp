#include "database/postgresql.hpp"
#include <iostream>
#include <memory>
#include <sstream>

PostgreSQLDatabase::PostgreSQLDatabase(const std::string &name, const std::string &host, int port,
                                       const std::string &database, const std::string &username,
                                       const std::string &password)
    : name(name), host(host), port(port), database(database), username(username),
      password(password) {

    // Build connection string
    std::stringstream ss;
    ss << "host=" << host << " port=" << port << " dbname=" << database << " user=" << username
       << " password=" << password;
    connectionString = ss.str();
}

PostgreSQLDatabase::~PostgreSQLDatabase() {
    disconnect();
}

bool PostgreSQLDatabase::connect() {
    if (connected && connection) {
        return true;
    }

    try {
        connection = std::make_unique<pqxx::connection>(connectionString);
        std::cout << "Successfully connected to PostgreSQL database: " << database << std::endl;
        connected = true;
        return true;
    } catch (const std::exception &e) {
        std::cerr << "Connection to database failed: " << e.what() << std::endl;
        connection.reset();
        connected = false;
        return false;
    }
}

void PostgreSQLDatabase::disconnect() {
    if (connection) {
        connection.reset();
    }
    connected = false;
}

bool PostgreSQLDatabase::isConnected() const {
    return connected && connection && connection->is_open();
}

const std::string &PostgreSQLDatabase::getName() const {
    return name;
}

const std::string &PostgreSQLDatabase::getConnectionString() const {
    return connectionString;
}

const std::string &PostgreSQLDatabase::getPath() const {
    return connectionString; // For PostgreSQL, path is the connection string
}

DatabaseType PostgreSQLDatabase::getType() const {
    return DatabaseType::POSTGRESQL;
}

void PostgreSQLDatabase::refreshTables() {
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

const std::vector<Table> &PostgreSQLDatabase::getTables() const {
    return tables;
}

std::vector<Table> &PostgreSQLDatabase::getTables() {
    return tables;
}

bool PostgreSQLDatabase::areTablesLoaded() const {
    return tablesLoaded;
}

void PostgreSQLDatabase::setTablesLoaded(bool loaded) {
    tablesLoaded = loaded;
}

std::string PostgreSQLDatabase::executeQuery(const std::string &query) {
    if (!connect()) {
        return "Error: Failed to connect to database";
    }

    try {
        pqxx::work txn(*connection);
        pqxx::result result = txn.exec(query);

        std::stringstream output;

        if (!result.empty()) {
            // Headers
            for (size_t i = 0; i < result.columns(); ++i) {
                output << result.column_name(i);
                if (i < result.columns() - 1)
                    output << " | ";
            }
            output << "\n";

            // Separator
            for (size_t i = 0; i < result.columns(); ++i) {
                output << "----------";
                if (i < result.columns() - 1)
                    output << "-+-";
            }
            output << "\n";

            // Data rows (limit to 1000)
            size_t rowLimit = std::min(result.size(), 1000);
            for (size_t i = 0; i < rowLimit; ++i) {
                for (size_t j = 0; j < result.columns(); ++j) {
                    output << (result[i][j].is_null() ? "NULL" : result[i][j].c_str());
                    if (j < result.columns() - 1)
                        output << " | ";
                }
                output << "\n";
            }

            if (result.size() > 1000) {
                output << "\n... (showing first 1000 rows)";
            }
        } else {
            output << "Query executed successfully. Rows affected: " << result.affected_rows();
        }

        return output.str();
    } catch (const std::exception &e) {
        return "Error: " + std::string(e.what());
    }
}

std::vector<std::vector<std::string>> PostgreSQLDatabase::getTableData(const std::string &tableName,
                                                                       int limit, int offset) {
    std::vector<std::vector<std::string>> data;
    if (!connect()) {
        return data;
    }

    try {
        pqxx::work txn(*connection);
        std::string sql = "SELECT * FROM " + txn.quote_name(tableName) + " LIMIT " +
                          std::to_string(limit) + " OFFSET " + std::to_string(offset);

        pqxx::result result = txn.exec(sql);

        for (const auto &row : result) {
            std::vector<std::string> rowData;
            for (size_t i = 0; i < row.size(); ++i) {
                rowData.push_back(row[i].is_null() ? "NULL" : row[i].c_str());
            }
            data.push_back(rowData);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error getting table data: " << e.what() << std::endl;
    }

    return data;
}

std::vector<std::string> PostgreSQLDatabase::getColumnNames(const std::string &tableName) {
    std::vector<std::string> columnNames;
    if (!connect()) {
        return columnNames;
    }

    try {
        pqxx::work txn(*connection);
        std::string sql = "SELECT column_name FROM information_schema.columns WHERE table_name = " +
                          txn.quote(tableName) + " ORDER BY ordinal_position";

        pqxx::result result = txn.exec(sql);

        for (const auto &row : result) {
            columnNames.push_back(row[0].c_str());
        }
    } catch (const std::exception &e) {
        std::cerr << "Error getting column names: " << e.what() << std::endl;
    }

    return columnNames;
}

int PostgreSQLDatabase::getRowCount(const std::string &tableName) {
    if (!connect()) {
        return 0;
    }

    try {
        pqxx::work txn(*connection);
        std::string sql = "SELECT COUNT(*) FROM " + txn.quote_name(tableName);
        pqxx::result result = txn.exec(sql);

        if (!result.empty()) {
            return result[0][0].as<int>();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error getting row count: " << e.what() << std::endl;
    }

    return 0;
}

bool PostgreSQLDatabase::isExpanded() const {
    return expanded;
}

void PostgreSQLDatabase::setExpanded(bool exp) {
    expanded = exp;
}

void *PostgreSQLDatabase::getConnection() const {
    return connection.get();
}

std::vector<std::string> PostgreSQLDatabase::getTableNames() {
    std::vector<std::string> tableNames;

    try {
        pqxx::work txn(*connection);
        std::string sql =
            "SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename";

        std::cout << "Executing query to get table names..." << std::endl;
        pqxx::result result = txn.exec(sql);

        for (const auto &row : result) {
            std::string tableName = row[0].c_str();
            std::cout << "Found table: " << tableName << std::endl;
            tableNames.push_back(tableName);
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to execute query: " << e.what() << std::endl;
    }

    std::cout << "Query completed. Found " << tableNames.size() << " tables." << std::endl;
    return tableNames;
}

std::vector<Column> PostgreSQLDatabase::getTableColumns(const std::string &tableName) {
    std::vector<Column> columns;

    try {
        pqxx::work txn(*connection);
        std::string sql = "SELECT c.column_name, c.data_type, c.is_nullable, "
                          "CASE WHEN tc.constraint_type = 'PRIMARY KEY' THEN true ELSE false END "
                          "as is_primary_key "
                          "FROM information_schema.columns c "
                          "LEFT JOIN information_schema.key_column_usage kcu ON c.column_name = "
                          "kcu.column_name AND c.table_name = kcu.table_name "
                          "LEFT JOIN information_schema.table_constraints tc ON "
                          "kcu.constraint_name = tc.constraint_name "
                          "WHERE c.table_name = " +
                          txn.quote(tableName) +
                          " "
                          "ORDER BY c.ordinal_position";

        pqxx::result result = txn.exec(sql);

        for (const auto &row : result) {
            Column col;
            col.name = row[0].c_str();
            col.type = row[1].c_str();
            col.isNotNull = std::string(row[2].c_str()) == "NO";
            col.isPrimaryKey = row[3].as<bool>();
            columns.push_back(col);
        }
    } catch (const std::exception &e) {
        std::cerr << "Error getting table columns: " << e.what() << std::endl;
    }

    return columns;
}