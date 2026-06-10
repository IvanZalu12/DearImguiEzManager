#include <DearImguiEzManager/MenuManager/LayoutManager.hpp>

#include <algorithm>
#include <cmath>

LayoutManager::LayoutManager(ScaleManager* scale) noexcept
    : m_pScale(scale)
{
}

WindowHandle LayoutManager::RegisterWindow(const char* name, int baseW, int baseH) noexcept
{
    WindowHandle h;
    h.id = static_cast<std::uint32_t>(m_windows.size());
    WindowState ws;
    ws.name  = name ? name : "";
    ws.baseW = baseW;
    ws.baseH = baseH;
    m_windows.push_back(std::move(ws));
    return h;
}

void LayoutManager::SetTileLayout(WindowHandle wnd, SplitDir rootDir,
                                  std::vector<TileDef> tiles) noexcept
{
    if (wnd.id >= m_windows.size()) return;
    auto& ws   = m_windows[wnd.id];
    ws.rootDir = rootDir;
    ws.tiles   = std::move(tiles);
}

void LayoutManager::SetTileGap(WindowHandle wnd, int baseGap) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].baseGap = baseGap;
}

void LayoutManager::SetTileInnerPadding(WindowHandle wnd, int basePadX, int basePadY) noexcept
{
    if (wnd.id < m_windows.size()) {
        m_windows[wnd.id].basePadX = basePadX;
        m_windows[wnd.id].basePadY = basePadY;
    }
}

void LayoutManager::SetTileBlendColor(WindowHandle wnd, ImVec4 color) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].tileBlendColor = color;
}

void LayoutManager::SetWorkspacePadding(WindowHandle wnd, int basePadX, int basePadY) noexcept
{
    if (wnd.id < m_windows.size()) {
        m_windows[wnd.id].baseWorkPadX = basePadX;
        m_windows[wnd.id].baseWorkPadY = basePadY;
    }
}

void LayoutManager::SetWindowRounding(WindowHandle wnd, int baseRounding) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].baseRounding = baseRounding;
}

void LayoutManager::SetTileRoundingStyle(WindowHandle wnd, TileRoundingStyle style) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].roundingStyle = style;
}

void LayoutManager::CenterOnFirstShow(WindowHandle wnd) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].centerOnFirst = true;
}

void LayoutManager::SetAntiOffscreen(WindowHandle wnd, bool enabled) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].antiOffscreen = enabled;
}

void LayoutManager::Anchor(WindowHandle child, WindowHandle parent,
                           AnchorSide side, int baseOffset) noexcept
{
    if (child.id >= m_windows.size()) return;
    auto& ws    = m_windows[child.id];
    ws.anchored = true;
    ws.anchor   = { parent, side, baseOffset };
}

void LayoutManager::SetFixed(WindowHandle wnd, bool fixed) noexcept
{
    if (wnd.id < m_windows.size())
        m_windows[wnd.id].fixed = fixed;
}

void LayoutManager::BeginFrame() noexcept
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (vp) m_viewportSize = vp->Size;
}

void LayoutManager::RenderWindow(WindowHandle wnd) noexcept
{
    if (!m_pScale || wnd.id >= m_windows.size()) return;
    auto& ws = m_windows[wnd.id];

    const float w = static_cast<float>(m_pScale->Px(ws.baseW));
    const float h = static_cast<float>(m_pScale->Px(ws.baseH));
    const int   r = m_pScale->Rounding(ws.baseRounding);

    ImVec2 pos = ws.lastPos;

    if (ws.anchored)
        pos = CalcAnchoredPos(ws);

    if (ws.firstShow && ws.centerOnFirst) {
        pos.x = (m_viewportSize.x - w) * 0.5f;
        pos.y = (m_viewportSize.y - h) * 0.5f;
    }

    if (ws.antiOffscreen) {
        constexpr float minVisible = 32.f;
        pos.x = std::clamp(pos.x, minVisible - w, m_viewportSize.x - minVisible);
        pos.y = std::clamp(pos.y, 0.f, m_viewportSize.y - minVisible);
    }

    // Anchored windows need ImGuiCond_Always so they track parent every frame
    const ImGuiCond posCond = (ws.firstShow || ws.anchored) ? ImGuiCond_Always : ImGuiCond_Once;
    ImGui::SetNextWindowPos(pos, posCond);
    ImGui::SetNextWindowSize(ImVec2(w, h));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, static_cast<float>(r));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoScrollbar
                           | ImGuiWindowFlags_NoScrollWithMouse;
    if (ws.fixed)
        flags |= ImGuiWindowFlags_NoMove;

    ImGui::Begin(ws.name.c_str(), nullptr, flags);

    ws.lastPos  = ImGui::GetWindowPos();
    ws.lastSize = ImGui::GetWindowSize();

    if (!ws.tiles.empty()) {
        const float wpX   = static_cast<float>(m_pScale->Px(ws.baseWorkPadX));
        const float wpY   = static_cast<float>(m_pScale->Px(ws.baseWorkPadY));
        const ImVec2 areaMin = { ws.lastPos.x + wpX,     ws.lastPos.y + wpY     };
        const ImVec2 areaMax = { ws.lastPos.x + w - wpX, ws.lastPos.y + h - wpY };
        const float  gap  = static_cast<float>(m_pScale->Px(ws.baseGap));
        const float  padX = static_cast<float>(m_pScale->Px(ws.basePadX));
        const float  padY = static_cast<float>(m_pScale->Px(ws.basePadY));

        RenderTiles(ws.tiles, ws.rootDir, areaMin, areaMax,
                    gap, padX, padY, ws.tileBlendColor, r,
                    ws.roundingStyle, 0,
                    true, true, true, true);
    }

    ImGui::End();
    ImGui::PopStyleVar(2);

    ws.firstShow = false;
}

