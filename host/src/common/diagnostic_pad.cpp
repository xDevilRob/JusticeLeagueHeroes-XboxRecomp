#include "app/app.h"
#include "controls.h"
#include "core/input.h"
#include "core/log.h"
#include "gpu/backend.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

// Opt-in diagnostic controller. A small command file gives one 150 ms press.
// Only the separate test run sets JLH_PAD_COMMAND_FILE; normal input is untouched.
void JlhDiagnosticPad(int port, input::Pad& pad) {
    const char* path = std::getenv("JLH_PAD_COMMAND_FILE");
    if (!path || port != 0) return;
    static std::mutex lock;
    std::lock_guard guard(lock);
    static std::string command;
    static bool cheat_chord = false;
    static bool live = false;
    static auto until = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    std::ifstream in(path);
    std::string next;
    if (in >> next) {
        in.close();
        std::ofstream clear(path, std::ios::trunc);
        if (next == "CAPTURE") gpu::g_capture.store(true);
        if (next == "INPUT_LIVE") live=true;
        if (next == "INPUT_DIAGNOSTIC") live=false;
        if(std::getenv("JLH_TEST_INPUT")) {
            if(next=="TEST_LAYOUT_XBOX")g_jlh_layout_preview.store(3);
            if(next=="TEST_LAYOUT_PS")g_jlh_layout_preview.store(4);
            if(next=="TEST_LAYOUT_KEYBOARD")g_jlh_layout_preview.store(5);
            if(next=="TEST_LAYOUT_AUTO")g_jlh_layout_preview.store(0);
        }
        if (next == "CHEAT_HOLD") cheat_chord = true;
        if (next == "CHEAT_RELEASE") cheat_chord = false;
        command = next;
        until = now + std::chrono::milliseconds(150);
        LOGR("JLH diagnostic pad: %s", next.c_str());
    }
    if(live)return;
    pad = input::Pad{};
    pad.connected = true;
    if (cheat_chord) pad.analog[6] = pad.analog[7] = 255;
    if (now >= until) return;
    if (command == "A") pad.analog[0] = 255;
    else if (command == "B") pad.analog[1] = 255;
    else if (command == "X") pad.analog[2] = 255;
    else if (command == "Y") pad.analog[3] = 255;
    else if (command == "MOVE_UP") pad.ly = 32767;
    else if (command == "MOVE_DOWN") pad.ly = -32767;
    else if (command == "MOVE_LEFT") pad.lx = -32767;
    else if (command == "MOVE_RIGHT") pad.lx = 32767;
    else if (command == "START") pad.buttons = 0x10;
    else if (command == "BACK") pad.buttons = 0x20;
    else if (command == "UP") pad.buttons = 1;
    else if (command == "DOWN") pad.buttons = 2;
    else if (command == "LEFT") pad.buttons = 4;
    else if (command == "RIGHT") pad.buttons = 8;
}

