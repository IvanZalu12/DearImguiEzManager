#pragma once

#ifdef WIN32
#  include <Windows.h>
   enum class ManagedWindowStyle { Normal, OverlayTransparent };
#endif

class GfxManager {
protected:
    using RenderCallbackFn   = void(*)() noexcept;
    using PreFrameCallbackFn = void(*)() noexcept;
    using MenuInitCallbackFn = void(*)();

    RenderCallbackFn   m_pRenderCallback   = nullptr;
    PreFrameCallbackFn m_pPreFrameCallback = nullptr;
    MenuInitCallbackFn m_pMenuInitCallback = nullptr;

#ifdef WIN32
    HWND               m_hwnd            = nullptr;
    HINSTANCE          m_hInstance       = nullptr;
    wchar_t            m_className[256]  = {};
    bool               m_classRegistered = false;
    ManagedWindowStyle m_windowStyle     = ManagedWindowStyle::Normal;

    static LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM) noexcept;

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
#else
    static GfxManager* Create(int w, int h) noexcept;
#endif

    virtual void Shutdown() noexcept {}
    virtual void RenderFrame() noexcept {}
    virtual void OnResize(int w, int h) noexcept {}
    virtual void RefreshImGuiFontTexture() noexcept = 0;
};
