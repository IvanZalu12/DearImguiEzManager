#pragma once

#include <DearImguiEzManager/MenuManager/ScaleManager.hpp>
#include <imgui.h>

#include <cstdint>
#include <string>
#include <vector>

/// FontsManager — two independent font pools with different rasterization strategies.
///
/// ┌─ GUIFont ────────────────────────────────────────────────────────────────┐
/// │  Lazy rebuild: the atlas stores exactly ONE copy per font.               │
/// │  Target size = ceil(baseSize × scale) — advance to the next int when     │
/// │  crossing a boundary. RebuildIfNeeded() checks this condition.           │
/// │  Scale changes are usually rare (once per monitor); other copies are     │
/// │  discarded on every atlas rebuild.                                       │
/// └──────────────────────────────────────────────────────────────────────────┘
///
/// ┌─ GamingFont ─────────────────────────────────────────────────────────────┐
/// │  All copies are built at startup: step kGamingStep (0.5 px), range       │
/// │  [baseSize×scaleMin .. baseSize×scaleMax].                               │
/// │  GetGaming() returns the nearest copy at or below + displayScale, which  │
/// │  grows smoothly toward the next copy. The caller sets font->Scale before │
/// │  PushFont and resets it to 1.f after PopFont.                            │
/// │  FreeType with auto-hinting gives smooth enough results at half-pixel    │
/// │  sizes.                                                                  │
/// └──────────────────────────────────────────────────────────────────────────┘
class FontsManager {
public:
    using AtlasInvalidateCallback = void (*)() noexcept;

    // ── Range constants ───────────────────────────────────────────────────────
    static constexpr float kScaleMin   = 1.0f;
    static constexpr float kScaleMax   = 4.0f;
    /// GamingFont rasterization step in absolute pixels.
    static constexpr float kGamingStep = 0.5f;

    // ── Type-safe handles ─────────────────────────────────────────────────────
    struct GUIFontHandle {
        std::uint32_t id{};
        bool operator==(const GUIFontHandle& o) const noexcept { return id == o.id; }
        bool operator!=(const GUIFontHandle& o) const noexcept { return id != o.id; }
    };
    struct GamingFontHandle {
        std::uint32_t id{};
        bool operator==(const GamingFontHandle& o) const noexcept { return id == o.id; }
        bool operator!=(const GamingFontHandle& o) const noexcept { return id != o.id; }
    };

    /// Result of GetGaming(). Usage:
    ///   auto use = fonts.GetGaming(hNick);
    ///   if (use.valid()) {
    ///       use.font->Scale = use.displayScale;
    ///       ImGui::PushFont(use.font);
    ///       /* render */
    ///       ImGui::PopFont();
    ///       use.font->Scale = 1.f;   // restore for other users
    ///   }
    struct GamingFontUse {
        ImFont* font         = nullptr;
        /// targetPx / builtCopyPx — always >= 1.0.
        /// Grows smoothly up to (builtCopyPx + kGamingStep) / builtCopyPx,
        /// then GetGaming() switches to the next copy and scale returns to ~1.0.
        float   displayScale = 1.f;

        bool valid() const noexcept { return font != nullptr; }
    };

    // ── Constructor ───────────────────────────────────────────────────────────
    explicit FontsManager(ScaleManager* scale) noexcept;

    /// D3D-independent atlas invalidation hook (e.g. texture reset in D3D9/D3D11).
    /// Set from DearImguiEzManager.
    void SetAtlasInvalidateCallback(AtlasInvalidateCallback cb) noexcept { m_atlasInvalidate = cb; }

    // ── GUIFont registration ──────────────────────────────────────────────────
    [[nodiscard]] GUIFontHandle RegisterGUIFont(
        const char* path, float baseSize,
        const ImWchar* glyphRanges = nullptr) noexcept;

    [[nodiscard]] GUIFontHandle RegisterGUIFontCompressed(
        const unsigned char* compressedData, unsigned int compressedDataSize,
        float baseSize, const ImWchar* glyphRanges = nullptr) noexcept;

