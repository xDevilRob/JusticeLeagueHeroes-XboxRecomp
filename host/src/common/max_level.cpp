#include "xdk/hle.h"
#include "core/log.h"
#include <cstdlib>

X86_EXTERN_FUNC(__imp__sub_0006DBB8);

// Opt-in test mode. Execute the game's setters on its own thread after its
// frame update, only once gameplay has a player and the profile is loaded.
X86_FUNC(sub_0006DBB8) {
    __imp__sub_0006DBB8(ctx, base);
    static const bool enabled = std::getenv("JLH_MAX_LEVEL") != nullptr;
    if (!enabled || !kernel::R32(0x44A88C)) return;
    bool changed = false;
    for (uint32_t hero = 0; hero < 15; ++hero) {
        const uint32_t record = 0x1DF638 + hero * 0x78;
        const uint32_t level = kernel::G<uint8_t>(record+3);
        if (level >= 1 && level < 20) {
            kernel::CallGuest(ctx, 0x71310, {hero, 20});
            LOGR("JLH max level: hero %u level %u -> %u", hero, level,
                kernel::G<uint8_t>(record+3));
            changed = true;
        }
    }
    if (changed) {
        // The native 'maximum abilities' cheat: nine ability ranks, all heroes.
        const uint32_t saved_ecx = ctx.ecx;
        ctx.ecx = 0x2ADF14; // thiscall: this image's native cheat manager
        kernel::CallGuest(ctx, 0x1C626, {5});
        ctx.ecx = saved_ecx;
        LOGR("JLH max level: native maximum-abilities cheat applied");
    }
}
