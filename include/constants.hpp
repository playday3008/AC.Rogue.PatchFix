#pragma once

namespace constants {
    inline constexpr float k_default_aspect          = 16.0F / 9.0F;
    inline constexpr float k_inv_default_aspect      = 9.0F / 16.0F;
    inline constexpr float k_base_width              = 1280.0F;
    inline constexpr float k_base_height             = 720.0F;
    inline constexpr float k_inv_base_width          = 1.0F / 1280.0F;
    inline constexpr float k_inv_base_height         = 1.0F / 720.0F;
    inline constexpr float k_fov_base_zoom           = 0.768F;
    inline constexpr float k_triple_screen_threshold = 4.0F;
    inline constexpr float k_multi_monitor_split     = 0.33333F;
    inline constexpr float k_float_epsilon           = 1e-6F;
} // namespace constants
