#pragma once

#include <array>
#include <atomic>
#include <string_view>

#include <mini/ini.h>

#include "hooks/registry/config_base.hpp"
#include "hooks/registry/dep_list.hpp"
#include "hooks/registry/hook_traits.hpp"
#include "hooks/tags.hpp"

namespace hooks {
    extern std::atomic<bool> g_is_in_game;

    template<>
    struct HookTraits<GameStateHook> {
        static constexpr std::string_view name = "GameState";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &patterns::ResolvedAddresses::game_unpause,
            &patterns::ResolvedAddresses::game_pause,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 1> {
            &patterns::ResolvedAddresses::game_pause2,
        };

        using Config = empty_config;

        static auto install(const patterns::ResolvedAddresses &addrs) -> bool;
    };
} // namespace hooks
