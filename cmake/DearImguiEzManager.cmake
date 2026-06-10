# DearImguiEzManager — static library on top of Dear ImGui.
# Use via add_subdirectory() or find_package(DearImguiEzManager).

if(TARGET DearImguiEzManager)
    return()
endif()

include(FetchContent)

# ── FreeType ──────────────────────────────────────────────────────────────────
if(DEZMGR_FETCH_FREETYPE)
    set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG        VER-2-13-3
    )
    FetchContent_MakeAvailable(freetype)
endif()

if(NOT TARGET freetype)
    message(FATAL_ERROR "FreeType target 'freetype' not found. Enable DEZMGR_FETCH_FREETYPE or provide it before including DearImguiEzManager.")
endif()

# ── Dear ImGui ────────────────────────────────────────────────────────────────
if(DEZMGR_IMGUI_DIR)
    set(_dezmgr_imgui_dir "${DEZMGR_IMGUI_DIR}")
elseif(DEZMGR_FETCH_IMGUI)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
    )
    FetchContent_MakeAvailable(imgui)
    set(_dezmgr_imgui_dir "${imgui_SOURCE_DIR}")
else()
    message(FATAL_ERROR
        "Dear ImGui not found. Set DEZMGR_IMGUI_DIR to the imgui source root "
        "or enable DEZMGR_FETCH_IMGUI=ON."
    )
endif()

if(NOT EXISTS "${_dezmgr_imgui_dir}/imgui.h")
    message(FATAL_ERROR "Invalid DEZMGR_IMGUI_DIR: '${_dezmgr_imgui_dir}' (imgui.h not found)")
endif()

# ── Generated paths header ────────────────────────────────────────────────────
set(DEZMGR_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${DEZMGR_GEN_DIR}")

set(_dezmgr_tahoma_ttf "${CMAKE_CURRENT_SOURCE_DIR}/assets/tahoma.ttf")

file(TO_CMAKE_PATH "${_dezmgr_tahoma_ttf}" _dezmgr_tahoma_ttf_cmake)
file(WRITE "${DEZMGR_GEN_DIR}/dearimgui_ezmgr_paths.h"
"#pragma once\n#define EZMGR_TAHOMA_TTF \"${_dezmgr_tahoma_ttf_cmake}\"\n")

# ── Library target ────────────────────────────────────────────────────────────
set(DEZMGR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gfxManager/gfxManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d/d3d11Manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/d3d/d3d9Manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MenuManager/FontsManager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MenuManager/EmbeddedFontsData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MenuManager/LayoutManager.cpp"
)

add_library(DearImguiEzManager STATIC ${DEZMGR_SOURCES})
add_library(DearImguiEzManager::DearImguiEzManager ALIAS DearImguiEzManager)

target_compile_definitions(DearImguiEzManager
    PUBLIC
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    PRIVATE
        PB_GFX_D3D_ENABLED
)

target_include_directories(DearImguiEzManager
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${DEZMGR_GEN_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${_dezmgr_imgui_dir}"
        "${_dezmgr_imgui_dir}/backends"
        "${_dezmgr_imgui_dir}/misc/freetype"
)

target_link_libraries(DearImguiEzManager
    PUBLIC
        freetype
)

if(WIN32)
    target_link_libraries(DearImguiEzManager
        PUBLIC
            d3d11
            d3d9
            dxgi
    )
    target_compile_definitions(DearImguiEzManager PUBLIC WIN32)
endif()

# The consumer must build and link Dear ImGui and its backends.
set(DEZMGR_IMGUI_DIR "${_dezmgr_imgui_dir}" CACHE PATH "Resolved Dear ImGui source root" FORCE)
