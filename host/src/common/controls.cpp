#include "controls.h"
#include "app/app.h"
#include "core/log.h"
#include "xdk/hle.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <cstdlib>

uint32_t g_jlh_button_layout=0;
std::atomic<uint32_t> g_jlh_layout_preview{0};
namespace {
std::atomic<uint32_t> detected{3};
std::atomic<bool> loaded{false};
std::mutex state_lock;
std::array<bool,256> keys{};
int mouse_x=0,wheel_units=0,zoom_direction=0;
int16_t camera_x=0;
uint64_t camera_tick=0;
uint64_t zoom_until=0;
bool escape_menu=false, cursor_clipped=false;
constexpr UINT_PTR cursor_timer=0x4a4c48;
bool Focused(HWND hwnd) {
    // The isolated automated run sends window messages without taking the user's foreground.
    static const bool test=std::getenv("JLH_TEST_INPUT") && std::getenv("JLH_PAD_COMMAND_FILE");
    return GetForegroundWindow()==hwnd || test;
}
bool Menu() {
    // The native UI manager clears its current movie when it returns to play.
    return !loaded.load() || !kernel::R32(0x44A88C) || kernel::R32(0x46A394)!=0;
}
void Cursor(HWND hwnd) {
    const bool confine=GetForegroundWindow()==hwnd && detected.load()==5 && !Menu();
    if(confine) {
        RECT r;GetClientRect(hwnd,&r);MapWindowPoints(hwnd,nullptr,reinterpret_cast<POINT*>(&r),2);
        ClipCursor(&r);
        if(!cursor_clipped) {ShowCursor(FALSE);cursor_clipped=true;}
    } else if(cursor_clipped) {ClipCursor(nullptr);ShowCursor(TRUE);cursor_clipped=false;}
}
void Clear() {keys.fill(false);mouse_x=wheel_units=zoom_direction=0;camera_x=0;zoom_until=0;}
}
uint32_t JlhActiveLayout() {
    const auto choice=g_jlh_layout_preview.load();
    return choice==2?3:choice>=1 && choice<=5?choice:detected.load();
}
void JlhControlsLoaded() {
    if(g_jlh_button_layout>5)g_jlh_button_layout=0;
    if(g_jlh_button_layout==2)g_jlh_button_layout=3;
    g_jlh_layout_preview.store(g_jlh_button_layout);
    loaded.store(true);
    LOGR("Controls: layout %u; keyboard/mouse enabled on player 1",g_jlh_button_layout);
}
void JlhCaptureFrames(void*);
void JlhControlsWindow(void* window) {
    auto hwnd=static_cast<HWND>(window);
    RAWINPUTDEVICE mouse{0x01,0x02,0,hwnd};
    if(!RegisterRawInputDevices(&mouse,1,sizeof(mouse)))LOGE("Controls: raw mouse registration failed %lu",GetLastError());
    SetTimer(hwnd,cursor_timer,16,nullptr);
    JlhCaptureFrames(window);
}
bool JlhControlsMessage(void* window,uint32_t msg,uintptr_t wp,intptr_t lp,intptr_t* result) {
    auto hwnd=static_cast<HWND>(window);
    if(msg==WM_TIMER && wp==cursor_timer) {Cursor(hwnd);*result=0;return true;}
    if(msg==WM_KILLFOCUS || msg==WM_DESTROY || (msg==WM_ACTIVATEAPP && !wp)) {
        std::lock_guard guard(state_lock);Clear();
        if(cursor_clipped){ClipCursor(nullptr);ShowCursor(TRUE);cursor_clipped=false;}
    }
    if(!Focused(hwnd))return false;
    std::lock_guard guard(state_lock);
    if(msg==WM_MOUSEWHEEL) {
        if(!Menu())wheel_units=std::clamp(wheel_units+int(int16_t((wp>>16)&0xffff)),-1200,1200);
        detected.store(5);
    }
    if(msg==WM_KEYDOWN || msg==WM_KEYUP || msg==WM_SYSKEYDOWN || msg==WM_SYSKEYUP) {
        unsigned key=unsigned(wp);
        if(key==VK_SHIFT)key=MapVirtualKeyW((unsigned(lp)>>16)&0xff,MAPVK_VSC_TO_VK_EX);
        const bool down=msg==WM_KEYDOWN || msg==WM_SYSKEYDOWN;
        if(key<keys.size()) {
            if(key==VK_ESCAPE && down && !keys[key])escape_menu=Menu();
            keys[key]=down;
            if(down && (key=='W'||key=='A'||key=='S'||key=='D'||key=='F'||key=='E'||key=='Q'||key=='Z'||key==VK_SPACE||key==VK_LSHIFT||key==VK_RETURN||key==VK_ESCAPE||key==VK_TAB||(key>=VK_LEFT&&key<=VK_DOWN)))detected.store(5);
        }
    }
    if(msg==WM_LBUTTONDOWN || msg==WM_LBUTTONUP || msg==WM_RBUTTONDOWN || msg==WM_RBUTTONUP || msg==WM_MBUTTONDOWN || msg==WM_MBUTTONUP) {
        const bool left=msg==WM_LBUTTONDOWN || msg==WM_LBUTTONUP;
        const bool middle=msg==WM_MBUTTONDOWN || msg==WM_MBUTTONUP;
        const bool down=msg==WM_LBUTTONDOWN || msg==WM_RBUTTONDOWN || msg==WM_MBUTTONDOWN;
        keys[middle?VK_MBUTTON:left?VK_LBUTTON:VK_RBUTTON]=down;
        if(down)detected.store(5);
    }
    if(msg==WM_INPUT) {
        RAWINPUT raw{};UINT size=sizeof(raw);
        if(GetRawInputData(reinterpret_cast<HRAWINPUT>(lp),RID_INPUT,&raw,&size,sizeof(RAWINPUTHEADER))==sizeof(raw) && raw.header.dwType==RIM_TYPEMOUSE && !(raw.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE)) {
            const auto& m=raw.data.mouse;
            mouse_x=std::clamp(mouse_x+int(m.lLastX),-1000,1000);
            if(std::abs(m.lLastX)+std::abs(m.lLastY)>1)detected.store(5);
        }
    }
    return false; // retain F3, Alt+Enter, window management and DefWindowProc's raw-input cleanup
}
void JlhDiagnosticPad(int,input::Pad&);
void JlhControlsPad(int port,input::Pad& pad) {
    if(port==0) {
        // Losing focus releases keys, not the virtual keyboard controller's connection.
        pad.connected=true;
        static const bool trace=std::getenv("JLH_TRACE_CONTROLS")!=nullptr;
        static int last_menu=-1;
        if(trace && int(Menu())!=last_menu) {
            last_menu=int(Menu());LOGR("Controls: menu %d player %08X ui %08X",last_menu,loaded.load()?kernel::R32(0x44A88C):0,loaded.load()?kernel::R32(0x46A394):0);
        }
        bool used=pad.buttons || std::abs(int(pad.lx))>8000 || std::abs(int(pad.ly))>8000 || std::abs(int(pad.rx))>8000 || std::abs(int(pad.ry))>8000;
        for(auto value:pad.analog)used|=value>30;
        if(used)detected.store(pad.layout);
        if(Focused(static_cast<HWND>(app::Window()))) {
            std::lock_guard guard(state_lock);
            // Keep a stable 8-ms stick report even if the guest polls repeatedly within one frame.
            const uint64_t now=GetTickCount64();
            if(now-camera_tick>=8) {
                const auto elapsed=std::clamp<uint64_t>(now-camera_tick,8,40);
                camera_x=int16_t(std::clamp(mouse_x*12000/int(elapsed),-32767,32767));
                mouse_x=0;camera_tick=now;
            }
            pad.connected=true;
            for(unsigned i=0;i<4;++i)if(keys[std::array<unsigned,4>{VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT}[i]])pad.buttons|=uint16_t(1u<<i);
            if(keys[VK_TAB])pad.buttons|=0x20;
            if(keys['Q'])pad.buttons|=0x80; // RS: map
            if(keys['Z'] || keys[VK_MBUTTON])pad.buttons|=0x40; // LS: align camera
            // Latch the meaning at key-down: a held Esc cannot open and immediately close a menu.
            if(keys[VK_ESCAPE]) {if(escape_menu)pad.analog[1]=255;else pad.buttons|=0x10;}
            if(keys[VK_LBUTTON] || (keys[VK_RETURN] && !keys[VK_MENU] && Menu()))pad.analog[0]=255;
            if(keys[VK_RBUTTON])pad.analog[1]=255;
            if(keys['F'])pad.analog[2]=255;
            if(keys[VK_SPACE])pad.analog[3]=255;
            if(keys[VK_LSHIFT])pad.analog[6]=255;
            if(keys['E'])pad.analog[7]=255;
            const int x=int(keys['D'])-int(keys['A']), y=int(keys['W'])-int(keys['S']);
            const int magnitude=x&&y?23170:32767;
            if(x || y){pad.lx=int16_t(x*magnitude);pad.ly=int16_t(y*magnitude);}
            if(Menu()){wheel_units=zoom_direction=0;zoom_until=0;}
            else {
                if(now>=zoom_until) {
                    zoom_direction=0;
                    if(std::abs(wheel_units)>=WHEEL_DELTA) {
                        zoom_direction=wheel_units>0?1:-1;wheel_units-=zoom_direction*WHEEL_DELTA;
                        zoom_until=now+80; // one bounded native-stick pulse per notch
                    }
                }
                if(camera_x)pad.rx=camera_x;
                if(zoom_direction)pad.ry=int16_t(zoom_direction*32767);
            }
            if(trace) {
                static input::Pad last{};
                if(pad.buttons!=last.buttons || !std::equal(std::begin(pad.analog),std::end(pad.analog),std::begin(last.analog)) || pad.lx!=last.lx || pad.ly!=last.ly || pad.rx!=last.rx || pad.ry!=last.ry) {
                    LOGR("Controls: buttons %02X analog %u,%u,%u,%u,%u,%u,%u,%u left %d,%d menu %d layout %u right %d,%d",pad.buttons,pad.analog[0],pad.analog[1],pad.analog[2],pad.analog[3],pad.analog[4],pad.analog[5],pad.analog[6],pad.analog[7],pad.lx,pad.ly,int(Menu()),JlhActiveLayout(),pad.rx,pad.ry);
                    last=pad;
                }
            }
        }
    }
    JlhDiagnosticPad(port,pad);
}

