#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

namespace patterns {
    struct ResolvedAddresses {
        std::optional<uintptr_t> get_game_id;
        std::optional<uintptr_t> lang_bf_write;
        std::optional<uintptr_t> lang_setup;
        std::optional<uintptr_t> get_language;
    };

    struct ScanEntry {
        std::string_view         name;
        std::string_view         bytes;
        ptrdiff_t                offset;
        std::optional<uintptr_t> ResolvedAddresses::*field;
    };

    // Byte signatures TBD — fill from IDA analysis of unpacked ACS.exe
    // clang-format off
    inline constexpr auto scan_entries = std::to_array<ScanEntry>({
        {.name="GET_GAME_ID",   .bytes="", .offset=0x00, .field=&ResolvedAddresses::get_game_id},
        {.name="LANG_BF_WRITE", .bytes="", .offset=0x00, .field=&ResolvedAddresses::lang_bf_write},
        {.name="LANG_SETUP",    .bytes="", .offset=0x00, .field=&ResolvedAddresses::lang_setup},
        {.name="GET_LANGUAGE",  .bytes="", .offset=0x00, .field=&ResolvedAddresses::get_language},
    });
    // clang-format on

    [[nodiscard]] auto scan_all(ResolvedAddresses &out) -> bool;
} // namespace patterns
