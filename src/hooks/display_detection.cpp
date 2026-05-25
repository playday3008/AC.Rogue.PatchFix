#include "hooks/display_detection.hpp"

#include <cstdint>

#include <atomic>
#include <utility>

#include <injector/assembly.hpp>
#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "constants.hpp"
#include "hooks/registry/registry.hpp"

namespace hooks {
    static std::atomic<uintptr_t> g_display_object {0};

    namespace {
        struct DisplayFlagHook {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                g_display_object.store(regs.rbx, std::memory_order_relaxed);

                if (!hooks::enabled<DisplayDetectionHook>()) {
                    if (regs.xmm1.f32[0] < regs.xmm0.f32[0]) {
                        regs.rdx = regs.rsi;
                    }
                    return;
                }

                auto mode = hooks::config<DisplayDetectionHook>().multi_monitor.get();
                switch (mode) {
                    case MultiMonitor::ForceSingle:
                        regs.rdx = 0;
                        break;
                    case MultiMonitor::ForceMulti:
                        regs.rdx = 1;
                        break;
                    case MultiMonitor::Auto: {
                        float w = *reinterpret_cast<float *>(regs.rbx + 0x10);
                        float h = *reinterpret_cast<float *>(regs.rbx + 0x14);
                        regs.rdx =
                            (h > 0.0F && (w / h) >= constants::k_triple_screen_threshold) ? 1 : 0;
                        break;
                    }
                    default:
                        std::unreachable();
                }
            }
        };
    } // namespace

    void HookTraits<DisplayDetectionHook>::on_reload(const Config &cfg) {
        uintptr_t obj = g_display_object.load(std::memory_order_relaxed);
        if (obj == 0) {
            return;
        }

        auto    mode = cfg.multi_monitor.get();
        uint8_t flag = 0;
        switch (mode) {
            case MultiMonitor::ForceSingle:
                flag = 0;
                break;
            case MultiMonitor::ForceMulti:
                flag = 1;
                break;
            case MultiMonitor::Auto: {
                float w = *reinterpret_cast<float *>(obj + 0x10);
                float h = *reinterpret_cast<float *>(obj + 0x14);
                flag    = (h > 0.0F && (w / h) >= constants::k_triple_screen_threshold) ? 1 : 0;
                break;
            }
            default:
                std::unreachable();
        }

        *reinterpret_cast<uint8_t *>(obj + 0x18) = flag;
        if (flag != 0) {
            float w = *reinterpret_cast<float *>(obj + 0x10);
            *reinterpret_cast<uint32_t *>(obj + 0x1C) =
                static_cast<uint32_t>(w * constants::k_multi_monitor_split);
        }

        log::get()->info("DisplayDetection: poked flag={}, singleWidth={}",
                         flag,
                         (flag != 0) ? *reinterpret_cast<uint32_t *>(obj + 0x1C) : 0);
    }

    auto HookTraits<DisplayDetectionHook>::install(const patterns::ResolvedAddresses &addrs)
        -> bool {
        log::get()->trace("DisplayDetectionHook: installing at 0x{:X}", addrs.display_flag.value());
        auto addr = addrs.display_flag.value();
        injector::MakeInline<DisplayFlagHook>(addr, addr + 7);
        log::get()->trace("DisplayDetectionHook: installed");
        return true;
    }
} // namespace hooks
