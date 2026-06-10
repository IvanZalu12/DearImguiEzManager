#include <DearImguiEzManager/gfxManager/gfxManager.hpp>

// ============================================================
// Android path
// ============================================================
#ifndef WIN32
#include <DearImguiEzManager/gfxManager/androidOpenGLManager.hpp>

GfxManager* GfxManager::Create(int w, int h) noexcept
{
    auto* mgr = new AndroidOpenGLManager();
    if (!mgr->InitializeGraphics(nullptr, w, h)) {
        delete mgr;
        return nullptr;
    }
    return mgr;
}

GfxManager::~GfxManager() noexcept {}

// ============================================================
// Windows / D3D path
// ============================================================
#else
#include <DearImguiEzManager/d3d/d3d11Manager.hpp>
#include <DearImguiEzManager/d3d/d3d9Manager.hpp>
#include <d3d11.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <iostream>
#include <imgui.h>
#include <imgui_impl_win32.h>
#pragma comment(lib, "dwmapi.lib")
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

GfxManager::~GfxManager() noexcept
{
    DestroyManagedWindow();
}

bool GfxManager::RegisterWindowClass(HINSTANCE instance, const wchar_t* className) noexcept
{
    if (!instance || !className || !className[0])
        return false;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_CLASSDC;
    wc.lpfnWndProc   = WndProcThunk;
    wc.hInstance     = instance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = className;

    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    wcsncpy_s(m_className, className, _TRUNCATE);
    m_hInstance       = instance;
    m_classRegistered = true;
    return true;
}

bool GfxManager::CreateManagedWindow(HINSTANCE instance, const wchar_t* title,
    int x, int y, int w, int h, ManagedWindowStyle style) noexcept
{
    if (!m_classRegistered || m_hwnd || instance != m_hInstance)
        return false;

    m_windowStyle = style;

    DWORD exStyle = 0;
    DWORD styleWs = WS_OVERLAPPEDWINDOW;
    if (style == ManagedWindowStyle::OverlayTransparent) {
        exStyle = WS_EX_LAYERED | WS_EX_TOPMOST;
        styleWs = WS_POPUP;
    }
    // CustomChrome keeps WS_OVERLAPPEDWINDOW (WS_THICKFRAME + WS_CAPTION + min/max/sys)
    // so the OS still drives resize, Aero Snap, maximize and the drop shadow. The visible
    // frame is removed in WM_NCCALCSIZE and re-implemented through WM_NCHITTEST.

    m_hwnd = CreateWindowExW(exStyle, m_className, title ? title : L"",
        styleWs, x, y, w, h, nullptr, nullptr, m_hInstance, nullptr);
    if (!m_hwnd)
        return false;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    if (style == ManagedWindowStyle::OverlayTransparent)
        SetLayeredWindowAttributes(m_hwnd, 0, 240, LWA_ALPHA);

    if (style == ManagedWindowStyle::CustomChrome) {
        // A 1px bottom margin keeps the standard window drop shadow while the client
        // area still covers the whole window (see WM_NCCALCSIZE).
        BOOL composition = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&composition)) && composition) {
            MARGINS m{ 0, 0, 0, 1 };
            DwmExtendFrameIntoClientArea(m_hwnd, &m);
        }
        // Force a non-client recalculation so our stripped frame takes effect now.
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return true;
}

void GfxManager::DestroyManagedWindow() noexcept
{
    if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    if (m_classRegistered && m_className[0] && m_hInstance) {
        UnregisterClassW(m_className, m_hInstance);
        m_classRegistered = false;
        m_className[0]    = L'\0';
    }
}

void GfxManager::ShowManagedWindow(int nCmdShow) noexcept
{
    if (m_hwnd) { ShowWindow(m_hwnd, nCmdShow); UpdateWindow(m_hwnd); }
}

void GfxManager::SetWindowAlpha(BYTE alpha) noexcept
{
    if (m_hwnd) SetLayeredWindowAttributes(m_hwnd, 0, alpha, LWA_ALPHA);
}

