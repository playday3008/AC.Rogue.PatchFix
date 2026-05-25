#pragma once

#include <cstdint>

#include <optional>

#include "patterns/signatures.hpp"

namespace hooks {
    using PatternField = std::optional<uintptr_t> patterns::ResolvedAddresses::*;

    template<typename Tag>
    struct HookTraits;

    template<typename Tag>
    concept HasOnReload =
        requires(const typename HookTraits<Tag>::Config &cfg) { HookTraits<Tag>::on_reload(cfg); };
} // namespace hooks
