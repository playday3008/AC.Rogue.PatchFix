#include "hooks/fov_correction.hpp"

#include <cmath>

#include <atomic>
#include <utility>

#include <injector/assembly.hpp>
#include <injector/injector.hpp>

#include "logger.hpp" // IWYU pragma: keep

#include "constants.hpp"
#include "hooks/registry/registry.hpp"

namespace hooks {
    std::atomic<float> g_current_aspect {constants::k_default_aspect};

    auto compute_hor_plus_correction() -> float {
        float aspect = g_current_aspect.load(std::memory_order_relaxed);
        if (aspect <= 0.0F ||
            std::abs(aspect - constants::k_default_aspect) < constants::k_float_epsilon) {
            return 1.0F;
        }

        float ratio     = constants::k_default_aspect / aspect;
        float corrected = 2.0F * std::atan(constants::k_fov_base_zoom * ratio);
        float original  = 2.0F * std::atan(constants::k_fov_base_zoom);
        if (std::abs(original) < constants::k_float_epsilon) {
            return 1.0F;
        }
        return corrected / original;
    }

    namespace {
        struct FOVCorrectionFunctor {
            [[maybe_unused]] static void operator()(injector::reg_pack &regs) {
                if (!hooks::enabled<FOVCorrectionHook>()) {
                    *reinterpret_cast<float *>(regs.rbx + 0x40) = regs.xmm6.f32[0];
                    return;
                }

                float fov = regs.xmm6.f32[0];

                const auto &cfg            = hooks::config<FOVCorrectionHook>();
                auto        mode           = cfg.mode.get();
                bool        apply_hor_plus = false;
                switch (mode) {
                    case FovMode::Auto:
                        apply_hor_plus = (g_current_aspect.load(std::memory_order_relaxed) <
                                          constants::k_default_aspect);
                        break;
                    case FovMode::VertPlus:
                        break;
                    case FovMode::HorPlus:
                        apply_hor_plus = true;
                        break;
                    default:
                        std::unreachable();
                }
                if (apply_hor_plus) {
                    fov *= compute_hor_plus_correction();
                }

                fov *= cfg.multiplier.get();

                *reinterpret_cast<float *>(regs.rbx + 0x40) = fov;
            }
        };
    } // namespace

    auto HookTraits<FOVCorrectionHook>::install(const patterns::ResolvedAddresses &addrs) -> bool {
        log::get()->trace("FOVCorrectionHook: installing at 0x{:X}", addrs.fov_store.value());
        auto addr = addrs.fov_store.value();
        injector::MakeInline<FOVCorrectionFunctor>(addr, addr + 5);
        log::get()->trace("FOVCorrectionHook: installed");
        return true;
    }
} // namespace hooks
