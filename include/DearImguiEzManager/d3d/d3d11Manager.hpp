#pragma once
#include <DearImguiEzManager/gfxManager/gfxManager.hpp>
#include <d3d11.h>
#include <dxgi.h>

class D3D11Manager final : public GfxManager {
    struct D3D11Data {
        IDXGISwapChain*         pSwapChain = nullptr;
        ID3D11Device*           pDevice    = nullptr;
        ID3D11DeviceContext*    pContext   = nullptr;
        ID3D11RenderTargetView* pMainRTV   = nullptr;
        bool bImGuiInitialized             = false;
    };

    D3D11Data* m_pData;

    bool CreateDeviceStandalone(int width, int height) noexcept;
    void CleanupRenderTarget() noexcept;
    bool CreateRenderTarget() noexcept;
    bool InitializeImGui() noexcept;
    void ShutdownImGui() noexcept;

protected:
    bool InitializeGraphics(void* /*unused*/, int width, int height) noexcept override;

public:
    D3D11Manager() noexcept;
    ~D3D11Manager() noexcept override;

    void Shutdown() noexcept override;
    void RenderFrame() noexcept override;
    void OnResize(int width, int height) noexcept override;
    void RefreshImGuiFontTexture() noexcept override;
};
