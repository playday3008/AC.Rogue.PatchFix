#pragma once

#include <array>
#include <string_view>

#include <mini/ini.h>

#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/registry/ini_field.hpp"
#include "hooks/tags.hpp"

namespace hooks {
    template<>
    struct HookTraits<FPSUnlockHook> {
        static constexpr std::string_view name = "FPSUnlock";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<GameStateHook>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &patterns::ResolvedAddresses::fps_sleep_branch,
            &patterns::ResolvedAddresses::fps_frame_time,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<float>      target {"FPS", "Target", 0.0F};
            static constexpr auto field_ptrs = std::tuple {&Config::target};
        };

        static void on_reload(const Config &cfg);
        static auto install(const patterns::ResolvedAddresses &addrs) -> bool;
    };
} // namespace hooks
