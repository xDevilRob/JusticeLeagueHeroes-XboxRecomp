#pragma once
#include <cstdint>

namespace jlh {
// A flushed packet has been entirely released by the stream, even when only
// part was played. JLH waits for completed == submitted before recycling it.
constexpr uint32_t StreamCompletedBytes(uint32_t size, uint32_t played, uint32_t status) {
    return status == 0 || status == 0x80004004u ? size : played;
}
}
