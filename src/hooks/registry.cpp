#include "hooks/registry/registry.hpp"

#include <cstddef>

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <mini/ini.h>

#include "logger.hpp" // IWYU pragma: keep

#include "hooks/registry/parsers.hpp"
#include "hooks/registry/validate.hpp"
#include "patterns/signatures.hpp"

namespace hooks {
    Registry g_registry;

    namespace {
        // --- Compile-time: validate all hook deps ---

        template<typename... Tags>
        constexpr auto validate_all(hook_list<Tags...> /*unused*/) -> bool {
            return (sizeof(validate_hook_deps<Tags, AllHooks>) && ...);
        }

        static_assert(validate_all(AllHooks {}));

        // --- Hook index computation ---

        template<typename Tag, typename... Tags>
        constexpr auto hook_idx_in(hook_list<Tags...> /*unused*/) -> std::size_t {
            constexpr std::array matches = {std::is_same_v<Tag, Tags>...};
            for (std::size_t i = 0; i < sizeof...(Tags); ++i) {
                if (matches.at(i)) {
                    return i;
                }
            }
            return sizeof...(Tags);
        }

        template<typename Tag>
        constexpr std::size_t hook_idx = hook_idx_in<Tag>(AllHooks {});

        // --- Dep index arrays (static storage for runtime use) ---

        template<typename Tag>
        struct DepIndices {
            static constexpr auto compute_hard() {
                return []<typename... Deps>(dep_list<Deps...>) -> auto {
                    return std::array<std::size_t, sizeof...(Deps)> {hook_idx<Deps>...};
                }(typename HookTraits<Tag>::hard_deps {});
            }
            static constexpr auto compute_soft() {
                return []<typename... Deps>(dep_list<Deps...>) -> auto {
                    return std::array<std::size_t, sizeof...(Deps)> {hook_idx<Deps>...};
                }(typename HookTraits<Tag>::soft_deps {});
            }
            static constexpr auto hard = compute_hard();
            static constexpr auto soft = compute_soft();
        };

        // --- Hook count ---

        template<typename... Tags>
        constexpr auto hook_list_size(hook_list<Tags...> /*unused*/) -> std::size_t {
            return sizeof...(Tags);
        }

        constexpr std::size_t N = hook_list_size(AllHooks {});

        // --- Flat dep arrays for constexpr topo sort ---

        template<typename... Tags>
        constexpr auto build_hard_deps(hook_list<Tags...> /*unused*/) {
            return std::array<std::span<const std::size_t>, sizeof...(Tags)> {
                DepIndices<Tags>::hard...};
        }

        template<typename... Tags>
        constexpr auto build_soft_deps(hook_list<Tags...> /*unused*/) {
            return std::array<std::span<const std::size_t>, sizeof...(Tags)> {
                DepIndices<Tags>::soft...};
        }

        // --- Constexpr topological sort (Kahn's algorithm) ---

        constexpr auto topo_sort() -> std::pair<std::array<std::size_t, N>, std::size_t> {
            auto hard = build_hard_deps(AllHooks {});
            auto soft = build_soft_deps(AllHooks {});

            std::array<std::size_t, N>                in_degree {};
            std::array<std::array<std::size_t, N>, N> adj {};
            std::array<std::size_t, N>                adj_count {};

            for (std::size_t i = 0; i < N; ++i) {
                for (auto dep : hard.at(i)) {
                    adj.at(dep).at(adj_count.at(dep)++) = i;
                    in_degree.at(i)++;
                }
                for (auto dep : soft.at(i)) {
                    adj.at(dep).at(adj_count.at(dep)++) = i;
                    in_degree.at(i)++;
                }
            }

            std::array<std::size_t, N> queue {};
            std::size_t                front = 0;
            std::size_t                back  = 0;

            for (std::size_t i = 0; i < N; ++i) {
                if (in_degree.at(i) == 0) {
                    queue.at(back++) = i;
                }
            }

            std::array<std::size_t, N> order {};
            std::size_t                count = 0;

            while (front < back) {
                auto u            = queue.at(front++);
                order.at(count++) = u;
                for (std::size_t i = 0; i < adj_count.at(u); ++i) {
                    auto v = adj.at(u).at(i);
                    if (--in_degree.at(v) == 0) {
                        queue.at(back++) = v;
                    }
                }
            }

            return {order, count};
        }

        constexpr auto sorted_result = topo_sort();
        static_assert(sorted_result.second == N, "Dependency cycle detected in hook graph");
        constexpr auto install_order = sorted_result.first;

        // --- Type-erased operations per hook ---

        struct HookOps {
            std::string_view             name;
            std::size_t                  index;
            std::span<const std::size_t> hard_deps;
            std::span<const std::size_t> soft_deps;

            void (*load_config)(Registry &r, mINI::INIStructure &ini);
            void (*load_enabled)(Registry &r, mINI::INIStructure &ini);
            bool (*check_required)(const patterns::ResolvedAddresses &addrs);
            bool (*do_install)(const patterns::ResolvedAddresses &addrs);
            void (*set_installed)(Registry &r, bool val);
            void (*set_enabled)(Registry &r, bool val);
            bool (*is_enabled)(const Registry &r);
            bool (*is_installed)(const Registry &r);
            void (*call_on_reload)(Registry &r);
        };

