#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "config/enums.hpp"
#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/tags.hpp"

namespace hooks {
    [[nodiscard]] auto compute_hor_plus_correction() -> float;

    template<>
    struct HookTraits<FOVCorrectionHook> {
        static constexpr std::string_view name = "FOVCorrection";

        using hard_deps = dep_list<ViewportFittingHook>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &patterns::ResolvedAddresses::fov_store,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<FovMode>    mode {"FOV", "Mode", FovMode::Auto};
            ini_field<float>      multiplier {"FOV", "Multiplier", 1.0F};
            static constexpr auto field_ptrs = std::tuple {&Config::mode, &Config::multiplier};
        };

        static auto install(const patterns::ResolvedAddresses &addrs) -> bool;
    };
} // namespace hooks
