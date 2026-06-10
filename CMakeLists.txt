cmake_minimum_required(VERSION 3.20)

project(DearImguiEzManager
    VERSION 0.1.0
    DESCRIPTION "C++ helpers for Dear ImGui: gfx backends, layout, fonts, scaling"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEZMGR_FETCH_FREETYPE "Fetch FreeType via FetchContent" ON)
option(DEZMGR_FETCH_IMGUI "Fetch Dear ImGui via FetchContent when DEZMGR_IMGUI_DIR is empty" OFF)
option(DEZMGR_BUILD_EXAMPLES "Build example applications" OFF)

set(DEZMGR_IMGUI_DIR "" CACHE PATH "Path to Dear ImGui source root (directory containing imgui.h)")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/DearImguiEzManager.cmake")

if(DEZMGR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(
    TARGETS DearImguiEzManager
    EXPORT DearImguiEzManagerTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/DearImguiEzManager"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/tahoma.ttf"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/DearImguiEzManager/assets"
)

install(
    EXPORT DearImguiEzManagerTargets
    FILE DearImguiEzManagerTargets.cmake
    NAMESPACE DearImguiEzManager::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DearImguiEzManager"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DearImguiEzManagerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/DearImguiEzManagerConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DearImguiEzManager"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DearImguiEzManagerConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/DearImguiEzManagerConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/DearImguiEzManagerConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/DearImguiEzManager"
)
