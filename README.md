# DearImguiEzManager

C++ helpers on top of [Dear ImGui](https://github.com/ocornut/imgui): graphics management (D3D9/D3D11, Android OpenGL), window/tile layout, UI scaling, FreeType fonts, and embedded TTF data.

**Author:** [IvanZalu12](https://github.com/IvanZalu12)

## Features

- **GfxManager** — window creation, render loop, D3D11 with D3D9 fallback (Windows)
- **LayoutManager** — hierarchical tile layout with callbacks
- **FontsManager** — font registration and atlas rebuild on scale changes
- **ScaleManager** — unified UI scale factor
- **ImGuiTextMeasure** — text measurement helpers

## Dependencies

| 	                  Dependency                    |                          Purpose                              |	
|---------------------------------------------------|---------------------------------------------------------------|
|   [Dear ImGui](https://github.com/ocornut/imgui)  | UI (required; linked by the consumer)                         |
|   [FreeType](https://freetype.org/)               | font rendering (fetched via CMake FetchContent by default)    |
|   Windows SDK                                     | D3D9/D3D11 backends (Windows only)                            |


## Quick start

### 1. Get Dear ImGui

Clone Dear ImGui next to this repo or point CMake to an existing checkout:

```bash
git clone https://github.com/ocornut/imgui.git third_party/imgui
```

### 2. Build the library

```bash
cmake -B build -DDEZMGR_IMGUI_DIR=third_party/imgui
cmake --build build --config Release
```

Optional: fetch ImGui automatically:

```bash
cmake -B build -DDEZMGR_FETCH_IMGUI=ON
```

### 3. Use in your project

**Via `add_subdirectory`:**

```cmake
set(DEZMGR_IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui" CACHE PATH "" FORCE)
add_subdirectory(third_party/DearImguiEzManager)

add_executable(MyApp main.cpp
    ${IMGUI_SOURCES}  # imgui.cpp, backends, imgui_freetype.cpp — your choice
)
target_link_libraries(MyApp PRIVATE DearImguiEzManager::DearImguiEzManager)
target_include_directories(MyApp PRIVATE
    "${DEZMGR_IMGUI_DIR}"
    "${DEZMGR_IMGUI_DIR}/backends"
    "${DEZMGR_IMGUI_DIR}/misc/freetype"
)
```

**Via install:**

```bash
cmake --install build --prefix install
```

```cmake
find_package(DearImguiEzManager CONFIG REQUIRED)
target_link_libraries(MyApp PRIVATE DearImguiEzManager::DearImguiEzManager)
```

### 4. Public API

Include a single umbrella header:

```cpp
#include <DearImguiEzManager/DearImguiEzManager.hpp>
```

## Repository layout

```
DearImguiEzManager/
  include/DearImguiEzManager/   # public headers
  src/                          # implementation (.cpp)
  cmake/                        # CMake modules
  assets/                       # tahoma.ttf (optional path for file-based font registration)
```

## CMake options

|         Option          | Default |           Description             |
|-------------------------|---------|-----------------------------------|
| `DEZMGR_IMGUI_DIR`      |    —    | Path to Dear ImGui source root    |
| `DEZMGR_FETCH_FREETYPE` |  `ON`   | Fetch FreeType via FetchContent   |
| `DEZMGR_FETCH_IMGUI`    |  `OFF`  | Fetch Dear ImGui via FetchContent |
| `DEZMGR_BUILD_EXAMPLES` |  `OFF`  | Build examples (when added)       |


## Platforms

- **Windows** — D3D11 / D3D9, Win32 backend
- **Android** — `androidOpenGLManager.hpp` (implementation in `gfxManager.cpp`)

## License

MIT — see [LICENSE](LICENSE).
