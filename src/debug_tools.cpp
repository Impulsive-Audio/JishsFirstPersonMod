#include "debug_tools.h"
#include "mod.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <numbers>
#include <string>
#include <vector>

namespace jishsfirstpersonmod {
namespace {
std::atomic<bool> enabled_flag{true};
std::atomic<int> enable_request{-1};
}
bool mod_enabled() noexcept { return enabled_flag.load(std::memory_order_acquire); }
void set_mod_enabled(bool enabled) noexcept { enabled_flag.store(enabled,std::memory_order_release); }
void request_mod_enable_toggle() noexcept { enable_request.store(mod_enabled() ? 0:1); }
int take_mod_enable_request() noexcept { return enable_request.exchange(-1); }

#ifndef JISH_DEV_TOOLS
namespace debug {
void publish(const dingosdk::first_person::Matrix&,const dingosdk::first_person::Matrix&,const dingosdk::first_person::Matrix*,const FrameFlags&) noexcept {}
void toggle_hud() noexcept {}
void toggle_recording() noexcept {}
bool hud_visible() noexcept { return false; }
bool recording() noexcept { return false; }
void tick() noexcept {}
}
#else
namespace debug {
namespace {
using Matrix=dingosdk::first_person::Matrix;
struct Readout { float yaw{},pitch{},roll{},x{},y{},z{}; };
struct Snapshot { Readout native,mod,shown; FrameFlags f; bool valid{},shown_valid{}; };
std::mutex state_mutex;
Snapshot snapshot;
std::atomic<bool> hud{};
std::atomic<bool> rec{};
HWND hud_window{};
bool hud_created{};
bool hud_shown{};
ULONGLONG hud_painted{},rec_flushed{};
std::FILE* rec_file{};
std::string rec_buffer;
unsigned long long rec_rows{};
unsigned rec_index{};

constexpr float pi=std::numbers::pi_v<float>;
float dot3(const float* a,const float* b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Readout readout(const Matrix& m) {
    Readout r;
    float forward[3]{-m[8],-m[9],-m[10]};
    const float fl=std::sqrt(dot3(forward,forward));
    if(fl>1e-4f) for(auto& v:forward) v/=fl;
    r.yaw=std::atan2(forward[0],forward[2])*180/pi;
    r.pitch=std::asin(std::clamp(forward[1],-1.f,1.f))*180/pi;
    // Roll: the up axis measured against the level up for this heading.
    float level_right[3]{-forward[2],0.f,forward[0]};
    const float rl=std::sqrt(dot3(level_right,level_right));
    if(rl>1e-4f) for(auto& v:level_right) v/=rl;
    float level_up[3]{level_right[1]*forward[2]-level_right[2]*forward[1],
                      level_right[2]*forward[0]-level_right[0]*forward[2],
                      level_right[0]*forward[1]-level_right[1]*forward[0]};
    const float up[3]{m[4],m[5],m[6]};
    const float side[3]={level_up[1]*up[2]-level_up[2]*up[1],level_up[2]*up[0]-level_up[0]*up[2],level_up[0]*up[1]-level_up[1]*up[0]};
    r.roll=std::atan2(dot3(side,forward),dot3(level_up,up))*180/pi;
    r.x=m[12];r.y=m[13];r.z=m[14];
    return r;
}
void format_readout(char* out,std::size_t size,const char* label,const Readout& r) {
    std::snprintf(out,size,"%-5s %8.2f %8.2f %8.2f   %9.3f %9.3f %9.3f",label,r.yaw,r.pitch,r.roll,r.x,r.y,r.z);
}
void paint() {
    if(!hud_window) return;
    Snapshot s;
    { std::lock_guard lock(state_mutex); s=snapshot; }
    char game[160]{"game  (none)"},mod[160]{"mod   (none)"},delta[160]{"delta (none)"};
    if(s.valid) {
        format_readout(game,sizeof game,"game",s.native);
        format_readout(mod,sizeof mod,"mod",s.mod);
        std::snprintf(delta,sizeof delta,"%-5s %8.2f %8.2f %8.2f   %9.3f %9.3f %9.3f","delta",
            s.mod.yaw-s.native.yaw,s.mod.pitch-s.native.pitch,s.mod.roll-s.native.roll,
            s.mod.x-s.native.x,s.mod.y-s.native.y,s.mod.z-s.native.z);
    }
    const char* actions[]{"none","mounting","jumping","hippy_candidate","hippy_jump"};
    const char* action=(s.f.action>=0 && s.f.action<5) ? actions[s.f.action]:"none";
    char head[200],line[200];
    std::snprintf(head,sizeof head,"Jish FP debug | mode %d custom %d action %s | elig %d susp %d yield %d r%d",
        s.f.mode,s.f.custom_mode,action,s.f.eligible?1:0,s.f.suspended?1:0,s.f.yield,s.f.reason);
    std::snprintf(line,sizeof line,"aim %.1f / %.1f cap %d act %d   sub %x/%x   req %d   rows %llu",
        s.f.aim_yaw,s.f.aim_pitch,s.f.aim_capture?1:0,s.f.aim_active?1:0,s.f.sub_offset,s.f.sub_vtable,s.f.requested?1:0,rec_rows);
    HDC dc=GetDC(hud_window);
    if(!dc) return;
    RECT client{};
    GetClientRect(hud_window,&client);
    HBRUSH background=CreateSolidBrush(RGB(12,12,14));
    FillRect(dc,&client,background);DeleteObject(background);
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(230,230,235));
    HFONT font=CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_MODERN,L"Consolas");
    HGDIOBJ previous=SelectObject(dc,font);
    TextOutA(dc,10,8,head,(int)std::strlen(head));
    TextOutA(dc,10,28,"             yaw     pitch     roll        x         y         z",(int)std::strlen("             yaw     pitch     roll        x         y         z"));
    SetTextColor(dc,RGB(150,190,255));TextOutA(dc,10,46,game,(int)std::strlen(game));
    SetTextColor(dc,RGB(150,255,170));TextOutA(dc,10,64,mod,(int)std::strlen(mod));
    SetTextColor(dc,RGB(255,200,120));TextOutA(dc,10,82,delta,(int)std::strlen(delta));
    SetTextColor(dc,RGB(180,180,190));TextOutA(dc,10,104,line,(int)std::strlen(line));
    if(rec.load()) {
        HBRUSH dot=CreateSolidBrush(RGB(235,35,35));
        HGDIOBJ old=SelectObject(dc,dot);
        Ellipse(dc,620,7,638,25);
        SelectObject(dc,old);DeleteObject(dot);
        SetTextColor(dc,RGB(255,80,80));TextOutA(dc,644,8,"REC",3);
    }
    SelectObject(dc,previous);DeleteObject(font);
    ReleaseDC(hud_window,dc);
}
void create_hud() {
    if(hud_created) return;
    hud_created=true;
    WNDCLASSEXW cls{sizeof cls};
    cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);
        cls.lpszClassName=L"JishFirstPersonDebugHud";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassExW(&cls);
    hud_window=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TRANSPARENT|WS_EX_LAYERED|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,
        cls.lpszClassName,L"Jish FP debug",WS_POPUP,40,40,700,132,nullptr,nullptr,cls.hInstance,nullptr);
    if(!hud_window) return;
    SetLayeredWindowAttributes(hud_window,0,225,LWA_ALPHA);
    ShowWindow(hud_window,SW_SHOWNOACTIVATE);
}
void flush_recording(bool force) {
    if(!rec_file) return;
    if(rec_buffer.empty()) return;
    std::fwrite(rec_buffer.data(),1,rec_buffer.size(),rec_file);
    rec_buffer.clear();
    if(force || rec_rows%120==0) std::fflush(rec_file);
}
}
void publish(const Matrix& native,const Matrix& final,const Matrix* shown,const FrameFlags& f) noexcept {
    try {
        const auto n=readout(native),m=readout(final);
        const bool shown_valid=shown!=nullptr;
        const auto s=shown_valid ? readout(*shown):Readout{};
        {
            std::lock_guard lock(state_mutex);
            snapshot={n,m,s,f,true,shown_valid};
            if(rec.load() && rec_file) {
                char row[400];
                std::snprintf(row,sizeof row,
                    "%llu,"
                    "%d," "%d," "%d," "%d," "%d,"
                    "%d," "%d," "%d," "%d," "%d,"
                    "%d," "%d," "%d," "%d," "%d,"
                    "%.2f,%.2f,"
                    "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
                    "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,"
                    "%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
                    GetTickCount64(),f.mode,f.custom_mode,f.action,f.jumping?1:0,f.mounting?1:0,
                    f.eligible?1:0,f.suspended?1:0,f.requested?1:0,f.active?1:0,f.yield,f.reason,f.sub_offset,f.sub_vtable,
                    f.aim_capture?1:0,f.aim_active?1:0,f.aim_yaw,f.aim_pitch,
                    n.yaw,n.pitch,n.roll,n.x,n.y,n.z,m.yaw,m.pitch,m.roll,m.x,m.y,m.z,
                    s.yaw,s.pitch,s.roll,s.x,s.y,s.z);
                rec_buffer+=row;++rec_rows;
                if(rec_buffer.size()>65536) flush_recording(false);
            }
        }
    } catch(...) {}
}
void toggle_hud() noexcept {
    const bool next=!hud.load();
    hud.store(next);
    if(next) { create_hud();if(hud_window) { ShowWindow(hud_window,SW_SHOWNOACTIVATE);SetWindowPos(hud_window,HWND_TOPMOST,40,40,700,132,SWP_NOACTIVATE|SWP_SHOWWINDOW);hud_shown=true;} }
    else if(hud_window) { ShowWindow(hud_window,SW_HIDE);hud_shown=false; }
    log(std::string("Debug HUD ")+(next?"enabled":"disabled"));
}
void toggle_recording() noexcept {
    const auto now=GetTickCount64();
    if(rec.load()) {
        flush_recording(true);
        if(rec_file) { std::fclose(rec_file);rec_file=nullptr; }
        rec.store(false);
        log("Debug recording stopped: "+std::to_string(rec_rows)+" rows");
        return;
    }
    const auto path=data_directory()/"logs"/("debug-recording-"+std::to_string(++rec_index)+".csv");
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(),error);
    rec_file=std::fopen(path.string().c_str(),"wb");
    if(!rec_file) { log("Debug recording could not open its file");return; }
    const char* header="time_ms,mode,custom_mode,action,jumping,mounting,eligible,suspended,requested,active,yield,reason,sub_offset,sub_vtable,aim_capture,aim_active,aim_yaw,aim_pitch,"
        "native_yaw,native_pitch,native_roll,native_x,native_y,native_z,"
        "mod_yaw,mod_pitch,mod_roll,mod_x,mod_y,mod_z,"
        "shown_yaw,shown_pitch,shown_roll,shown_x,shown_y,shown_z\n";
    std::fwrite(header,1,std::strlen(header),rec_file);
    rec_rows=0;rec_flushed=now;rec.store(true);
    log("Debug recording started: "+path.string());
}
bool hud_visible() noexcept { return hud.load(); }
bool recording() noexcept { return rec.load(); }
void tick() noexcept {
    const auto now=GetTickCount64();
    if(hud.load()) {
        // The readout belongs to the game window: hide it whenever the game is
        // not the foreground window, and bring it back on return.
        DWORD pid{};const auto foreground=GetForegroundWindow();
        const bool focused=foreground && GetWindowThreadProcessId(foreground,&pid) && pid==GetCurrentProcessId();
        if(hud_window && focused!=hud_shown) { hud_shown=focused;ShowWindow(hud_window,focused?SW_SHOWNOACTIVATE:SW_HIDE); }
        if(focused && hud_window && now-hud_painted>=100) { hud_painted=now;paint(); }
    }
    if(rec.load() && now-rec_flushed>=1000) { rec_flushed=now;std::lock_guard lock(state_mutex);flush_recording(false); }
}
}
#endif
}
