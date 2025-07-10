#include "application.hpp"
#include "database/database.hpp"
#include "tabs/tab_manager.hpp"
#include "ui/database_sidebar.hpp"
#include "utils/file_dialog.hpp"
#include "utils/toggle_button.hpp"
#include "themes.hpp"
#include <iostream>
#include <fstream>
#include <imgui_internal.h>

Application &Application::getInstance()
{
    static Application instance;
    return instance;
}

bool Application::initialize()
{
    std::cout << "Starting Dear SQL..." << std::endl;

    if (!initializeGLFW())
    {
        return false;
    }

    if (!initializeImGui())
    {
        return false;
    }

    // Initialize NFD
    if (!FileDialog::initialize())
    {
        std::cerr << "Failed to initialize Native File Dialog" << std::endl;
        return false;
    }

    // Create managers
    tabManager = std::make_unique<TabManager>();
    databaseSidebar = std::make_unique<DatabaseSidebar>();
    fileDialog = std::make_unique<FileDialog>();

    std::cout << "Application initialized successfully" << std::endl;
    return true;
}

void Application::run()
{
    glClearColor(
        darkTheme ? 0.110f : 0.957f,
        darkTheme ? 0.110f : 0.957f,
        darkTheme ? 0.137f : 0.957f,
        0.98f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMainUI();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void Application::cleanup()
{
    // Cleanup databases
    for (auto &db : databases)
    {
        db->disconnect();
    }
    databases.clear();

    // Cleanup components
    tabManager.reset();
    databaseSidebar.reset();
    fileDialog.reset();

    // Cleanup NFD
    FileDialog::cleanup();

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup GLFW
    if (window)
    {
        glfwDestroyWindow(window);
    }
    glfwTerminate();

    std::cout << "Application cleanup completed" << std::endl;
}

void Application::setDarkTheme(bool dark)
{
    darkTheme = dark;
    Theme::ApplyNativeTheme(darkTheme ? Theme::NATIVE_DARK : Theme::NATIVE_LIGHT);
}

void Application::addDatabase(std::shared_ptr<Database> db)
{
    databases.push_back(db);
}

bool Application::initializeGLFW()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(1280, 720, "Dear SQL", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::cout << "GLFW window created successfully" << std::endl;
    return true;
}

bool Application::initializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    setupFonts();

    ImGui::StyleColorsDark();
    Theme::ApplyNativeTheme(darkTheme ? Theme::NATIVE_DARK : Theme::NATIVE_LIGHT);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::cout << "ImGui initialized" << std::endl;
    return true;
}

void Application::setupFonts()
{
    ImGuiIO &io = ImGui::GetIO();

    // Configure fonts with Japanese support
    ImFontConfig fontConfig;
    fontConfig.MergeMode = false;

    // Try to load fonts from assets folder first, then fallback to system fonts
    std::vector<std::string> fontPaths = {
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
}

void Application::setupDockingLayout(ImGuiID dockspaceId)
{
    if (dockingLayoutInitialized)
        return;

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    // Split the dockspace into left and right
    ImGuiID dock_left, dock_right;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &dock_left, &dock_right);

    // Dock windows to specific nodes
    ImGui::DockBuilderDockWindow("Databases", dock_left);
    ImGui::DockBuilderDockWindow("Content", dock_right);

    ImGui::DockBuilderFinish(dockspaceId);
    dockingLayoutInitialized = true;
}

void Application::renderMainUI()
{
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
    renderMenuBar();

    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

    // Setup default docking layout
    setupDockingLayout(dockspace_id);

    // Database sidebar
    databaseSidebar->render();

    // Main content area
    ImGui::Begin("Content");
    if (tabManager->isEmpty())
    {
        tabManager->renderEmptyState();
    }
    else
    {
        tabManager->renderTabs();
    }
    ImGui::End();

    // End DockSpace
    ImGui::End();
}

void Application::renderMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Database", "Ctrl+O"))
            {
                auto db = fileDialog->openDatabaseDialog();
                if (db)
                {
                    // Try to connect and load tables
                    if (db->connect())
                    {
                        db->refreshTables();
                        std::cout << "Adding database to list. Tables loaded: " << db->getTables().size() << std::endl;
                        addDatabase(db);
                    }
                    else
                    {
                        std::cerr << "Failed to open database: " << db->getPath() << std::endl;
                    }
                }
            }
            if (ImGui::MenuItem("New SQL Editor", "Ctrl+N"))
            {
                tabManager->createSQLEditorTab();
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
                    if (db->isConnected())
                    {
                        db->refreshTables();
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
        UIUtils::ToggleButton("##ThemeToggle", &darkTheme);
        if (ImGui::IsItemClicked())
        {
            setDarkTheme(darkTheme);
        }

        ImGui::EndMenuBar();
    }
}
