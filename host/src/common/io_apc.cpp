#include "kernel/exports.h"
#include <atomic>

// Xbox ordinal 232 dispatches an overlapped I/O completion as
// callback(error, transferred, overlapped). Interface reference:
// Cxbx-Reloaded EmuKrnlNt.cpp, NtUserIoApcDispatcher (not copied).
namespace kernel {
KERNEL_EXPORT(232, NtUserIoApcDispatcher, 3) {
    const uint32_t callback = a[0], iosb = a[1];
    if (!callback || !iosb) x86_trap(ctx, R32(ctx.esp), "invalid I/O APC callback or status block");
    const uint32_t status = R32(iosb);
    const bool success = static_cast<int32_t>(status) >= 0;
    // Reuse the runtime's ordinal 301 status mapping for a failed completion.
    const uint32_t error = success ? 0 : CallGuest(ctx, 0xFE000000u + 301 * 4, {status});
    const uint32_t transferred = success ? R32(iosb + 4) : 0;
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) LOGR("JLH: I/O APC completion active (status %08X, bytes %u)", status, transferred);
    CallGuest(ctx, callback, {error, transferred, iosb});
    return 0; // void dispatcher; callback result has no NTSTATUS meaning
}
}
