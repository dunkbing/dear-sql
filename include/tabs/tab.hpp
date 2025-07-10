#pragma once

#include <memory>
#include <string>
#include <vector>

enum class TabType { SQL_EDITOR, TABLE_VIEWER };

class Tab {
public:
    Tab(const std::string &name, TabType type);
    virtual ~Tab() = default;

    // Common properties
    const std::string &getName() const {
        return name;
    }
    void setName(const std::string &newName) {
        name = newName;
    }
    TabType getType() const {
        return type;
    }
    bool isOpen() const {
        return open;
    }
    void setOpen(bool isOpen) {
        open = isOpen;
    }
    bool shouldFocus() const {
        return needsFocus;
    }
    void setShouldFocus(bool focus) {
        needsFocus = focus;
    }

    // Virtual method for rendering tab content
    virtual void render() = 0;

protected:
    std::string name;
    TabType type;
    bool open = true;
    bool needsFocus = false;
};

class SQLEditorTab : public Tab {
public:
    SQLEditorTab(const std::string &name);

    void render() override;

    // SQL Editor specific methods
    const std::string &getQuery() const {
        return sqlQuery;
    }
    void setQuery(const std::string &query) {
        sqlQuery = query;
    }
    const std::string &getResult() const {
        return queryResult;
    }
    void setResult(const std::string &result) {
        queryResult = result;
    }

private:
    std::string sqlQuery;
    std::string queryResult;
    char sqlBuffer[4096] = "";
    char resultBuffer[16384] = "";
};

class TableViewerTab : public Tab {
public:
    TableViewerTab(const std::string &name, const std::string &databasePath,
                   const std::string &tableName);

    void render() override;

    // Table Viewer specific methods
    const std::string &getDatabasePath() const {
        return databasePath;
    }
    const std::string &getTableName() const {
        return tableName;
    }
    void loadData();
    void nextPage();
    void previousPage();
    void firstPage();
    void lastPage();

private:
    std::string databasePath;
    std::string tableName;
    std::vector<std::vector<std::string>> tableData;
    std::vector<std::string> columnNames;
    int currentPage = 0;
    int rowsPerPage = 100;
    int totalRows = 0;
};
