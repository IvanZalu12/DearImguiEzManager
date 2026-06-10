#pragma once

#include <DearImguiEzManager/MenuManager/ScaleManager.hpp>
#include <imgui.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

enum class SplitDir : std::uint8_t { Horizontal, Vertical };
enum class AnchorSide : std::uint8_t { Left, Right, Top, Bottom };
enum class TileRoundingStyle : std::uint8_t {
    OuterOnly,   // default: only the 4 outer corners of the whole window layout
    AllTiles,    // every leaf tile gets all 4 corners rounded
    FirstLevel,  // each first-level group's boundary corners are rounded independently
};

struct TileContext {
    float contentW;
    float contentH;
    float tileW;
    float tileH;
    ImVec2 posMin;
    ImVec2 posMax;
    const char* name;
};

using TileCallback = std::function<void(const TileContext& ctx)>;

struct TileDef {
    float fraction;
    std::string name;

    TileCallback callback;

    SplitDir childDir = SplitDir::Horizontal;
    std::vector<TileDef> children;

    TileDef(float frac, const char* n, TileCallback cb)
        : fraction(frac), name(n), callback(std::move(cb)) {}

    TileDef(float frac, const char* n, SplitDir dir, std::vector<TileDef> kids)
        : fraction(frac), name(n), childDir(dir), children(std::move(kids)) {}
};

struct WindowHandle {
    std::uint32_t id = UINT32_MAX;
    bool IsValid() const noexcept { return id != UINT32_MAX; }
};

class LayoutManager {
public:
    explicit LayoutManager(ScaleManager* scale) noexcept;

    WindowHandle RegisterWindow(const char* name, int baseW, int baseH) noexcept;

    void SetTileLayout(WindowHandle wnd, SplitDir rootDir,
                       std::vector<TileDef> tiles) noexcept;

    void SetTileGap(WindowHandle wnd, int baseGap) noexcept;
    void SetTileInnerPadding(WindowHandle wnd, int basePadX, int basePadY) noexcept;
    void SetTileBlendColor(WindowHandle wnd, ImVec4 color) noexcept;
    void SetWorkspacePadding(WindowHandle wnd, int basePadX, int basePadY) noexcept;
    void SetWindowRounding(WindowHandle wnd, int baseRounding) noexcept;
    void SetTileRoundingStyle(WindowHandle wnd, TileRoundingStyle style) noexcept;

    void CenterOnFirstShow(WindowHandle wnd) noexcept;
    void SetAntiOffscreen(WindowHandle wnd, bool enabled) noexcept;
    void Anchor(WindowHandle child, WindowHandle parent,
                AnchorSide side, int baseOffset = 0) noexcept;
    void SetFixed(WindowHandle wnd, bool fixed) noexcept;

    // viewportScreenOrigin: the render client area's top-left in screen pixels
    // (e.g. GfxManager::GetClientScreenOrigin). Used to keep floating windows on the
    // same screen spot across maximize/restore. Pass {0,0} to disable that behavior.
    void BeginFrame(ImVec2 viewportScreenOrigin = ImVec2(0.f, 0.f)) noexcept;
    void RenderWindow(WindowHandle wnd) noexcept;

    ImVec2 GetWindowPos(WindowHandle wnd) const noexcept;
    ImVec2 GetWindowSize(WindowHandle wnd) const noexcept;

private:
    struct AnchorInfo {
        WindowHandle parent;
        AnchorSide   side       = AnchorSide::Right;
        int          baseOffset = 0;
    };

    struct WindowState {
        std::string name;
        int baseW = 0;
        int baseH = 0;

        SplitDir rootDir = SplitDir::Horizontal;
        std::vector<TileDef> tiles;

        int    baseGap      = 4;
        int    basePadX     = 6;
        int    basePadY     = 6;
        int    baseWorkPadX = 0;
        int    baseWorkPadY = 0;
        int    baseRounding = 6;
        ImVec4 tileBlendColor  = { 0.f, 0.f, 0.f, 0.f };
        TileRoundingStyle roundingStyle = TileRoundingStyle::OuterOnly;

        bool firstShow      = true;
        bool centerOnFirst  = false;
        bool antiOffscreen  = true;
        bool fixed          = false;
        bool forceReposition = false;  // one-shot: re-apply pos with ImGuiCond_Always

        bool       anchored = false;
        AnchorInfo anchor;

        ImVec2 lastPos  = {0, 0};
        ImVec2 lastSize = {0, 0};
    };

    void RenderTiles(const std::vector<TileDef>& tiles, SplitDir dir,
                     ImVec2 areaMin, ImVec2 areaMax,
                     float gap, float padX, float padY,
                     ImVec4 blendColor,
                     int windowRounding,
                     TileRoundingStyle roundingStyle,
                     int depth,
                     bool cornerTL, bool cornerTR,
                     bool cornerBL, bool cornerBR) noexcept;

    void RenderLeafTile(const TileDef& tile,
                        ImVec2 min, ImVec2 max,
                        float padX, float padY,
                        ImVec4 blendColor,
                        int rTL, int rTR, int rBL, int rBR) noexcept;

    ImVec2 CalcAnchoredPos(const WindowState& child) const noexcept;

    ScaleManager*             m_pScale;
    std::vector<WindowState>  m_windows;
    ImVec2                    m_viewportSize   = {0, 0};
    ImVec2                    m_viewportOrigin = {0, 0};  // client top-left in screen px
};
