#include <filesystem>
#include <memory>
#include <string>

#include <Windows.h>

#include <mini/ini.h>

#include "logger.hpp" // IWYU pragma: keep

#include "config/file_watcher.hpp"
#include "hooks/registry/registry.hpp"
#include "patterns/signatures.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
static std::unique_ptr<FileWatcher> g_watcher;
#pragma clang diagnostic pop

static auto get_module_directory(HMODULE hModule) -> std::filesystem::path {
    std::string path(MAX_PATH, '\0');
    path.resize(GetModuleFileNameA(hModule, path.data(), static_cast<DWORD>(path.size())));
    auto dir = std::filesystem::path(path).parent_path();
    return dir.empty() ? std::filesystem::path(".") : dir;
}

static void init(HMODULE hModule) {
    auto dir = get_module_directory(hModule);

    log::init((dir / (PROJECT_NAME ".log")).string());

    auto ini_path = dir / (PROJECT_NAME ".ini");

    log::get()->info("Initializing...");

    mINI::INIFile      file(ini_path.string());
    mINI::INIStructure ini;
    if (!file.read(ini)) {
        log::get()->warn("Failed to read INI, using defaults");
    }
    log::get()->trace("INI loaded from {}", ini_path.string());

    patterns::ResolvedAddresses addrs;
    bool                        all = patterns::scan_all(addrs);
    log::get()->info("Pattern scan: {}", all ? "all found" : "some missing");

    hooks::g_registry.install_all(addrs, ini);
    log::get()->trace("Hook registry initialized");

    g_watcher = std::make_unique<FileWatcher>(ini_path, [ini_path] -> void {
        log::get()->info("INI change detected, reloading...");
        mINI::INIFile      f(ini_path.string());
        mINI::INIStructure data;
        if (f.read(data)) {
            hooks::g_registry.reload(data);
        } else {
            log::get()->warn("Config reload failed: could not read INI");
        }
    });
    log::get()->info("File watcher started for {}", ini_path.string());
}

// NOLINTNEXTLINE(misc-use-internal-linkage, modernize-use-trailing-return-type)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*unused*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        init(hModule);
    } else if (reason == DLL_PROCESS_DETACH) {
        log::shutdown();
        g_watcher.reset();
    }
    return TRUE;
}