void GfxManager::SetClickThrough(bool enabled) noexcept
{
    if (!m_hwnd) return;
    LONG ex = GetWindowLongW(m_hwnd, GWL_EXSTYLE);
    SetWindowLongW(m_hwnd, GWL_EXSTYLE, enabled ? (ex | WS_EX_TRANSPARENT) : (ex & ~WS_EX_TRANSPARENT));
}

// ── CustomChrome window controls ───────────────────────────────────────────
bool GfxManager::IsWindowMaximized() const noexcept
{
    return m_hwnd && IsZoomed(m_hwnd);
}

void GfxManager::MinimizeWindow() noexcept
{
    if (m_hwnd) ShowWindow(m_hwnd, SW_MINIMIZE);
}

void GfxManager::ToggleMaximizeWindow() noexcept
{
    if (m_hwnd) ShowWindow(m_hwnd, IsZoomed(m_hwnd) ? SW_RESTORE : SW_MAXIMIZE);
}

void GfxManager::CloseWindow() noexcept
{
    if (m_hwnd) PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
}

float GfxManager::RenderDefaultTitleBar(const char* title) noexcept
{
    const float h = m_titleBarHeight;
    if (m_windowStyle != ManagedWindowStyle::CustomChrome || h <= 0.0f || !m_hwnd)
        return 0.0f;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    const bool focused = (GetForegroundWindow() == m_hwnd);

    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.0f, 0.0f));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration   | ImGuiWindowFlags_NoMove          |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("##EzMgrTitleBar", nullptr, flags);

    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const ImVec2 p0  = vp->Pos;
    const ImVec2 p1  = ImVec2(p0.x + vp->Size.x, p0.y + h);
    const ImU32  bg  = ImGui::GetColorU32(focused ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
    dl->AddRectFilled(p0, p1, bg);

    // Title text, vertically centered on the left.
    if (title && title[0]) {
        const ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(p0.x + h * 0.4f, p0.y + (h - ts.y) * 0.5f),
                    ImGui::GetColorU32(ImGuiCol_Text), title);
    }

    // Caption buttons (right-aligned): minimize, maximize/restore, close.
    const float  btnW    = h * 1.5f;
    const ImU32  glyphCol = ImGui::GetColorU32(focused ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    bool anyButtonHovered = false;

    // One caption button: invisible hit area + hover highlight + glyph drawn by `draw`.
    auto captionButton = [&](const char* id, float slotFromRight,
                             ImU32 hoverCol, auto&& draw) -> bool {
        const ImVec2 min(p1.x - btnW * slotFromRight, p0.y);
        ImGui::SetCursorScreenPos(min);
        const bool clicked = ImGui::InvisibleButton(id, ImVec2(btnW, h));
        const bool hovered = ImGui::IsItemHovered();
        const bool active  = ImGui::IsItemActive();
        if (hovered) anyButtonHovered = true;
        if (hovered || active) {
            ImU32 c = hoverCol;
            if (active) {  // slightly darker while pressed
                ImVec4 cv = ImGui::ColorConvertU32ToFloat4(hoverCol);
                cv.x *= 0.8f; cv.y *= 0.8f; cv.z *= 0.8f;
                c = ImGui::ColorConvertFloat4ToU32(cv);
            }
            dl->AddRectFilled(min, ImVec2(min.x + btnW, min.y + h), c);
        }
        const ImVec2 c{ min.x + btnW * 0.5f, min.y + h * 0.5f };
        draw(c, hovered);
        return clicked;
    };

    const float   g       = h * 0.18f;                 // half glyph size
    const ImU32   neutral = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    const ImU32   closeBg = IM_COL32(232, 17, 35, 255); // Windows-style red

    // Minimize  ─  (slot 3 from the right)
    if (captionButton("##min", 3.0f, neutral, [&](ImVec2 c, bool) {
            dl->AddLine(ImVec2(c.x - g, c.y), ImVec2(c.x + g, c.y), glyphCol, 1.0f);
        }))
        MinimizeWindow();

    // Maximize / Restore  □  (slot 2)
    const bool maximized = IsZoomed(m_hwnd);
    if (captionButton("##max", 2.0f, neutral, [&](ImVec2 c, bool) {
            if (maximized) {  // restore: two overlapping squares
                const float o = g * 0.45f;
                dl->AddRect(ImVec2(c.x - g + o, c.y - g - o), ImVec2(c.x + g + o, c.y + g - o), glyphCol, 0.0f, 0, 1.0f);
                dl->AddRectFilled(ImVec2(c.x - g - o, c.y - g + o), ImVec2(c.x + g - o, c.y + g + o), bg);
                dl->AddRect(ImVec2(c.x - g - o, c.y - g + o), ImVec2(c.x + g - o, c.y + g + o), glyphCol, 0.0f, 0, 1.0f);
            } else {          // maximize: single square
                dl->AddRect(ImVec2(c.x - g, c.y - g), ImVec2(c.x + g, c.y + g), glyphCol, 0.0f, 0, 1.0f);
            }
        }))
        ToggleMaximizeWindow();

    // Close  ✖  (slot 1) — red hover, white glyph on hover.
    if (captionButton("##close", 1.0f, closeBg, [&](ImVec2 c, bool hovered) {
            const ImU32 col = hovered ? IM_COL32_WHITE : glyphCol;
            dl->AddLine(ImVec2(c.x - g, c.y - g), ImVec2(c.x + g, c.y + g), col, 1.0f);
            dl->AddLine(ImVec2(c.x - g, c.y + g), ImVec2(c.x + g, c.y - g), col, 1.0f);
        }))
        CloseWindow();

    // Tell WM_NCHITTEST whether the empty title-bar area is draggable this frame.
    m_titleBarHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && !anyButtonHovered;

    ImGui::End();
    ImGui::PopStyleVar(3);
    return h;
}

