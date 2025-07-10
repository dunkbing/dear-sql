#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

// Forward declarations
class Database;
class Tab;
class TabManager;
class DatabaseSidebar;
class FileDialog;

class Application
{
public:
    static Application &getInstance();

    // Delete copy constructor and assignment operator
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    // Main application lifecycle
    bool initialize();
    void run();
    void cleanup();

    // Getters for managers and state
    TabManager *getTabManager() const { return tabManager.get(); }
    DatabaseSidebar *getDatabaseSidebar() const { return databaseSidebar.get(); }
    FileDialog *getFileDialog() const { return fileDialog.get(); }

    // Theme management
    bool isDarkTheme() const { return darkTheme; }
    void setDarkTheme(bool dark);

    // Selection state
    int getSelectedDatabase() const { return selectedDatabase; }
    void setSelectedDatabase(int index) { selectedDatabase = index; }
    int getSelectedTable() const { return selectedTable; }
    void setSelectedTable(int index) { selectedTable = index; }

    // UI state
    bool isDockingLayoutInitialized() const { return dockingLayoutInitialized; }
    void setDockingLayoutInitialized(bool initialized) { dockingLayoutInitialized = initialized; }

    // Database management
    std::vector<std::shared_ptr<Database>> &getDatabases() { return databases; }
    const std::vector<std::shared_ptr<Database>> &getDatabases() const { return databases; }
    void addDatabase(std::shared_ptr<Database> db);

    // Window reference
    GLFWwindow *getWindow() const { return window; }

private:
    Application() = default;
    ~Application() = default;

    // Core components
    GLFWwindow *window = nullptr;
    std::unique_ptr<TabManager> tabManager;
    std::unique_ptr<DatabaseSidebar> databaseSidebar;
    std::unique_ptr<FileDialog> fileDialog;

    // Application state
    bool darkTheme = true;
    int selectedDatabase = -1;
    int selectedTable = -1;
    bool dockingLayoutInitialized = false;

    // Data
    std::vector<std::shared_ptr<Database>> databases;

    // Private helper methods
    bool initializeGLFW();
    bool initializeImGui();
    void setupFonts();
    void setupDockingLayout(ImGuiID dockspaceId);
    void renderMainUI();
    void renderMenuBar();
};