void LayoutManager::RenderTiles(const std::vector<TileDef>& tiles, SplitDir dir,
                                ImVec2 areaMin, ImVec2 areaMax,
                                float gap, float padX, float padY,
                                ImVec4 blendColor,
                                int windowRounding,
                                TileRoundingStyle roundingStyle,
                                int depth,
                                bool cTL, bool cTR, bool cBL, bool cBR) noexcept
{
    if (tiles.empty()) return;

    const float totalW    = areaMax.x - areaMin.x;
    const float totalH    = areaMax.y - areaMin.y;
    const float totalGaps = gap * static_cast<float>(tiles.size() - 1);

    const float available = (dir == SplitDir::Horizontal)
        ? (totalW - totalGaps)
        : (totalH - totalGaps);

    float fracSum = 0.f;
    for (const auto& t : tiles) fracSum += t.fraction;
    if (fracSum < 1e-6f) fracSum = 1.f;

    float cursor = 0.f;

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile       = tiles[i];
        const float normalized = tile.fraction / fracSum;
        const float span       = std::floor(available * normalized);

        ImVec2 tMin, tMax;
        if (dir == SplitDir::Horizontal) {
            tMin = { areaMin.x + cursor, areaMin.y };
            tMax = { tMin.x + span,      areaMax.y };
        } else {
            tMin = { areaMin.x,          areaMin.y + cursor };
            tMax = { areaMax.x,          tMin.y + span      };
        }

        const bool isFirst = (i == 0);
        const bool isLast  = (i == tiles.size() - 1);

        bool tileTL, tileTR, tileBL, tileBR;
        if (dir == SplitDir::Horizontal) {
            tileTL = cTL && isFirst;
            tileBL = cBL && isFirst;
            tileTR = cTR && isLast;
            tileBR = cBR && isLast;
        } else {
            tileTL = cTL && isFirst;
            tileTR = cTR && isFirst;
            tileBL = cBL && isLast;
            tileBR = cBR && isLast;
        }

        if (!tile.children.empty()) {
            // FirstLevel: each top-level group resets to all-4-corners so its
            // boundary leaves get the correct outer rounding independently.
            bool nextTL = tileTL, nextTR = tileTR, nextBL = tileBL, nextBR = tileBR;
            if (roundingStyle == TileRoundingStyle::FirstLevel && depth == 0)
                nextTL = nextTR = nextBL = nextBR = true;

            RenderTiles(tile.children, tile.childDir, tMin, tMax,
                        gap, padX, padY, blendColor, windowRounding,
                        roundingStyle, depth + 1,
                        nextTL, nextTR, nextBL, nextBR);
        } else {
            int rTL, rTR, rBL, rBR;
            if (roundingStyle == TileRoundingStyle::AllTiles ||
                (roundingStyle == TileRoundingStyle::FirstLevel && depth == 0)) {
                rTL = rTR = rBL = rBR = windowRounding;
            } else {
                rTL = tileTL ? windowRounding : 0;
                rTR = tileTR ? windowRounding : 0;
                rBL = tileBL ? windowRounding : 0;
                rBR = tileBR ? windowRounding : 0;
            }
            RenderLeafTile(tile, tMin, tMax, padX, padY, blendColor, rTL, rTR, rBL, rBR);
        }

        cursor += span + gap;
    }
}