UINT GfxManager::HandleWMSG() noexcept
{
    MSG msg{};
    UINT last = 0;
    while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        last = msg.message;
        if (msg.message == WM_QUIT) return WM_QUIT;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return last;
}

// Returns the per-monitor resize border thickness (frame + padded border) in pixels.
static POINT FrameBorderThickness() noexcept
{
    return POINT{
        GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER),
        GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER),
    };
}

// Borderless-but-functional window: strips the visual non-client frame while keeping
// resize / Aero Snap / maximize / drop shadow alive. Sets `handled` when it consumes msg.
LRESULT GfxManager::HandleCustomChromeMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                              bool& handled) noexcept
{
    handled = true;
    switch (msg) {
    case WM_NCCALCSIZE:
        // wp == TRUE: shrink the proposed window rect into the client rect. Returning 0
        // with no adjustment makes the client area span the entire window (no frame).
        // Maximize is constrained to the work area in WM_GETMINMAXINFO, so the maximized
        // client lines up with the monitor and nothing spills off-screen.
        if (wp == TRUE)
            return 0;
        break;

    case WM_GETMINMAXINFO: {
        // Constrain maximize to the monitor work area so the taskbar stays visible and
        // the borderless window doesn't bleed past the monitor edges.
        HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi{ sizeof(mi) };
        if (mon && GetMonitorInfoW(mon, &mi)) {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            const RECT& work = mi.rcWork;
            const RECT& full = mi.rcMonitor;
            mmi->ptMaxPosition.x = work.left - full.left;
            mmi->ptMaxPosition.y = work.top  - full.top;
            mmi->ptMaxSize.x     = work.right  - work.left;
            mmi->ptMaxSize.y     = work.bottom - work.top;
            return 0;
        }
        break;
    }

    case WM_NCHITTEST: {
        const POINT cursor{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        RECT win{};
        GetWindowRect(hwnd, &win);

        // Resize borders (only when not maximized — a maximized window can't be resized).
        if (!IsZoomed(hwnd)) {
            const POINT b = FrameBorderThickness();
            enum { kClient = 0, kLeft = 1, kRight = 2, kTop = 4, kBottom = 8 };
            const int region =
                  (cursor.x <  win.left   + b.x ? kLeft   : 0)
                | (cursor.x >= win.right  - b.x ? kRight  : 0)
                | (cursor.y <  win.top    + b.y ? kTop    : 0)
                | (cursor.y >= win.bottom - b.y ? kBottom : 0);
            switch (region) {
            case kTop | kLeft:     return HTTOPLEFT;
            case kTop | kRight:    return HTTOPRIGHT;
            case kBottom | kLeft:  return HTBOTTOMLEFT;
            case kBottom | kRight: return HTBOTTOMRIGHT;
            case kLeft:            return HTLEFT;
            case kRight:           return HTRIGHT;
            case kTop:             return HTTOP;
            case kBottom:          return HTBOTTOM;
            default:               break;
            }
        }
        // Inside the title-bar band: draggable empty space → HTCAPTION (move + snap +
        // double-click maximize). Over a button/widget the renderer clears the flag so
        // the click reaches ImGui through HTCLIENT.
        if (cursor.y < win.top + static_cast<int>(m_titleBarHeight) && m_titleBarHovered)
            return HTCAPTION;
        return HTCLIENT;
    }

    case WM_DWMCOMPOSITIONCHANGED: {
        BOOL composition = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&composition)) && composition) {
            MARGINS m{ 0, 0, 0, 1 };
            DwmExtendFrameIntoClientArea(hwnd, &m);
        }
        return 0;
    }
    }
    handled = false;
    return 0;
}

