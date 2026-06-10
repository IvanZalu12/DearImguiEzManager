#pragma once

#ifdef WIN32
#  include <Windows.h>
   // Normal           — standard OS window with native title bar and _ □ ✖ buttons.
   // OverlayTransparent — borderless, layered, click-through-capable overlay.
   // CustomChrome      — borderless window whose title bar and _ □ ✖ buttons are drawn
   //                     by ImGui, while resize / Aero Snap / maximize / drop shadow keep
   //                     working through proper non-client message handling.
   enum class ManagedWindowStyle { Normal, OverlayTransparent, CustomChrome };
#endif

class GfxManager {
protected:
    using RenderCallbackFn   = void(*)() noexcept;
    using PreFrameCallbackFn = void(*)() noexcept;
    using MenuInitCallbackFn = void(*)();

    RenderCallbackFn   m_pRenderCallback   = nullptr;
    PreFrameCallbackFn m_pPreFrameCallback = nullptr;
    MenuInitCallbackFn m_pMenuInitCallback = nullptr;

    // Guards against re-entering RenderFrame (e.g. a synchronous WM_SIZE dispatched
    // while a frame is already open would otherwise trip ImGui's NewFrame assert).
    bool               m_rendering         = false;

#ifdef WIN32
    HWND               m_hwnd            = nullptr;
    HINSTANCE          m_hInstance       = nullptr;
    wchar_t            m_className[256]  = {};
    bool               m_classRegistered = false;
    ManagedWindowStyle m_windowStyle     = ManagedWindowStyle::Normal;

    // ── CustomChrome state ──────────────────────────────────────────────────
    float m_titleBarHeight  = 32.0f;   // logical height of the draggable title bar (px)
    bool  m_titleBarHovered = false;   // refreshed each frame by the title-bar renderer

    static LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM) noexcept;
    LRESULT HandleCustomChromeMessage(HWND, UINT, WPARAM, LPARAM, bool& handled) noexcept;

    bool RegisterWindowClass(HINSTANCE, const wchar_t* className) noexcept;
    bool CreateManagedWindow(HINSTANCE, const wchar_t* title,
                             int x, int y, int w, int h, ManagedWindowStyle) noexcept;
    void DestroyManagedWindow() noexcept;
#endif

    // Called once by Create(); nativeHandle is HWND on Win32, nullptr on Android
    virtual bool InitializeGraphics(void* nativeHandle, int w, int h) noexcept = 0;

public:
    GfxManager() noexcept = default;
    virtual ~GfxManager() noexcept;

    GfxManager(const GfxManager&)            = delete;
    GfxManager& operator=(const GfxManager&) = delete;

    void SetRenderCallback(RenderCallbackFn cb) noexcept     { m_pRenderCallback   = cb; }
    void SetPreFrameCallback(PreFrameCallbackFn cb) noexcept { m_pPreFrameCallback = cb; }
    void SetMenuInitCallback(MenuInitCallbackFn cb) noexcept { m_pMenuInitCallback = cb; }

#ifdef WIN32
    // Auto-selects D3D11 with fallback to D3D9
    static GfxManager* Create(HINSTANCE instance, int w, int h, const wchar_t* title,
                               ManagedWindowStyle style = ManagedWindowStyle::Normal) noexcept;

    void ShowManagedWindow(int nCmdShow = SW_SHOWDEFAULT) noexcept;
    void SetWindowAlpha(BYTE alpha) noexcept;
    void SetClickThrough(bool enabled) noexcept;
    UINT HandleWMSG() noexcept;
    HWND GetHWND() const noexcept { return m_hwnd; }

    // Top-left of the render client area in screen pixels. Feed to LayoutManager::BeginFrame
    // so floating windows keep their on-screen position across maximize/restore.
    POINT GetClientScreenOrigin() const noexcept;

    // ── CustomChrome window controls (no-ops for other styles) ──────────────
    // Logical height of the draggable title bar in pixels. Scale it yourself if
    // your UI is DPI-scaled (e.g. via ScaleManager). Default: 32.
    void  SetTitleBarHeight(float px) noexcept   { m_titleBarHeight = px; }
    float GetTitleBarHeight() const noexcept     { return m_titleBarHeight; }

    bool  IsWindowMaximized() const noexcept;
    void  MinimizeWindow() noexcept;
    void  ToggleMaximizeWindow() noexcept;
    void  CloseWindow() noexcept;                // posts WM_CLOSE → quits the loop

    // Draws the built-in title bar (title text + _ □ ✖ buttons) for a CustomChrome
    // window. Call once at the very top of your render callback, inside the ImGui
    // frame. Returns the title bar height in pixels so you can offset your content.
    // The buttons are wired to minimize / maximize-restore / close automatically and
    // empty title-bar space drags the window through native non-client handling.
    float RenderDefaultTitleBar(const char* title) noexcept;

    // Lower-level hook for hand-rolled title bars: report whether the cursor is over
    // a draggable (non-interactive) region of your title bar this frame, so the OS
    // can move/snap the window. Pair with a title bar you draw yourself.
    void  SetTitleBarDragHovered(bool hovered) noexcept { m_titleBarHovered = hovered; }
#else
    static GfxManager* Create(int w, int h) noexcept;
#endif

    virtual void Shutdown() noexcept {}
    virtual void RenderFrame() noexcept {}
    virtual void OnResize(int w, int h) noexcept {}
    virtual void RefreshImGuiFontTexture() noexcept = 0;
};