    // ── GamingFont registration ───────────────────────────────────────────────
    [[nodiscard]] GamingFontHandle RegisterGamingFont(
        const char* path, float baseSize,
        const ImWchar* glyphRanges = nullptr) noexcept;

    [[nodiscard]] GamingFontHandle RegisterGamingFontCompressed(
        const unsigned char* compressedData, unsigned int compressedDataSize,
        float baseSize, const ImWchar* glyphRanges = nullptr) noexcept;

    // ── Render-time access ────────────────────────────────────────────────────

    /// Returns the single built GUIFont copy (nullptr until the first BuildAtlas).
    ImFont* GetGUI(GUIFontHandle h) const noexcept;

    /// Target pixel size of a GUIFont at the current scale (lround(baseSize * scale)).
    int GetGUITargetSizePx(GUIFontHandle h) const noexcept;
    /// Pixel size used at the last BuildAtlas (0 before the first build).
    int GetGUIBuiltSizePx(GUIFontHandle h) const noexcept;

    /// Returns the nearest rasterized GamingFont copy at or below + displayScale.
    /// Returns GamingFontUse{} if the atlas has not been built yet.
    GamingFontUse GetGaming(GamingFontHandle h) const noexcept;

    // ── Atlas lifecycle ───────────────────────────────────────────────────────

    /// Checks whether the ceil-size of any GUIFont crossed an int boundary since
    /// the last build. If so, calls BuildAtlas() and returns true.
    /// Call after every ScaleManager::SetScale() (e.g. from DearImguiEzManager).
    bool RebuildIfNeeded() noexcept;

    /// Full atlas (re)build:
    ///   • GamingFont — all 0.5-px copies from min to max (once at startup / on registration change)
    ///   • GUIFont    — one copy at the current ceil(base×scale)
    /// Invokes AtlasInvalidateCallback when finished.
    bool BuildAtlas() noexcept;

private:
    enum class FontSource : std::uint8_t { FileTtf, CompressedTtf };

    static void CopyGlyphRanges(const ImWchar* src, std::vector<ImWchar>& out) noexcept;

    // ── Internal descriptors ──────────────────────────────────────────────────

    struct GUIFontEntry {
        FontSource           source             = FontSource::FileTtf;
        std::string          path;
        const unsigned char* compressedData     = nullptr;
        unsigned int         compressedDataSize = 0;
        float                baseSize           = 0.f;
        std::vector<ImWchar> glyphOwned;

        /// The only live copy in the atlas (nullptr until the first BuildAtlas).
        ImFont* builtFont   = nullptr;
        /// ceil(baseSize×scale) at the last build; used to detect changes.
        int     builtSizePx = 0;
    };

    struct GamingFontEntry {
        FontSource           source             = FontSource::FileTtf;
        std::string          path;
        const unsigned char* compressedData     = nullptr;
        unsigned int         compressedDataSize = 0;
        float                baseSize           = 0.f;
        std::vector<ImWchar> glyphOwned;

        /// copies[i] was rasterized at size copyPx[i].
        /// copyPx[0] = baseSize*kScaleMin, step kGamingStep.
        std::vector<ImFont*> copies;
        std::vector<float>   copyPx;
    };

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Target int size of a GUIFont at the current scale.
    int guiTargetSizePx(const GUIFontEntry& e) const noexcept;

    bool addGUIFontToAtlas   (GUIFontEntry&    e, ImGuiIO& io) noexcept;
    bool addGamingFontToAtlas(GamingFontEntry& e, ImGuiIO& io) noexcept;

    const GUIFontEntry*    guiEntry   (GUIFontHandle    h) const noexcept;
    const GamingFontEntry* gamingEntry(GamingFontHandle h) const noexcept;

    // ── Data ──────────────────────────────────────────────────────────────────
    ScaleManager*                m_pScale;
    std::vector<GUIFontEntry>    m_guiFonts;
    std::vector<GamingFontEntry> m_gamingFonts;
    AtlasInvalidateCallback      m_atlasInvalidate = nullptr;
};
