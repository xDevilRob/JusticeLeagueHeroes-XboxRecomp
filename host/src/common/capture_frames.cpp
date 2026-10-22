#include "gpu/backend.h"
#include <chrono>
#include <cstdlib>
#include <thread>
#include <initializer_list>

// Optional runtime diagnostic, no effect on normal launches.
void JlhCaptureFrames(void*) {
    if (!std::getenv("JLH_CAPTURE_FRAMES")) return;
    std::thread([] {
        for (int delay : {3, 30, 30}) {
            std::this_thread::sleep_for(std::chrono::seconds(delay));
            gpu::g_capture.store(true);
        }
    }).detach();
}

