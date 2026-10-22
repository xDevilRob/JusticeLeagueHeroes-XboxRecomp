#include "xdk/hle.h"
#include "core/log.h"
#include <cstdlib>

X86_EXTERN_FUNC(__imp__sub_0006D77C);
X86_EXTERN_FUNC(__imp__sub_0006B718);
X86_EXTERN_FUNC(__imp__sub_0006BA4B);
X86_EXTERN_FUNC(__imp__sub_0006CCE4);

namespace {
bool Enabled() {
    static const bool enabled = std::getenv("JLH_TRACE_AUDIO") != nullptr;
    return enabled;
}
}

// Observe the original game's requests, without substituting tracks or changing state.
X86_FUNC(sub_0006D77C) {
    if (Enabled()) LOGR("JLH music request channel %u name '%s' flags %08X caller %08X",
        kernel::R32(ctx.esp+4), kernel::ReadCString(kernel::R32(ctx.esp+8), 160).c_str(),
        kernel::R32(ctx.esp+12), kernel::R32(ctx.esp));
    __imp__sub_0006D77C(ctx, base);
}

X86_FUNC(sub_0006B718) {
    const bool trace = Enabled();
    const std::string name = trace ? kernel::ReadCString(kernel::R32(ctx.esp+4), 160) : std::string{};
    __imp__sub_0006B718(ctx, base);
    if (trace) LOGR("JLH music lookup '%s' result %08X data %08X", name.c_str(), ctx.eax,
        ctx.eax ? kernel::R32(ctx.eax+0x3C) : 0);
}

X86_FUNC(sub_0006BA4B) {
    if (Enabled()) LOGR("JLH music stop channel %u caller %08X", kernel::R32(ctx.esp+4), kernel::R32(ctx.esp));
    __imp__sub_0006BA4B(ctx, base);
}

X86_FUNC(sub_0006CCE4) {
    const uint32_t channel = kernel::R32(ctx.esp+4);
    const bool trace = Enabled() && channel < 3;
    const uint32_t queued = trace ? kernel::R32(0x42B244+channel*0x348) : 0;
    if (trace) LOGR("JLH music enqueue channel %u queued %u source %08X arg8 %08X", channel, queued,
        kernel::R32(ctx.esp+8), kernel::R32(ctx.esp+32));
    __imp__sub_0006CCE4(ctx, base);
    if (trace) LOGR("JLH music enqueue result channel %u queued %u", channel,
        kernel::R32(0x42B244+channel*0x348));
}
