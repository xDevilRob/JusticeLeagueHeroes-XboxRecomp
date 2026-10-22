#pragma once
#include <cmath>
#include <cstdint>

namespace jlh {
// Xbox APU pitch is signed, in 1/4096-octave units relative to 48 kHz.
// Hardware research: https://xboxdevwiki.net/APU#Pitch_calculation
inline bool PitchFrequency(int32_t pitch, uint32_t& hz) {
    if (pitch < -32768 || pitch > 32767) return false;
    hz = static_cast<uint32_t>(std::lround(48000.0 * std::exp2(double(pitch) / 4096.0)));
    return true;
}
}
