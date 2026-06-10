#include <DearImguiEzManager/d3d/d3d9Manager.hpp>
#include <cstring>
#include <iostream>
#include <DearImguiEzManager/MenuManager/ImGuiStyleColors.hpp>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#define LOG(x) std::cout << x << std::endl

D3D9Manager::~D3D9Manager() noexcept
{
    Shutdown();
}

bool D3D9Manager::InitializeGraphics(void* /*unused*/, int width, int height) noexcept
{
    if (!m_hwnd)
        return false;

    LOG("[D3D9Manager] Initializing...");

    if (!CreateDeviceInternal(width, height)) {
        LOG("[D3D9Manager] Device creation failed");
        return false;
    }
    if (!InitializeImGui()) {
        LOG("[D3D9Manager] ImGui init failed");
        Shutdown();
        return false;
    }

    LOG("[D3D9Manager] Ready");
    return true;
}

void D3D9Manager::Shutdown() noexcept
{
    ShutdownImGui();
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }
    if (m_pD3D)    { m_pD3D->Release();    m_pD3D    = nullptr; }
    std::memset(&m_pp, 0, sizeof(m_pp));
    m_backbufW = 0;
    m_backbufH = 0;
}

void D3D9Manager::RenderFrame() noexcept
{
    if (!m_pDevice || !m_bImGuiInit)
        return;

    HRESULT coop = m_pDevice->TestCooperativeLevel();
    if (coop == D3DERR_DEVICELOST)
        return;
    if (coop == D3DERR_DEVICENOTRESET && !TryResetDevice())
        return;

    if (m_pPreFrameCallback)
        m_pPreFrameCallback();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_pRenderCallback)
        m_pRenderCallback();

    ImGui::Render();

    m_pDevice->SetRenderState(D3DRS_ZENABLE,          FALSE);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    m_pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_RGBA(25, 25, 25, 255), 1.0f, 0);

    if (m_pDevice->BeginScene() >= 0) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        m_pDevice->EndScene();
    }

    HRESULT pr = m_pDevice->Present(nullptr, nullptr, nullptr, nullptr);
    if (pr == D3DERR_DEVICELOST || pr == D3DERR_DEVICENOTRESET)
        TryResetDevice();
}

void D3D9Manager::OnResize(int width, int height) noexcept
{
    if (!m_pDevice || width <= 0 || height <= 0)
        return;
    m_backbufW = width;
    m_backbufH = height;
    m_pp.BackBufferWidth  = (UINT)width;
    m_pp.BackBufferHeight = (UINT)height;
    if (m_bImGuiInit) ImGui_ImplDX9_InvalidateDeviceObjects();
    if (SUCCEEDED(m_pDevice->Reset(&m_pp)) && m_bImGuiInit)
        ImGui_ImplDX9_CreateDeviceObjects();
}

void D3D9Manager::RefreshImGuiFontTexture() noexcept
{
    if (!m_pDevice || !ImGui::GetCurrentContext()) return;
    ImGui_ImplDX9_InvalidateDeviceObjects();
    ImGui_ImplDX9_CreateDeviceObjects();
}

bool D3D9Manager::CreateDeviceInternal(int width, int height) noexcept
{
    if (!m_hwnd || width <= 0 || height <= 0 || m_pD3D)
        return false;

    m_backbufW = width;
    m_backbufH = height;

    m_pp.Windowed               = TRUE;
    m_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    m_pp.hDeviceWindow          = m_hwnd;
    m_pp.BackBufferWidth        = (UINT)width;
    m_pp.BackBufferHeight       = (UINT)height;
    m_pp.BackBufferFormat       = D3DFMT_UNKNOWN;
    m_pp.EnableAutoDepthStencil = FALSE;
    m_pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

    HMODULE hMod = LoadLibraryW(L"d3d9.dll");
    if (!hMod) {
        LOG("[D3D9Manager] d3d9.dll not found");
        return false;
    }

    using PFN = IDirect3D9*(WINAPI*)(UINT);
    auto pfn = reinterpret_cast<PFN>(GetProcAddress(hMod, "Direct3DCreate9"));
    if (pfn) m_pD3D = pfn(D3D_SDK_VERSION);
    FreeLibrary(hMod);

    if (!m_pD3D) {
        LOG("[D3D9Manager] Direct3DCreate9 failed");
        return false;
    }

    HRESULT hr = m_pD3D->CreateDevice(
        D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
        &m_pp, &m_pDevice);

    if (FAILED(hr) || !m_pDevice) {
        LOG("[D3D9Manager] CreateDevice failed hr=0x" << std::hex << hr);
        m_pD3D->Release();
        m_pD3D = nullptr;
        return false;
    }
    return true;
}

bool D3D9Manager::InitializeImGui() noexcept
{
    if (m_bImGuiInit) return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    EZMGR_STYLE_COLORS_APPLY();

    if (!ImGui_ImplWin32_Init(m_hwnd)) { ImGui::DestroyContext(); return false; }
    if (!ImGui_ImplDX9_Init(m_pDevice)) {
        ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false;
    }

    if (m_pMenuInitCallback) m_pMenuInitCallback();
    m_bImGuiInit = true;
    return true;
}

void D3D9Manager::ShutdownImGui() noexcept
{
    if (m_bImGuiInit) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_bImGuiInit = false;
    }
}

bool D3D9Manager::TryResetDevice() noexcept
{
    if (!m_pDevice) return false;
    const HRESULT coop = m_pDevice->TestCooperativeLevel();
    if (coop == D3DERR_DEVICELOST) return false;
    if (coop != D3DERR_DEVICENOTRESET) return true;
    ImGui_ImplDX9_InvalidateDeviceObjects();
    return SUCCEEDED(m_pDevice->Reset(&m_pp)) && ImGui_ImplDX9_CreateDeviceObjects();
}