        template<typename Tag>
        auto make_ops() -> HookOps {
            return HookOps {
                .name      = HookTraits<Tag>::name,
                .index     = hook_idx<Tag>,
                .hard_deps = DepIndices<Tag>::hard,
                .soft_deps = DepIndices<Tag>::soft,

                .load_config = [](Registry &r, mINI::INIStructure &ini) -> auto {
                    r.config<Tag>().load_all(ini);
                },
                .load_enabled = [](Registry &r, mINI::INIStructure &ini) -> auto {
                    if (!ini.has("Hooks")) {
                        return;
                    }
                    auto       &sec = ini["Hooks"];
                    std::string key(HookTraits<Tag>::name);
                    if (sec.has(key)) {
                        r.set_enabled<Tag>(default_parser<bool> {}(sec[key]));
                    }
                },
                .check_required = [](const patterns::ResolvedAddresses &addrs) -> bool {
                    return std::ranges::all_of(HookTraits<Tag>::required_patterns,
                                               [&](PatternField f) -> auto {
                                                   return (addrs.*f).has_value();
                                               });
                },
                .do_install = [](const patterns::ResolvedAddresses &addrs) -> bool {
                    return HookTraits<Tag>::install(addrs);
                },
                .set_installed  = [](Registry &r, bool val) -> auto { r.set_installed<Tag>(val); },
                .set_enabled    = [](Registry &r, bool val) -> auto { r.set_enabled<Tag>(val); },
                .is_enabled     = [](const Registry &r) -> bool { return r.enabled<Tag>(); },
                .is_installed   = [](const Registry &r) -> bool { return r.installed<Tag>(); },
                .call_on_reload = [](Registry &r) -> auto {
                    if constexpr (HasOnReload<Tag>) {
                        HookTraits<Tag>::on_reload(r.config<Tag>());
                    }
                },
            };
        }

        template<typename... Tags>
        auto make_all_ops(hook_list<Tags...> /*unused*/) -> std::array<HookOps, sizeof...(Tags)> {
            return {make_ops<Tags>()...};
        }

        auto get_ops() -> const std::array<HookOps, N> & {
            static const auto ops = make_all_ops(AllHooks {});
            return ops;
        }

        // --- Cascade disable ---

        void cascade_disable(std::array<bool, N> &enabled) {
            bool changed = true;
            while (changed) {
                changed = false;
                for (std::size_t i = 0; i < N; ++i) {
                    if (!enabled.at(i)) {
                        continue;
                    }
                    for (auto dep : get_ops().at(i).hard_deps) {
                        if (!enabled.at(dep)) {
                            log::get()->info(
                                "Hook '{}': disabled (hard dependency '{}' is disabled)",
                                get_ops().at(i).name,
                                get_ops().at(dep).name);
                            enabled.at(i) = false;
                            changed       = true;
                            break;
                        }
                    }
                }
            }
        }
    } // anonymous namespace

    // --- Registry public methods ---

    void Registry::install_all(const patterns::ResolvedAddresses &addrs, mINI::INIStructure &ini) {
        log::get()->trace("install_all: loading configs");
        for (const auto &op : get_ops()) {
            op.load_config(*this, ini);
        }

        log::get()->trace("install_all: loading enabled flags");
        for (const auto &op : get_ops()) {
            op.load_enabled(*this, ini);
        }

        std::array<bool, N> enabled_flags {};
        for (std::size_t i = 0; i < N; ++i) {
            enabled_flags.at(i) = get_ops().at(i).is_enabled(*this);
        }
        cascade_disable(enabled_flags);
        for (std::size_t i = 0; i < N; ++i) {
            get_ops().at(i).set_enabled(*this, enabled_flags.at(i));
        }

        log::get()->trace("install_all: installing in dependency order");

        int installed_count = 0;
        for (auto idx : install_order) {
            const auto &op = get_ops().at(idx);

            if (!op.is_enabled(*this)) {
                log::get()->info("Hook '{}': disabled", op.name);
                continue;
            }

            if (!op.check_required(addrs)) {
                log::get()->warn("Hook '{}': missing required patterns, skipping", op.name);
                continue;
            }

            if (op.do_install(addrs)) {
                op.set_installed(*this, true);
                log::get()->info("Hook '{}': installed", op.name);
                ++installed_count;
            } else {
                log::get()->warn("Hook '{}': install failed", op.name);
            }
        }

        log::get()->info("Initialization complete: {}/{} hooks installed", installed_count, N);
    }

    void Registry::reload(mINI::INIStructure &ini) {
        log::get()->info("Config reload...");

        for (const auto &op : get_ops()) {
            op.load_config(*this, ini);
        }
        log::get()->trace("reload: configs loaded");

        for (const auto &op : get_ops()) {
            op.load_enabled(*this, ini);
        }

        std::array<bool, N> enabled_flags {};
        for (std::size_t i = 0; i < N; ++i) {
            enabled_flags.at(i) = get_ops().at(i).is_enabled(*this);
        }
        cascade_disable(enabled_flags);
        for (std::size_t i = 0; i < N; ++i) {
            get_ops().at(i).set_enabled(*this, enabled_flags.at(i));
        }

        for (const auto &op : get_ops()) {
            if (op.is_installed(*this) && op.is_enabled(*this)) {
                log::get()->trace("reload: calling on_reload for '{}'", op.name);
                op.call_on_reload(*this);
            }
        }

        log::get()->info("Config reloaded");
    }
} // namespace hooks
