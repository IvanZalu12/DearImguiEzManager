#pragma once
#include <DearImguiEzManager/gfxManager/gfxManager.hpp>
#include <d3d9.h>

class D3D9Manager final : public GfxManager {
    IDirect3D9*           m_pD3D      = nullptr;
    IDirect3DDevice9*     m_pDevice   = nullptr;
    D3DPRESENT_PARAMETERS m_pp        = {};
    int                   m_backbufW  = 0;
    int                   m_backbufH  = 0;
    bool                  m_bImGuiInit = false;

    bool CreateDeviceInternal(int width, int height) noexcept;
    bool InitializeImGui() noexcept;
    void ShutdownImGui() noexcept;
    bool TryResetDevice() noexcept;

protected:
    bool InitializeGraphics(void* /*unused*/, int width, int height) noexcept override;

public:
    D3D9Manager() noexcept = default;
    ~D3D9Manager() noexcept override;

    void Shutdown() noexcept override;
    void RenderFrame() noexcept override;
    void OnResize(int width, int height) noexcept override;
    void RefreshImGuiFontTexture() noexcept override;
};
