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
#include <iostream>
#include <imgui.h>
#include <imgui_impl_win32.h>
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

    m_hwnd = CreateWindowExW(exStyle, m_className, title ? title : L"",
        styleWs, x, y, w, h, nullptr, nullptr, m_hInstance, nullptr);
    if (!m_hwnd)
        return false;

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    if (style == ManagedWindowStyle::OverlayTransparent)
        SetLayeredWindowAttributes(m_hwnd, 0, 240, LWA_ALPHA);
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

LRESULT CALLBACK GfxManager::WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
{
    auto* self = reinterpret_cast<GfxManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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
