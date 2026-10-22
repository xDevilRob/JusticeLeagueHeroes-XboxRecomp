#include "xdk/hle.h"
#include "core/log.h"
#include "audio_pitch.h"
#include <atomic>

X86_EXTERN_FUNC(GUEST(IDirectSoundBuffer_SetFrequency));

// This image's buffer pitch wrapper (stdcall, this + pitch) calls 001917B6,
// then 00190CA7, which writes the pitch at the console voice settings +0x18.
// The HLE buffer has no console internals: use the mixer's frequency operation.
// The executable's XBE hash check restricts this address to the analyzed image.
X86_FUNC(sub_0019190E) {
    const int32_t pitch = static_cast<int32_t>(kernel::R32(ctx.esp + 8));
    uint32_t hz = 0;
    if (!jlh::PitchFrequency(pitch, hz)) {
        kernel::Return(ctx, 0x80070057u, 2); // E_INVALIDARG; do not pretend success.
        return;
    }
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) LOGR("JLH: buffer pitch bridge active (%d -> %u Hz)", pitch, hz);
    // Tail-call the existing HLE implementation with the same stdcall layout.
    // It validates the buffer handle and propagates its HRESULT to the caller.
    kernel::W32(ctx.esp + 8, hz);
    GUEST(IDirectSoundBuffer_SetFrequency)(ctx, base);
}

X86_EXTERN_FUNC(GUEST(DirectSoundCreateStream));
// The 001929EB wrapper mirrors CreateSoundBuffer, but constructs the stream
// vtables 001CD700/001CD6F0. Adapt (device, desc, out, outer) to the SDK's
// existing two-argument stream factory rather than executing console constructors.
X86_FUNC(sub_001929EB) {
    const uint32_t esp = ctx.esp;
    const uint32_t desc = kernel::R32(esp + 8);
    const uint32_t out = kernel::R32(esp + 12);
    if (kernel::R32(esp + 16)) {
        kernel::Return(ctx, 0x80070057u, 4);
        return;
    }
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true)) LOGR("JLH: device stream factory bridge active");
    kernel::W32(esp + 4, desc);
    kernel::W32(esp + 8, out);
    GUEST(DirectSoundCreateStream)(ctx, base);
    ctx.esp = esp + 20; // factory consumed 2 args, this interface consumes 4
}
