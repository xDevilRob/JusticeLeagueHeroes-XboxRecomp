#pragma once
#include <atomic>
#include <cstdint>
#include "core/input.h"
extern uint32_t g_jlh_button_layout;
extern std::atomic<uint32_t> g_jlh_layout_preview;
uint32_t JlhActiveLayout();
void JlhControlsLoaded();
void JlhControlsWindow(void*);
bool JlhControlsMessage(void*,uint32_t,uintptr_t,intptr_t,intptr_t*);
void JlhControlsPad(int,input::Pad&);
