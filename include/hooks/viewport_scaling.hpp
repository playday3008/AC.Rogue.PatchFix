#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/registry/parsers.hpp"
#include "hooks/tags.hpp"

namespace hooks {
    template<>
    struct HookTraits<ViewportScalingHook> {
        static constexpr std::string_view name = "ViewportScaling";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &patterns::ResolvedAddresses::scaling_branch_start,
            &patterns::ResolvedAddresses::scaling_branch_end,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &patterns::ResolvedAddresses::scaling_offsets,
        };

        struct Config : config_base<Config> {
            ini_field<float, clamped_unit_parser> ui_stretch_h {"UI", "StretchHorizontal", 0.0F};
            ini_field<float, clamped_unit_parser> ui_stretch_v {"UI", "StretchVertical", 0.0F};
            static constexpr auto                 field_ptrs =
                std::tuple {&Config::ui_stretch_h, &Config::ui_stretch_v};
        };

        static auto install(const patterns::ResolvedAddresses &addrs) -> bool;
    };
} // namespace hooks
