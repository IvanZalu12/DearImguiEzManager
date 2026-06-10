#pragma once

#include <imgui.h>
#include <cmath>

#ifndef SCALE_MANAGER_HPP
#define SCALE_MANAGER_HPP

#ifndef SCALE_STEP 
#define SCALE_STEP 0.05f
#endif


class ScaleManager {
public:
    ScaleManager() noexcept
        : m_scale(1.0f)
    {
    }

    void SetScale(float scale) noexcept
    {
        if (scale < 1.0f)
            scale = 1.0f;
        else if (scale > 4.0f)
            scale = 4.0f;
        scale = std::round(scale / SCALE_STEP) * SCALE_STEP;
        if (scale < 1.0f)
            scale = 1.0f;
        if (scale > 4.0f)
            scale = 4.0f;
        m_scale = scale;
    }

    float GetScale() const noexcept { return m_scale; }

    int Px(int baseValue) const noexcept
    {
        return static_cast<int>(std::lround(static_cast<double>(baseValue) * static_cast<double>(m_scale)));
    }

    int PxFloor(int baseValue) const noexcept
    {
        return static_cast<int>(std::floor(static_cast<double>(baseValue) * static_cast<double>(m_scale)));
    }

    float PxF(float baseValue) const noexcept
    {
        return std::round(baseValue * m_scale);
    }

    int Rounding(int baseRounding) const noexcept
    {
        const double v = static_cast<double>(baseRounding) * std::pow(static_cast<double>(m_scale), 0.7);
        return static_cast<int>(std::lround(v));
    }

    ImVec2 Pad(int baseX, int baseY) const noexcept
    {
        return ImVec2(static_cast<float>(Px(baseX)), static_cast<float>(Px(baseY)));
    }

private:
    float m_scale;
};

#endif // SCALE_MANAGER_HPP