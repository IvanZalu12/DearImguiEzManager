#include <DearImguiEzManager/MenuManager/FontsManager.hpp>
#include <misc/freetype/imgui_freetype.h>

#include <Windows.h>
#include <cmath>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

FontsManager::FontsManager(ScaleManager* scale) noexcept
    : m_pScale(scale)
{}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

void FontsManager::CopyGlyphRanges(const ImWchar* src, std::vector<ImWchar>& out) noexcept
{
    out.clear();
    if (!src)
        return;
    for (int i = 0; ; i += 2) {
        out.push_back(src[i]);
        out.push_back(src[i + 1]);
        if (src[i] == 0 && src[i + 1] == 0)
            break;
    }
}

int FontsManager::guiTargetSizePx(const GUIFontEntry& e) const noexcept
{
    // lround(baseSize × scale): round to the nearest integer.
    // ceil would grow too aggressively: at scale=1.05 baseSize=9
    // ceil(9.45)=10 — the font jumps early, whereas lround(9.45)=9.
    // lround better matches the desired logical size without overshooting.
    const float raw = e.baseSize * m_pScale->GetScale();
    return static_cast<int>(std::lround(static_cast<double>(raw)));
}

const FontsManager::GUIFontEntry* FontsManager::guiEntry(GUIFontHandle h) const noexcept
{
    return (h.id < m_guiFonts.size()) ? &m_guiFonts[h.id] : nullptr;
}

