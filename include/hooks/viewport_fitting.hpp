#pragma once

#include <array>
#include <atomic>
#include <string_view>

#include <mini/ini.h>

#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"
#include "hooks/tags.hpp"

namespace hooks {
    extern std::atomic<float> g_current_aspect;

    template<>
    struct HookTraits<ViewportFittingHook> {
        static constexpr std::string_view name = "ViewportFitting";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<GameStateHook>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &patterns::ResolvedAddresses::viewport_ratio_load,
            &patterns::ResolvedAddresses::viewport_ratio_mul,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &patterns::ResolvedAddresses::coord_transform,
        };

        struct Config : config_base<Config> {
            ini_field<float, ratio_parser> aspect_ratio {"Display", "AspectRatio", 0.0F};
            static constexpr auto          field_ptrs = std::tuple {&Config::aspect_ratio};
        };

        static void on_reload(const Config &cfg);
        static auto install(const patterns::ResolvedAddresses &addrs) -> bool;
    };
} // namespace hooks