LRESULT CALLBACK GfxManager::WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
{
    auto* self = reinterpret_cast<GfxManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (self && self->m_windowStyle == ManagedWindowStyle::CustomChrome) {
        bool handled = false;
        LRESULT r = self->HandleCustomChromeMessage(hwnd, msg, wp, lp, handled);
        if (handled)
            return r;
    }

    if (ImGui::GetCurrentContext() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (self && wp != SIZE_MINIMIZED)
            self->OnResize(LOWORD(lp), HIWORD(lp));
        return 0;
    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static bool ProbeWorkingD3D11() noexcept
{
    HMODULE mod = LoadLibraryW(L"d3d11.dll");
    if (!mod) return false;

    using PFN = HRESULT(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
        const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
    auto pfn = reinterpret_cast<PFN>(GetProcAddress(mod, "D3D11CreateDevice"));
    bool ok  = false;
    if (pfn) {
        ID3D11Device* dev{}; ID3D11DeviceContext* ctx{}; D3D_FEATURE_LEVEL fl{};
        ok = SUCCEEDED(pfn(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                           nullptr, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx));
        if (ctx) ctx->Release();
        if (dev) dev->Release();
    }
    FreeLibrary(mod);
    return ok;
}

GfxManager* GfxManager::Create(HINSTANCE instance, int w, int h, const wchar_t* title,
                                ManagedWindowStyle style) noexcept
{
    const wchar_t* kClass = L"DearImguiEzMgrWnd";
    const int posX = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    const int posY = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    auto tryInit = [&](GfxManager* mgr) -> bool {
        if (!mgr->RegisterWindowClass(instance, kClass) ||
            !mgr->CreateManagedWindow(instance, title, posX, posY, w, h, style))
            return false;
        RECT rc{};
        GetClientRect(mgr->m_hwnd, &rc);
        return mgr->InitializeGraphics(mgr->m_hwnd, rc.right - rc.left, rc.bottom - rc.top);
    };

    if (ProbeWorkingD3D11()) {
        auto* d11 = new D3D11Manager();
        if (tryInit(d11)) {
            std::cout << "[GfxManager] Using D3D11\n";
            return d11;
        }
        std::cout << "[GfxManager] D3D11 failed, falling back to D3D9\n";
        delete d11;
    }

    auto* d9 = new D3D9Manager();
    if (tryInit(d9)) {
        std::cout << "[GfxManager] Using D3D9\n";
        return d9;
    }
    std::cout << "[GfxManager] D3D9 failed\n";
    delete d9;
    return nullptr;
}
#endif // WIN32
