#include "database/db_interface.hpp"
#include "database/postgresql.hpp"
#include "database/sqlite.hpp"

std::shared_ptr<DatabaseInterface>
DatabaseFactory::createDatabase(const DatabaseConnectionInfo &info) {
    switch (info.type) {
    case DatabaseType::SQLITE:
        return std::make_shared<SQLiteDatabase>(info.name, info.path);

    case DatabaseType::POSTGRESQL:
        return std::make_shared<PostgreSQLDatabase>(info.name, info.host, info.port, info.database,
                                                    info.username, info.password);

    default:
        return nullptr;
    }
}
