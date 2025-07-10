#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

class QueryExecutor {
public:
    static std::string executeQuery(sqlite3 *db, const std::string &query);

    static std::vector<std::vector<std::string>>
    getTableData(sqlite3 *db, const std::string &tableName, int limit, int offset);

    static std::vector<std::string> getColumnNames(sqlite3 *db, const std::string &tableName);

    static int getRowCount(sqlite3 *db, const std::string &tableName);
};
