#include <DearImguiEzManager/d3d/d3d11Manager.hpp>
#include <iostream>
#include <DearImguiEzManager/MenuManager/ImGuiStyleColors.hpp>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#define LOG(x) std::cout << x << std::endl

D3D11Manager::D3D11Manager() noexcept
    : m_pData(new D3D11Data())
{
}

D3D11Manager::~D3D11Manager() noexcept
{
    Shutdown();
    delete m_pData;
    m_pData = nullptr;
}

bool D3D11Manager::InitializeGraphics(void* /*unused*/, int width, int height) noexcept
{
    if (!m_pData || !m_hwnd)
        return false;

    LOG("[D3D11Manager] Initializing...");

    if (!CreateDeviceStandalone(width, height)) {
        LOG("[D3D11Manager] Device creation failed");
        return false;
    }
    if (!InitializeImGui()) {
        LOG("[D3D11Manager] ImGui init failed");
        Shutdown();
        return false;
    }

    LOG("[D3D11Manager] Ready");
    return true;
}

void D3D11Manager::Shutdown() noexcept
{
    if (!m_pData) return;
    ShutdownImGui();
    CleanupRenderTarget();
    if (m_pData->pContext)   { m_pData->pContext->Release();   m_pData->pContext   = nullptr; }
    if (m_pData->pDevice)    { m_pData->pDevice->Release();    m_pData->pDevice    = nullptr; }
    if (m_pData->pSwapChain) { m_pData->pSwapChain->Release(); m_pData->pSwapChain = nullptr; }
}

void D3D11Manager::RenderFrame() noexcept
{
    if (!m_pData || !m_pData->bImGuiInitialized)
        return;
    if (m_hwnd && IsIconic(m_hwnd))   // minimized: client is 0x0, skip the whole frame
        return;
    if (m_rendering)   // re-entrancy guard (e.g. a synchronous WM_SIZE mid-frame)
        return;
    m_rendering = true;

    if (m_pPreFrameCallback)
        m_pPreFrameCallback();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_pRenderCallback)
        m_pRenderCallback();

    ImGui::Render();

    const float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    m_pData->pContext->ClearRenderTargetView(m_pData->pMainRTV, clearColor);
    m_pData->pContext->OMSetRenderTargets(1, &m_pData->pMainRTV, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_pData->pSwapChain->Present(1, 0);

    m_rendering = false;
}

void D3D11Manager::OnResize(int width, int height) noexcept
{
    if (!m_pData || !m_pData->pSwapChain || width == 0 || height == 0)
        return;
    CleanupRenderTarget();
    m_pData->pSwapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTarget();
}

void D3D11Manager::RefreshImGuiFontTexture() noexcept
{
    if (!m_pData || !m_pData->pDevice || !ImGui::GetCurrentContext())
        return;
    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();
}

bool D3D11Manager::CreateDeviceStandalone(int width, int height) noexcept
{
    HMODULE hMod = LoadLibraryW(L"d3d11.dll");
    if (!hMod) {
        LOG("[D3D11Manager] d3d11.dll not found");
        return false;
    }

    using PFN = HRESULT(WINAPI*)(
        IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
        const D3D_FEATURE_LEVEL*, UINT, UINT,
        const DXGI_SWAP_CHAIN_DESC*,
        IDXGISwapChain**, ID3D11Device**,
        D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

    auto pfn = reinterpret_cast<PFN>(GetProcAddress(hMod, "D3D11CreateDeviceAndSwapChain"));

    HRESULT hr = E_FAIL;
    if (pfn) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount                        = 2;
        sd.BufferDesc.Width                   = (UINT)width;
        sd.BufferDesc.Height                  = (UINT)height;
        sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator   = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow                       = m_hwnd;
        sd.SampleDesc.Count                   = 1;
        sd.SampleDesc.Quality                 = 0;
        sd.Windowed                           = TRUE;
        sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL fl;
        hr = pfn(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                 nullptr, 0, D3D11_SDK_VERSION, &sd,
                 &m_pData->pSwapChain, &m_pData->pDevice, &fl, &m_pData->pContext);
    }

    FreeLibrary(hMod);

    if (FAILED(hr) || !m_pData->pSwapChain) {
        LOG("[D3D11Manager] D3D11CreateDeviceAndSwapChain failed hr=0x" << std::hex << hr);
        return false;
    }
    return CreateRenderTarget();
}

void D3D11Manager::CleanupRenderTarget() noexcept
{
    if (m_pData && m_pData->pMainRTV) {
        m_pData->pMainRTV->Release();
        m_pData->pMainRTV = nullptr;
    }
}

bool D3D11Manager::CreateRenderTarget() noexcept
{
    if (!m_pData || !m_pData->pSwapChain || !m_pData->pDevice)
        return false;

    ID3D11Texture2D* pBack = nullptr;
    if (FAILED(m_pData->pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBack)) || !pBack)
        return false;

    HRESULT hr = m_pData->pDevice->CreateRenderTargetView(pBack, nullptr, &m_pData->pMainRTV);
    pBack->Release();
    return SUCCEEDED(hr) && m_pData->pMainRTV;
}

bool D3D11Manager::InitializeImGui() noexcept
{
    if (!m_pData || m_pData->bImGuiInitialized)
        return false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    EZMGR_STYLE_COLORS_APPLY();

    if (!ImGui_ImplWin32_Init(m_hwnd)) { ImGui::DestroyContext(); return false; }
    if (!ImGui_ImplDX11_Init(m_pData->pDevice, m_pData->pContext)) {
        ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext(); return false;
    }

    if (m_pMenuInitCallback) m_pMenuInitCallback();
    m_pData->bImGuiInitialized = true;
    return true;
}

void D3D11Manager::ShutdownImGui() noexcept
{
    if (m_pData && m_pData->bImGuiInitialized) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_pData->bImGuiInitialized = false;
    }
}
