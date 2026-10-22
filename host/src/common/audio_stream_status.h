#pragma once
#include <cstddef>
#include <cstdint>

namespace jlh {
// XDK 5849 DSSSTATUS: READY, PLAYING and PAUSED are independent flags.
constexpr uint32_t StreamStatus(size_t queued, size_t capacity, bool paused, bool sync) {
    uint32_t status = queued < capacity ? 1u : 0u;
    if (queued) {
        if (paused) status |= 0x20000u;
        else if (!sync) status |= 0x10000u;
    }
    return status;
}
}
