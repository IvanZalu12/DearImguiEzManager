#pragma once
#ifndef WIN32
#include <DearImguiEzManager/gfxManager/gfxManager.hpp>

class AndroidOpenGLManager final : public GfxManager {
    int    m_width    = 0;
    int    m_height   = 0;
    double m_lastTime = 0.0;
    bool   m_ready    = false;

protected:
    bool InitializeGraphics(void* nativeHandle, int w, int h) noexcept override;

public:
    void Shutdown() noexcept override;
    void RenderFrame() noexcept override;
    void OnResize(int w, int h) noexcept override;
    void RefreshImGuiFontTexture() noexcept override;
};
#endif