void LayoutManager::RenderLeafTile(const TileDef& tile,
                                   ImVec2 min, ImVec2 max,
                                   float padX, float padY,
                                   ImVec4 blendColor,
                                   int rTL, int rTR, int rBL, int rBR) noexcept
{
    // Background drawn on parent draw list (per-corner rounding, before child window)
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImDrawFlags roundFlags = ImDrawFlags_RoundCornersNone;
    float maxR = 0.f;

    if (rTL > 0) { roundFlags |= ImDrawFlags_RoundCornersTopLeft;     maxR = std::max(maxR, (float)rTL); }
    if (rTR > 0) { roundFlags |= ImDrawFlags_RoundCornersTopRight;    maxR = std::max(maxR, (float)rTR); }
    if (rBL > 0) { roundFlags |= ImDrawFlags_RoundCornersBottomLeft;  maxR = std::max(maxR, (float)rBL); }
    if (rBR > 0) { roundFlags |= ImDrawFlags_RoundCornersBottomRight; maxR = std::max(maxR, (float)rBR); }

    const ImU32 bgColor = ImGui::GetColorU32(ImGuiCol_ChildBg);
    if (roundFlags != ImDrawFlags_RoundCornersNone)
        dl->AddRectFilled(min, max, bgColor, maxR, roundFlags);
    else
        dl->AddRectFilled(min, max, bgColor, 0.f);

    if (blendColor.w > 0.f) {
        const ImU32 blendU32 = ImGui::ColorConvertFloat4ToU32(blendColor);
        if (roundFlags != ImDrawFlags_RoundCornersNone)
            dl->AddRectFilled(min, max, blendU32, maxR, roundFlags);
        else
            dl->AddRectFilled(min, max, blendU32, 0.f);
    }

    // BeginChild gives the tile its own window context so that ItemSize()'s
    // cursor.x reset lands on the tile's own left edge (window->Pos.x == min.x),
    // not on the parent window's left edge. Without this, only the leftmost tile
    // in each horizontal strip renders correctly.
    const ImVec2 size = { max.x - min.x, max.y - min.y };

    ImGui::SetCursorScreenPos(min);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.f, 0.f, 0.f, 0.f));

    const bool visible = ImGui::BeginChild(tile.name.c_str(), size, false,
                                           ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse);
    if (visible && tile.callback) {
        // Apply padding via Indent + SetCursorPos so that ItemSize()'s cursor.x
        // reset respects the offset on every item, not just the first.
        // PushStyleVar(WindowPadding) before BeginChild does NOT reliably update
        // DC.CursorStartPos after the window already exists.
        if (padX > 0.f) ImGui::Indent(padX);
        ImGui::SetCursorPos(ImVec2(padX, padY));

        TileContext ctx;
        ctx.contentW = size.x - padX * 2.f;
        ctx.contentH = size.y - padY * 2.f;
        ctx.tileW    = size.x;
        ctx.tileH    = size.y;
        ctx.posMin   = min;
        ctx.posMax   = max;
        ctx.name     = tile.name.c_str();
        tile.callback(ctx);

        if (padX > 0.f) ImGui::Unindent(padX);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(1);
}

ImVec2 LayoutManager::CalcAnchoredPos(const WindowState& child) const noexcept
{
    if (!child.anchored || child.anchor.parent.id >= m_windows.size())
        return child.lastPos;

    const auto& parent = m_windows[child.anchor.parent.id];
    const float off    = static_cast<float>(m_pScale->Px(child.anchor.baseOffset));
    const float cw     = static_cast<float>(m_pScale->Px(child.baseW));
    const float ch     = static_cast<float>(m_pScale->Px(child.baseH));

    ImVec2 pos;
    switch (child.anchor.side) {
        case AnchorSide::Right:
            pos.x = parent.lastPos.x + parent.lastSize.x + off;
            pos.y = parent.lastPos.y;
            break;
        case AnchorSide::Left:
            pos.x = parent.lastPos.x - cw - off;
            pos.y = parent.lastPos.y;
            break;
        case AnchorSide::Bottom:
            pos.x = parent.lastPos.x;
            pos.y = parent.lastPos.y + parent.lastSize.y + off;
            break;
        case AnchorSide::Top:
            pos.x = parent.lastPos.x;
            pos.y = parent.lastPos.y - ch - off;
            break;
    }
    return pos;
}

ImVec2 LayoutManager::GetWindowPos(WindowHandle wnd) const noexcept
{
    if (wnd.id >= m_windows.size()) return {0, 0};
    return m_windows[wnd.id].lastPos;
}

ImVec2 LayoutManager::GetWindowSize(WindowHandle wnd) const noexcept
{
    if (wnd.id >= m_windows.size()) return {0, 0};
    return m_windows[wnd.id].lastSize;
}
