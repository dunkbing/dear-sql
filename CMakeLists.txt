cmake_minimum_required(VERSION 3.15)
project(dear-sql)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
add_subdirectory(external/json)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Native File Dialog Extended (as git submodule)
add_subdirectory(external/nativefiledialog-extended)

# GLFW + Backend (Metal on macOS, OpenGL elsewhere)
find_package(glfw3 3.3 REQUIRED)

# Common ImGui sources
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
)

# Platform-specific backend configuration
if(APPLE)
    # Metal backend for macOS
    find_library(METAL_LIBRARY Metal REQUIRED)
    find_library(QUARTZCORE_LIBRARY QuartzCore REQUIRED)
    find_library(FOUNDATION_LIBRARY Foundation REQUIRED)

    if(NOT METAL_LIBRARY)
        message(FATAL_ERROR "Metal framework not found. Make sure you're building on macOS 10.11 or later.")
    endif()

    message(STATUS "Using Metal backend for macOS")
    message(STATUS "Found Metal framework: ${METAL_LIBRARY}")
    message(STATUS "Found QuartzCore framework: ${QUARTZCORE_LIBRARY}")
    message(STATUS "Found Foundation framework: ${FOUNDATION_LIBRARY}")

    # Add Metal backend source
    list(APPEND IMGUI_SOURCES external/imgui/backends/imgui_impl_metal.mm)

    # Define preprocessor macro
    add_definitions(-DUSE_METAL_BACKEND)
else()
    # OpenGL backend for other platforms
    find_package(OpenGL REQUIRED)

    message(STATUS "Using OpenGL backend for ${CMAKE_SYSTEM_NAME}")

    # Add OpenGL backend source
    list(APPEND IMGUI_SOURCES external/imgui/backends/imgui_impl_opengl3.cpp)

    # Define preprocessor macro
    add_definitions(-DUSE_OPENGL_BACKEND)
endif()

# Application source files
set(APP_SOURCES
    src/main.cpp

    # Database
    src/database/database.cpp
    src/database/query_executor.cpp

    # Tabs
    src/tabs/tab.cpp
    src/tabs/tab_manager.cpp

    # UI
    src/ui/database_sidebar.cpp

    # Utils
    src/utils/file_dialog.cpp
    src/utils/toggle_button.cpp
)

# Platform-specific application sources
if(APPLE)
    # Use .mm extension for Objective-C++ on macOS
    list(APPEND APP_SOURCES src/application.mm)
else()
    # Use .cpp extension for other platforms
    list(APPEND APP_SOURCES src/application.cpp)
endif()

# Main application
add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    external/imgui
    external/imgui/backends
    external/json/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    SQLite::SQLite3
    nlohmann_json::nlohmann_json
    nfd
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${FOUNDATION_LIBRARY}
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
    )
endif()

# Set macOS specific properties
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Dear SQL"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()
