# DearImguiEzManager — Usage Guide

Extended API reference with copy-paste examples. For build and CMake setup see [README.md](README.md).

All public headers are reachable through a single include:

```cpp
#include <DearImguiEzManager/DearImguiEzManager.hpp>
#include <imgui.h>
```

---

## Table of contents

1. [Minimal application skeleton](#1-minimal-application-skeleton)
2. [GfxManager — window & render loop](#2-gfxmanager--window--render-loop)
3. [ScaleManager — UI scaling](#3-scalemanager--ui-scaling)
4. [FontsManager — fonts & atlas lifecycle](#4-fontsmanager--fonts--atlas-lifecycle)
5. [LayoutManager — tile layouts](#5-layoutmanager--tile-layouts)
6. [ImGuiTextMeasure — text sizing helpers](#6-imguitextmeasure--text-sizing-helpers)
7. [ImGuiStyleColors — theme shortcuts](#7-imguistylecolors--theme-shortcuts)
8. [Embedded font data](#8-embedded-font-data)
9. [Full demo walkthrough](#9-full-demo-walkthrough)

---

## 1. Minimal application skeleton

Typical Windows app wiring: create `GfxManager`, register fonts/layout during init, then run the message loop.

```cpp
#include <Windows.h>
#include <DearImguiEzManager/DearImguiEzManager.hpp>
#include <imgui.h>

static GfxManager* g_gfx = nullptr;
static ScaleManager g_scale;
static FontsManager g_fonts(&g_scale);
static LayoutManager g_layout(&g_scale);

static void AtlasRefresh() noexcept
{
    if (g_gfx)
        g_gfx->RefreshImGuiFontTexture();
}

static void PreFrameCallback() noexcept
{
    (void)g_fonts.RebuildIfNeeded();   // rebuild atlas when scale crosses a pixel boundary
}

static void RenderCallback() noexcept
{
    g_gfx->RenderDefaultTitleBar("My App");   // CustomChrome only; no-op for other styles

    const POINT origin = g_gfx->GetClientScreenOrigin();
    g_layout.BeginFrame(ImVec2(static_cast<float>(origin.x), static_cast<float>(origin.y)));
    // g_layout.RenderWindow(...);
}

static bool MenuInit()
{
    g_fonts.SetAtlasInvalidateCallback(AtlasRefresh);
    auto font = g_fonts.RegisterGUIFont("assets/tahoma.ttf", 16.0f);
    if (!g_fonts.BuildAtlas())
        return false;

    // register windows, tile layouts, etc.
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    GfxManager* gfx = GfxManager::Create(
        hInstance, 1280, 720, L"My App",
        ManagedWindowStyle::CustomChrome);

    if (!gfx)
        return 1;

    g_gfx = gfx;

    if (!MenuInit()) {
        gfx->Shutdown();
        delete gfx;
        return 1;
    }

    gfx->SetPreFrameCallback(PreFrameCallback);
    gfx->SetRenderCallback(RenderCallback);
    gfx->ShowManagedWindow(SW_SHOWDEFAULT);

    for (;;) {
        if (gfx->HandleWMSG() == WM_QUIT)
            break;
        gfx->RenderFrame();
    }

    gfx->Shutdown();
    delete gfx;
    return 0;
}
```

**Callback order each frame**

| Phase | Where | Purpose |
|-------|-------|---------|
| `HandleWMSG()` | main loop | pump Win32 messages |
| `PreFrameCallback` | before `ImGui::NewFrame` | font atlas rebuild, input prep |
| `RenderCallback` | inside ImGui frame | draw UI (title bar, layout, widgets) |
| `RenderFrame()` | GfxManager | present swap chain |

---

## 2. GfxManager — window & render loop

`GfxManager::Create` picks **D3D11** and falls back to **D3D9** automatically on Windows.

### Window styles

```cpp
enum class ManagedWindowStyle {
    Normal,              // standard OS chrome (_ □ ✖ handled by Windows)
    OverlayTransparent,  // borderless layered overlay; supports alpha & click-through
    CustomChrome,        // borderless; title bar drawn by ImGui, native resize/snap kept
};
```

**Normal window**

```cpp
auto* gfx = GfxManager::Create(hInstance, 800, 600, L"Standard Window",
                               ManagedWindowStyle::Normal);
```

**Transparent overlay**

```cpp
auto* gfx = GfxManager::Create(hInstance, 800, 600, L"Overlay",
                               ManagedWindowStyle::OverlayTransparent);
gfx->SetWindowAlpha(200);          // 0–255
gfx->SetClickThrough(true);        // mouse passes through when not over ImGui widgets
```

**Custom chrome (ImGui title bar)**

```cpp
auto* gfx = GfxManager::Create(hInstance, 1280, 720, L"Custom Chrome",
                               ManagedWindowStyle::CustomChrome);
gfx->SetTitleBarHeight(36.0f);     // logical px; multiply by ScaleManager if needed

// Inside RenderCallback, at the very top of the frame:
float titleH = gfx->RenderDefaultTitleBar("My Application");
// titleH can be used to offset content below the bar
```

For a hand-rolled title bar, tell the manager when the cursor is over a draggable region:

```cpp
gfx->SetTitleBarDragHovered(ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered());
```

Window controls (CustomChrome):

```cpp
gfx->MinimizeWindow();
gfx->ToggleMaximizeWindow();
gfx->CloseWindow();                // posts WM_CLOSE
bool maxed = gfx->IsWindowMaximized();
```

### Callbacks

```cpp
gfx->SetPreFrameCallback([]() noexcept { /* before NewFrame */ });
gfx->SetRenderCallback([]() noexcept { /* draw UI */ });
gfx->SetMenuInitCallback([]() { /* optional; called once after graphics init */ });
```

### Client origin for floating windows

Pass the render client top-left (screen pixels) to `LayoutManager::BeginFrame` so floating panels stay in place across maximize/restore:

```cpp
const POINT origin = gfx->GetClientScreenOrigin();
layout.BeginFrame(ImVec2(static_cast<float>(origin.x), static_cast<float>(origin.y)));
```

---

## 3. ScaleManager — UI scaling

Single global scale factor in range **1.0 … 4.0**, snapped to `SCALE_STEP` (default **0.05**).

```cpp
ScaleManager scale;
scale.SetScale(1.5f);

int   btnW  = scale.Px(120);           // round(base × scale)
int   btnH  = scale.PxFloor(32);       // floor variant
float gap   = scale.PxF(4.0f);         // float variant
int   round = scale.Rounding(6);       // non-linear rounding for corners
ImVec2 pad  = scale.Pad(8, 4);         // scaled ImVec2 padding
```

`LayoutManager` and `FontsManager` hold a pointer to the same `ScaleManager` instance — always pass the shared object:

```cpp
ScaleManager scale;
FontsManager fonts(&scale);
LayoutManager layout(&scale);
```

Runtime scale slider (from a status bar tile):

```cpp
static float uiScale = scale.GetScale();
if (ImGui::SliderFloat("UI Scale", &uiScale, 1.0f, 4.0f)) {
    scale.SetScale(uiScale);
    fonts.RebuildIfNeeded();   // or rely on PreFrameCallback
}
```

---

## 4. FontsManager — fonts & atlas lifecycle

Two font pools with different strategies:

| Pool | Strategy | Best for |
|------|----------|----------|
| **GUIFont** | one rasterized copy per font; lazy rebuild on scale change | menus, panels, static UI |
| **GamingFont** | pre-built copies every 0.5 px from `base×1.0` to `base×4.0` | HUD text that scales smoothly every frame |

### Setup

```cpp
FontsManager fonts(&scale);

fonts.SetAtlasInvalidateCallback([]() noexcept {
    g_gfx->RefreshImGuiFontTexture();   // required after every BuildAtlas()
});

// From file:
FontsManager::GUIFontHandle uiFont =
    fonts.RegisterGUIFont("assets/tahoma.ttf", 16.0f);

// From embedded compressed TTF (see §8):
FontsManager::GUIFontHandle uiFont =
    fonts.RegisterGUIFontCompressed(Tahoma_compressed_data, Tahoma_compressed_size, 16.0f);

// Optional custom glyph ranges (nullptr = ImGui default):
static const ImWchar cyrillic[] = { 0x0400, 0x04FF, 0 };
auto fontRu = fonts.RegisterGUIFont("assets/tahoma.ttf", 14.0f, cyrillic);

if (!fonts.BuildAtlas())
    return false;   // check console for FreeType errors
```

### Using GUIFont

```cpp
ImFont* f = fonts.GetGUI(uiFont);
if (f) {
    ImGui::PushFont(f);
    ImGui::Text("Scaled menu text");
    ImGui::PopFont();
}

int targetPx = fonts.GetGUITargetSizePx(uiFont);   // ceil(baseSize × scale)
int builtPx  = fonts.GetGUIBuiltSizePx(uiFont);    // size in current atlas
```

### Using GamingFont (smooth HUD scaling)

```cpp
FontsManager::GamingFontHandle hudFont =
    fonts.RegisterGamingFontCompressed(Tahoma_compressed_data, Tahoma_compressed_size, 12.0f);

// Each frame:
FontsManager::GamingFontUse use = fonts.GetGaming(hudFont);
if (use.valid()) {
    use.font->Scale = use.displayScale;
    ImGui::PushFont(use.font);
    ImGui::Text("HP: %.0f", hp);
    ImGui::PopFont();
    use.font->Scale = 1.f;   // restore — other code may share the ImFont*
}
```

### Atlas rebuild rules

| Call | When |
|------|------|
| `BuildAtlas()` | once after all `Register*()` calls; also after adding new fonts |
| `RebuildIfNeeded()` | every frame (or after `SetScale`) — rebuilds only when a GUIFont crosses an integer pixel size |

```cpp
// Typical PreFrameCallback:
void PreFrameCallback() noexcept
{
    if (fonts.RebuildIfNeeded())
        ; // atlas was rebuilt; RefreshImGuiFontTexture already invoked via callback
}
```

---

## 5. LayoutManager — tile layouts

Hierarchical split-pane layout: windows contain a tree of tiles; leaf tiles invoke user callbacks.

### Core types

```cpp
enum class SplitDir : std::uint8_t { Horizontal, Vertical };

struct TileContext {
    float contentW, contentH;   // area inside inner padding — use for widget widths
    float tileW, tileH;         // full tile size including padding
    ImVec2 posMin, posMax;      // screen-space bounds of the tile
    const char* name;           // debug / ImGui ID suffix
};

using TileCallback = std::function<void(const TileContext& ctx)>;
```

### Register a window

```cpp
WindowHandle mainWnd = layout.RegisterWindow("##MainPanel", 800, 600);
layout.CenterOnFirstShow(mainWnd);
layout.SetWorkspacePadding(mainWnd, 8, 8);       // gap between window edge and tile area
layout.SetTileInnerPadding(mainWnd, 12, 8);      // gap between tile border and content
layout.SetTileGap(mainWnd, 4);                   // gap between adjacent tiles
layout.SetWindowRounding(mainWnd, 8);
layout.SetTileBlendColor(mainWnd, ImVec4(0.05f, 0.05f, 0.05f, 0.1f));
layout.SetTileRoundingStyle(mainWnd, TileRoundingStyle::AllTiles);
```

`TileRoundingStyle`:

| Value | Effect |
|-------|--------|
| `OuterOnly` | round only the four outer corners of the whole layout (default) |
| `AllTiles` | every leaf tile gets four rounded corners |
| `FirstLevel` | each top-level group gets its own rounded boundary |

### Tile callbacks

```cpp
static void TileSidebar(const TileContext& ctx)
{
    ImGui::TextDisabled("Sidebar");
    ImGui::Separator();
    ImGui::Button("Action", ImVec2(ctx.contentW, 0));   // full content width
}

static void TileMain(const TileContext& ctx)
{
    ImGui::BeginChild("main", ImVec2(ctx.contentW, ctx.contentH));
    // ...
    ImGui::EndChild();
}
```

Use `##` suffixes in widget labels (as in ImGui) — tile names are already unique per leaf.

### Building a layout tree

**Leaf tile** — fraction + name + callback:

```cpp
{ 0.30f, "sidebar", TileSidebar }
```

**Group tile** — fraction + name + split direction + children:

```cpp
{ 0.40f, "top_row", SplitDir::Horizontal, {
    { 0.50f, "left",  TileLeft  },
    { 0.50f, "right", TileRight },
}}
```

Fractions are relative weights within the parent (they need not sum to exactly 1.0, but typically do).

**Complete example — three-row dashboard:**

```cpp
layout.SetTileLayout(mainWnd, SplitDir::Vertical, {
    // Top row — three columns
    { 0.40f, "top_row", SplitDir::Horizontal, {
        { 0.333f, "nav",    TileNavigation },
        { 0.334f, "stats",  TileStats      },
        { 0.333f, "opts",   TileOptions    },
    }},
    // Middle row — two columns
    { 0.30f, "mid_row", SplitDir::Horizontal, {
        { 0.50f, "character", TileCharacter  },
        { 0.50f, "inventory", TileInventory  },
    }},
    // Bottom bar — single leaf
    { 0.30f, "status_bar", TileStatusBar },
});
```

### Render loop integration

```cpp
void RenderCallback() noexcept
{
    g_gfx->RenderDefaultTitleBar("Dashboard");

    const POINT origin = g_gfx->GetClientScreenOrigin();
    g_layout.BeginFrame(ImVec2(static_cast<float>(origin.x),
                                static_cast<float>(origin.y)));
    g_layout.RenderWindow(mainWnd);
}
```

`BeginFrame` must be called once per ImGui frame before any `RenderWindow`.

### Multiple windows & anchoring

```cpp
WindowHandle toolbar = layout.RegisterWindow("##Toolbar", 800, 48);
WindowHandle panel   = layout.RegisterWindow("##Panel",   320, 400);

layout.SetFixed(toolbar, true);                              // non-draggable
layout.Anchor(panel, toolbar, AnchorSide::Right, baseOffset=8);

// AnchorSide: Left | Right | Top | Bottom
// baseOffset is scaled via ScaleManager
```

Other window options:

```cpp
layout.SetAntiOffscreen(wnd, true);    // clamp position inside viewport (default: on)
layout.GetWindowPos(wnd);              // last known ImGui position
layout.GetWindowSize(wnd);             // last known size
```

---

## 6. ImGuiTextMeasure — text sizing helpers

Thin wrappers around `ImGui::CalcTextSize` that accept `const char*`, `std::string`, or `std::string_view`.

```cpp
#include <DearImguiEzManager/MenuManager/ImGuiTextMeasure.hpp>

std::string label = "Disconnect##sb";
float labelW = EZMGR_CalcTextWidth(label);
float labelH = EZMGR_CalcTextHeight(label);
ImVec2 size  = EZMGR_CalcTextSize(label);

// Right-align a button inside a tile:
ImGui::SameLine(ctx.contentW - labelW);
ImGui::Button(label.c_str(), ImVec2(labelW, 0));

// Size progress bars to match text line height:
ImGui::ProgressBar(ratio, ImVec2(ctx.contentW, EZMGR_CalcTextHeight("X") + 1.0f));
```

These use the **currently active ImGui font**. Push the desired font first if measurements must match a specific face.

---

## 7. ImGuiStyleColors — theme shortcuts

```cpp
#include <DearImguiEzManager/MenuManager/ImGuiStyleColors.hpp>

EZMGR_STYLE_COLORS_DARK();      // ImGui::StyleColorsDark()
EZMGR_STYLE_COLORS_LIGHT();     // ImGui::StyleColorsLight()
EZMGR_STYLE_COLORS_CLASSIC();   // ImGui::StyleColorsClassic()
EZMGR_STYLE_COLORS_APPLY();     // alias for Light (override in your project if needed)
```

Call once after `ImGui::CreateContext()`, before the first frame.

---

## 8. Embedded font data

The library ships compressed TTF blobs (Tahoma, Verdana and variants) in `EmbeddedFontsData.hpp`:

```cpp
#include <DearImguiEzManager/MenuManager/EmbeddedFontsData.hpp>

// Available symbols:
//   Tahoma_compressed_data,          Tahoma_compressed_size
//   Verdana_compressed_data,           Verdana_compressed_size
//   VerdanaBold_compressed_data,     VerdanaBold_compressed_size
//   VerdanaItalic_compressed_data,     VerdanaItalic_compressed_size
//   VerdanaBoldItalic_compressed_data, VerdanaBoldItalic_compressed_size

auto regular = fonts.RegisterGUIFontCompressed(
    Verdana_compressed_data, Verdana_compressed_size, 16.0f);
auto bold = fonts.RegisterGUIFontCompressed(
    VerdanaBold_compressed_data, VerdanaBold_compressed_size, 16.0f);
```

File-based registration remains available for custom fonts:

```cpp
fonts.RegisterGUIFont("assets/tahoma.ttf", 16.0f);
```

---

## 9. Full demo walkthrough

The reference implementation lives in the consumer project `DearImguiEzMgr` (`src/main.cpp`). It demonstrates:

- `CustomChrome` window with `RenderDefaultTitleBar`
- six tile callbacks sharing one `ScaleManager`
- compressed embedded fonts at multiple base sizes
- live UI scale slider with `RebuildIfNeeded`
- `EZMGR_CalcTextWidth` for right-aligned status-bar buttons
- nested `SetTileLayout` with horizontal and vertical splits

Minimal excerpt — layout definition:

```cpp
g_mainWnd = g_layout.RegisterWindow("##MainDemo", 800, 600);
g_layout.CenterOnFirstShow(g_mainWnd);
g_layout.SetWorkspacePadding(g_mainWnd, 8, 8);
g_layout.SetTileInnerPadding(g_mainWnd, 12, 8);
g_layout.SetTileBlendColor(g_mainWnd, ImVec4(0.05f, 0.05f, 0.05f, 0.1f));
g_layout.SetTileRoundingStyle(g_mainWnd, TileRoundingStyle::AllTiles);

g_layout.SetTileLayout(g_mainWnd, SplitDir::Vertical, {
    { 0.40f, "top_row", SplitDir::Horizontal, {
        { 0.333f, "top_nav",   TileNavigation },
        { 0.334f, "top_stats", TileStats      },
        { 0.333f, "top_opts",  TileOptions    },
    }},
    { 0.30f, "mid_row", SplitDir::Horizontal, {
        { 0.50f, "mid_char", TileCharacter },
        { 0.50f, "mid_inv",  TileInventory },
    }},
    { 0.30f, "bottom_bar", TileStatusBar },
});
```

---

## Android note

On non-Windows platforms `GfxManager::Create(int w, int h)` returns an OpenGL ES backend (`AndroidOpenGLManager`). There is no Win32 message loop — integrate `RenderFrame()` into your platform's frame callback and omit `HandleWMSG()` / `ShowManagedWindow()`.

---

## Quick reference

| Component | Header | Key entry points |
|-----------|--------|------------------|
| Umbrella | `DearImguiEzManager.hpp` | includes everything below |
| GfxManager | `gfxManager/gfxManager.hpp` | `Create`, `RenderFrame`, `HandleWMSG`, `RenderDefaultTitleBar` |
| ScaleManager | `MenuManager/ScaleManager.hpp` | `SetScale`, `GetScale`, `Px`, `Pad`, `Rounding` |
| FontsManager | `MenuManager/FontsManager.hpp` | `RegisterGUIFont*`, `RegisterGamingFont*`, `BuildAtlas`, `RebuildIfNeeded` |
| LayoutManager | `MenuManager/LayoutManager.hpp` | `RegisterWindow`, `SetTileLayout`, `BeginFrame`, `RenderWindow` |
| Text measure | `MenuManager/ImGuiTextMeasure.hpp` | `EZMGR_CalcTextWidth`, `EZMGR_CalcTextHeight`, `EZMGR_CalcTextSize` |
| Style colors | `MenuManager/ImGuiStyleColors.hpp` | `EZMGR_STYLE_COLORS_*` macros |
| Embedded fonts | `MenuManager/EmbeddedFontsData.hpp` | `*_compressed_data`, `*_compressed_size` |
