#pragma once

#include <imgui.h>

#include <string>
#include <string_view>

inline ImVec2 EZMGR_CalcTextSize(const char* text,
                                           const char* text_end = nullptr,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
}

inline ImVec2 EZMGR_CalcTextSize(const std::string& text,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size(),
                               hide_text_after_double_hash, wrap_width);
}

inline ImVec2 EZMGR_CalcTextSize(std::string_view text,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return ImGui::CalcTextSize(text.data(), text.data() + text.size(),
                               hide_text_after_double_hash, wrap_width);
}

inline float EZMGR_CalcTextWidth(const char* text,
                                           const char* text_end = nullptr,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width).x;
}

inline float EZMGR_CalcTextWidth(const std::string& text,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, hide_text_after_double_hash, wrap_width).x;
}

inline float EZMGR_CalcTextWidth(std::string_view text,
                                           bool hide_text_after_double_hash = true,
                                           float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, hide_text_after_double_hash, wrap_width).x;
}

inline float EZMGR_CalcTextHeight(const char* text,
                                            const char* text_end = nullptr,
                                            bool hide_text_after_double_hash = true,
                                            float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width).y;
}

inline float EZMGR_CalcTextHeight(const std::string& text,
                                            bool hide_text_after_double_hash = true,
                                            float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, hide_text_after_double_hash, wrap_width).y;
}

inline float EZMGR_CalcTextHeight(std::string_view text,
                                            bool hide_text_after_double_hash = true,
                                            float wrap_width = -1.0f) noexcept
{
    return EZMGR_CalcTextSize(text, hide_text_after_double_hash, wrap_width).y;
}