const FontsManager::GamingFontEntry* FontsManager::gamingEntry(GamingFontHandle h) const noexcept
{
    return (h.id < m_gamingFonts.size()) ? &m_gamingFonts[h.id] : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Registration
// ─────────────────────────────────────────────────────────────────────────────

FontsManager::GUIFontHandle FontsManager::RegisterGUIFont(
    const char* path, float baseSize, const ImWchar* glyphRanges) noexcept
{
    GUIFontEntry e;
    e.source   = FontSource::FileTtf;
    e.path     = path ? path : "";
    e.baseSize = baseSize;
    CopyGlyphRanges(glyphRanges, e.glyphOwned);
    const GUIFontHandle h{ static_cast<std::uint32_t>(m_guiFonts.size()) };
    m_guiFonts.push_back(std::move(e));
    return h;
}

FontsManager::GUIFontHandle FontsManager::RegisterGUIFontCompressed(
    const unsigned char* data, unsigned int dataSize,
    float baseSize, const ImWchar* glyphRanges) noexcept
{
    GUIFontEntry e;
    e.source             = FontSource::CompressedTtf;
    e.compressedData     = data;
    e.compressedDataSize = dataSize;
    e.baseSize           = baseSize;
    CopyGlyphRanges(glyphRanges, e.glyphOwned);
    const GUIFontHandle h{ static_cast<std::uint32_t>(m_guiFonts.size()) };
    m_guiFonts.push_back(std::move(e));
    return h;
}

FontsManager::GamingFontHandle FontsManager::RegisterGamingFont(
    const char* path, float baseSize, const ImWchar* glyphRanges) noexcept
{
    GamingFontEntry e;
    e.source   = FontSource::FileTtf;
    e.path     = path ? path : "";
    e.baseSize = baseSize;
    CopyGlyphRanges(glyphRanges, e.glyphOwned);
    const GamingFontHandle h{ static_cast<std::uint32_t>(m_gamingFonts.size()) };
    m_gamingFonts.push_back(std::move(e));
    return h;
}

FontsManager::GamingFontHandle FontsManager::RegisterGamingFontCompressed(
    const unsigned char* data, unsigned int dataSize,
    float baseSize, const ImWchar* glyphRanges) noexcept
{
    GamingFontEntry e;
    e.source             = FontSource::CompressedTtf;
    e.compressedData     = data;
    e.compressedDataSize = dataSize;
    e.baseSize           = baseSize;
    CopyGlyphRanges(glyphRanges, e.glyphOwned);
    const GamingFontHandle h{ static_cast<std::uint32_t>(m_gamingFonts.size()) };
    m_gamingFonts.push_back(std::move(e));
    return h;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Render-time access
// ─────────────────────────────────────────────────────────────────────────────

ImFont* FontsManager::GetGUI(GUIFontHandle h) const noexcept
{
    const GUIFontEntry* e = guiEntry(h);
    return e ? e->builtFont : nullptr;
}

int FontsManager::GetGUITargetSizePx(GUIFontHandle h) const noexcept
{
    const GUIFontEntry* e = guiEntry(h);
    return e ? guiTargetSizePx(*e) : 0;
}

int FontsManager::GetGUIBuiltSizePx(GUIFontHandle h) const noexcept
{
    const GUIFontEntry* e = guiEntry(h);
    return e ? e->builtSizePx : 0;
}

FontsManager::GamingFontUse FontsManager::GetGaming(GamingFontHandle h) const noexcept
{
    const GamingFontEntry* e = gamingEntry(h);
    if (!e || e->copies.empty())
        return {};

    // Desired size at the current scale (continuous float).
    const float targetPx = e->baseSize * m_pScale->GetScale();

    // Nearest 0.5 grid boundary at or below: floor(targetPx / 0.5) * 0.5
    // Example: targetPx=18.2 → floorPx=18.0; targetPx=18.5 → floorPx=18.5 (exact hit).
    const float floorPx = std::floor(targetPx / kGamingStep) * kGamingStep;

    // Find the last copy with sizePx <= floorPx (i.e. the largest not exceeding it).
    // Linear scan: ~85 copies for baseSize=14, cache-friendly.
    std::size_t bestIdx = 0;
    for (std::size_t i = 1; i < e->copyPx.size(); ++i) {
        if (e->copyPx[i] <= floorPx + 1e-4f)
            bestIdx = i;
        else
            break;  // copyPx is monotonic — no need to search further
    }

    // displayScale = targetPx / builtCopyPx.
    // • Minimum: ~1.0 (exact grid hit).
    // • Maximum: (floorPx + kGamingStep) / floorPx — when approaching the next copy.
    //   For kGamingStep=0.5 and floorPx=14.0 that is 14.5/14.0 ≈ 1.036. Barely noticeable.
    const float builtPx      = e->copyPx[bestIdx];
    const float displayScale = (builtPx > 0.f) ? (targetPx / builtPx) : 1.f;

    return { e->copies[bestIdx], displayScale };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Internal atlas builders
// ─────────────────────────────────────────────────────────────────────────────

bool FontsManager::addGUIFontToAtlas(GUIFontEntry& e, ImGuiIO& io) noexcept
{
    const int   sizePx = guiTargetSizePx(e);
    const float sizeF  = static_cast<float>(sizePx);
    const ImWchar* ranges = e.glyphOwned.empty() ? nullptr : e.glyphOwned.data();

    ImFont* font = nullptr;
    if (e.source == FontSource::CompressedTtf)
        font = io.Fonts->AddFontFromMemoryCompressedTTF(
            e.compressedData, static_cast<int>(e.compressedDataSize),
            sizeF, nullptr, ranges);
    else
        font = io.Fonts->AddFontFromFileTTF(e.path.c_str(), sizeF, nullptr, ranges);

    if (!font) {
        const std::string msg = (e.source == FontSource::CompressedTtf)
            ? std::string("[FontsManager] GUIFont: AddFromMemory failed, size=") + std::to_string(sizePx)
            : std::string("[FontsManager] GUIFont: AddFromFile failed (") + e.path + "), size=" + std::to_string(sizePx);
        std::cerr << msg << "\n";
        OutputDebugStringA((msg + "\n").c_str());
        return false;
    }

    e.builtFont   = font;
    e.builtSizePx = sizePx;
    return true;
}

bool FontsManager::addGamingFontToAtlas(GamingFontEntry& e, ImGuiIO& io) noexcept
{
    e.copies.clear();
    e.copyPx.clear();

    const ImWchar* ranges = e.glyphOwned.empty() ? nullptr : e.glyphOwned.data();

    const float minPx = e.baseSize * kScaleMin;
    const float maxPx = e.baseSize * kScaleMax;

    // Copy count: N = floor((maxPx - minPx) / step) + 1
    // Use an int step counter to avoid float accumulation error.
    // Example (baseSize=14): minPx=14.0, maxPx=56.0, step=0.5 → 85 copies.
    const int steps = static_cast<int>(
        std::floor((maxPx - minPx) / kGamingStep + 1e-4f)) + 1;

    e.copies.reserve(static_cast<std::size_t>(steps));
    e.copyPx.reserve(static_cast<std::size_t>(steps));

    for (int i = 0; i < steps; ++i)
    {
        // sizePx is an exact multiple of 0.5; the integer counter guarantees this.
        const float sizePx = minPx + static_cast<float>(i) * kGamingStep;

        ImFont* font = nullptr;
        if (e.source == FontSource::CompressedTtf)
            font = io.Fonts->AddFontFromMemoryCompressedTTF(
                e.compressedData, static_cast<int>(e.compressedDataSize),
                sizePx, nullptr, ranges);
        else
            font = io.Fonts->AddFontFromFileTTF(e.path.c_str(), sizePx, nullptr, ranges);

        if (!font) {
            const std::string msg = (e.source == FontSource::CompressedTtf)
                ? std::string("[FontsManager] GamingFont: AddFromMemory failed, size=") + std::to_string(sizePx)
                : std::string("[FontsManager] GamingFont: AddFromFile failed (") + e.path + "), size=" + std::to_string(sizePx);
            std::cerr << msg << "\n";
            OutputDebugStringA((msg + "\n").c_str());
            return false;
        }

        e.copies.push_back(font);
        e.copyPx.push_back(sizePx);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Atlas lifecycle
// ─────────────────────────────────────────────────────────────────────────────

bool FontsManager::RebuildIfNeeded() noexcept
{
    if (!m_pScale)
        return false;

    // Check each GUIFont: did the ceil-size change since the last build?
    // GamingFont is skipped — its full copy set is fixed; scale only affects
    // copy selection in GetGaming(), with no atlas rebuild.
    for (const GUIFontEntry& e : m_guiFonts) {
        if (guiTargetSizePx(e) != e.builtSizePx)
            return BuildAtlas();
    }
    return false;
}

bool FontsManager::BuildAtlas() noexcept
{
    if (!m_pScale)
        return false;

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    io.Fonts->FontLoaderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.Fonts->Clear();

    // ── GamingFont: full 0.5-px copy set ─────────────────────────────────────
    // Build first — they occupy most of the atlas and keep a stable order.
    for (GamingFontEntry& e : m_gamingFonts)
        if (!addGamingFontToAtlas(e, io)) return false;

    // ── GUIFont: one copy at the current ceil size ────────────────────────────
    // Build after GamingFont; on scale change the old copy is dropped with the
    // previous atlas contents (io.Fonts->Clear() above).
    for (GUIFontEntry& e : m_guiFonts)
        if (!addGUIFontToAtlas(e, io)) return false;

    if (!io.Fonts->Build())
        return false;

    // ImGui 1.92+: FontSizeBase controls the default rendering size for ImGui::Text() etc.
    // Sync it to the first GUI font's target size so text scales with ScaleManager.
    if (!m_guiFonts.empty())
        ImGui::GetStyle().FontSizeBase = static_cast<float>(guiTargetSizePx(m_guiFonts[0]));

    if (m_atlasInvalidate)
        m_atlasInvalidate();

    return true;
}
