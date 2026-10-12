#include "menu.h"
#include "comfort_policy.h"
#include "player_state_contract.h"
#include "stick_runtime.h"
#include "game_camera_ownership.h"
#include "Extension/UI/skate_theme.h"
#include "Extension/UI/Overlay/skate_style.h"
#include <windowsx.h>
#include <algorithm>
#include <cfloat>
#include <cmath>

namespace jishsfirstpersonmod {
namespace {
ImFont* body{}; ImFont* heading{}; ImFont* title{};
BindingRecorder recorder;
std::string recorder_feedback,graphics_error;
ImFont* embedded(const wchar_t* name,float pixels) {
    HMODULE module{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&load_panel_fonts),&module);
    const auto resource=FindResourceW(module,name,RT_RCDATA);
    if(!resource) return nullptr;
    const auto data=LockResource(LoadResource(module,resource));
    ImFontConfig config; config.FontDataOwnedByAtlas=false;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<void*>(data),static_cast<int>(SizeofResource(module,resource)),pixels,&config);
}
void section(const char* text) {
    ImGui::PushFont(heading); ImGui::TextUnformatted(text); ImGui::PopFont();
    const auto at=ImGui::GetCursorScreenPos();
    const auto width=ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddRectFilled(at,{at.x+width,at.y+2},dingosdk::skate_theme::tile_light);
    ImGui::GetWindowDrawList()->AddRectFilled(at,{at.x+34,at.y+3},dingosdk::skate_theme::blue);
    ImGui::Dummy({0,9});
}
void settings_change(const Config& config) {
    Command cmd; cmd.kind=CommandKind::camera_settings; cmd.spring=config.spring; cmd.value=config.fov; submit_command(cmd);
}
void feed_input(HWND window) {
    auto& io=ImGui::GetIO();
    for(const auto& input:take_panel_input()) {
        if(input.message==WM_LBUTTONDOWN || input.message==WM_LBUTTONUP ||
           input.message==WM_RBUTTONDOWN || input.message==WM_RBUTTONUP ||
           input.message==WM_MBUTTONDOWN || input.message==WM_MBUTTONUP) {
            io.AddMousePosEvent(static_cast<float>(GET_X_LPARAM(input.lparam)),static_cast<float>(GET_Y_LPARAM(input.lparam)));
        }
        switch(input.message) {
        case WM_MOUSEMOVE: io.AddMousePosEvent(static_cast<float>(GET_X_LPARAM(input.lparam)),static_cast<float>(GET_Y_LPARAM(input.lparam))); break;
        case WM_LBUTTONDOWN: io.AddMouseButtonEvent(0,true); break;
        case WM_LBUTTONUP: io.AddMouseButtonEvent(0,false); break;
        case WM_RBUTTONDOWN: io.AddMouseButtonEvent(1,true); break;
        case WM_RBUTTONUP: io.AddMouseButtonEvent(1,false); break;
        case WM_MBUTTONDOWN: io.AddMouseButtonEvent(2,true); break;
        case WM_MBUTTONUP: io.AddMouseButtonEvent(2,false); break;
        case WM_MOUSEWHEEL: io.AddMouseWheelEvent(0,static_cast<float>(GET_WHEEL_DELTA_WPARAM(input.wparam))/WHEEL_DELTA); break;
        case WM_MOUSEHWHEEL: io.AddMouseWheelEvent(-static_cast<float>(GET_WHEEL_DELTA_WPARAM(input.wparam))/WHEEL_DELTA,0); break;
        case WM_CHAR: if(input.wparam>0 && input.wparam<0x10000) io.AddInputCharacterUTF16(static_cast<ImWchar16>(input.wparam)); break;
        case WM_KILLFOCUS: io.AddFocusEvent(false); break;
        case WM_KEYDOWN: case WM_KEYUP: {
            const bool down=input.message==WM_KEYDOWN;
            ImGuiKey key=ImGuiKey_None;
            const auto vk=input.wparam;
            if(vk>='A' && vk<='Z') key=static_cast<ImGuiKey>(ImGuiKey_A+(vk-'A'));
            else if(vk>='0' && vk<='9') key=static_cast<ImGuiKey>(ImGuiKey_0+(vk-'0'));
            else switch(vk) {
            case VK_TAB:key=ImGuiKey_Tab;break; case VK_LEFT:key=ImGuiKey_LeftArrow;break; case VK_RIGHT:key=ImGuiKey_RightArrow;break;
            case VK_UP:key=ImGuiKey_UpArrow;break; case VK_DOWN:key=ImGuiKey_DownArrow;break; case VK_HOME:key=ImGuiKey_Home;break;
            case VK_END:key=ImGuiKey_End;break; case VK_DELETE:key=ImGuiKey_Delete;break; case VK_BACK:key=ImGuiKey_Backspace;break;
            case VK_RETURN:key=ImGuiKey_Enter;break; case VK_ESCAPE:key=ImGuiKey_Escape;break; case VK_SPACE:key=ImGuiKey_Space;break;
            case VK_CONTROL:io.AddKeyEvent(ImGuiMod_Ctrl,down);break; case VK_SHIFT:io.AddKeyEvent(ImGuiMod_Shift,down);break;
            }
            if(key!=ImGuiKey_None) io.AddKeyEvent(key,down);
            break;
        }
        }
    }
    // GetCursorInfo is not filtered by stock's GetCursorPos input gate. Poll
    // after queued messages so old movement cannot override the latest position.
    CURSORINFO cursor{sizeof(CURSORINFO)};
    if(GetCursorInfo(&cursor)) {
        auto point=cursor.ptScreenPos;
        if(ScreenToClient(window,&point)) io.AddMousePosEvent(static_cast<float>(point.x),static_cast<float>(point.y));
    }
}
}
void publish_menu_error(const std::string& error) { graphics_error=error; log(error); }
void cancel_panel_recording() { recorder.cancel(); recorder_feedback.clear();cancel_menu_key_recording(); }
void load_panel_fonts() {
    auto& io=ImGui::GetIO(); io.IniFilename=nullptr; io.LogFilename=nullptr;
    io.BackendPlatformName="JishsFirstPersonMod Win32 messages";
    body=embedded(L"FONT_BODY",17); heading=embedded(L"FONT_HEADING",22); title=embedded(L"FONT_BRUSH",24);
    if(!body) body=io.Fonts->AddFontDefault(); if(!heading) heading=body; if(!title) title=heading;
}
void draw_panel_frame(const PanelFrame& frame) {
    auto& io=ImGui::GetIO(); io.DisplaySize=frame.display_size; io.DeltaTime=frame.delta_time;
    const bool visible=frame.visible,focused=frame.focused;
    if(!visible || !focused) { cancel_panel_recording(); io.ClearInputKeys(); io.ClearInputMouse(); hide_panel(); return; }
    io.AddFocusEvent(true); if(frame.window) feed_input(frame.window);
    ImGui::NewFrame();
    auto model=menu_snapshot(); auto config=model.config;
    const auto controller=frame.controller;
    const bool was_recording=recorder.recording;
    const auto action=recorder.action;
    if(const auto mask=recorder.update(controller,true,focused,GetTickCount64())) {
        Command command; command.kind=CommandKind::binding; command.action=action; command.mask=*mask;
        submit_command(command); recorder_feedback.clear(); model=menu_snapshot(); config=model.config;
    } else if(was_recording && !recorder.recording) recorder_feedback="Recording cancelled. Your binding is unchanged.";
    const float dpi=std::clamp(frame.dpi,1.0f,2.0f);
    io.FontGlobalScale=dpi;
    const ImVec2 maximum{std::max(200.0f,io.DisplaySize.x-16),std::max(180.0f,io.DisplaySize.y-16)};
    ImGui::SetNextWindowSize({std::min(500*dpi,maximum.x),std::min(680*dpi,maximum.y)},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({std::max(8.0f,io.DisplaySize.x-510*dpi),8},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({std::min(360*dpi,maximum.x),std::min(300*dpi,maximum.y)},maximum);
    const int colours=dingosdk::skate_theme::push_widget_colours();
    ImGui::PushFont(body);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0); ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{16*dpi,12*dpi});
    PanelBounds bounds{}; bool keyboard{};
    if(ImGui::Begin("JishsFirstPersonMod",nullptr,ImGuiWindowFlags_NoCollapse)) {
        const auto pos=ImGui::GetWindowPos(); const auto size=ImGui::GetWindowSize();
        const ImVec2 bounded{std::clamp(pos.x,0.0f,std::max(0.0f,io.DisplaySize.x-size.x)),std::clamp(pos.y,0.0f,std::max(0.0f,io.DisplaySize.y-size.y))};
        if(pos.x!=bounded.x || pos.y!=bounded.y) ImGui::SetWindowPos(bounded);
        bounds={static_cast<int>(bounded.x),static_cast<int>(bounded.y),static_cast<int>(bounded.x+size.x),static_cast<int>(bounded.y+size.y)};
        ImGui::PushFont(title); ImGui::TextUnformatted("FIRST PERSON CONTROLS"); ImGui::PopFont();
        ImGui::Text("Version %s",JISH_MOD_VERSION);
        ImGui::Text("Show/hide panel key: %s",menu_key_label(config.menu_key).c_str());
        if(menu_key_recording()) {
            ImGui::TextUnformatted("Press a keyboard key. Insert/Escape cancels.");
            if(ImGui::Button("Cancel key recording")) cancel_menu_key_recording();
        } else {
            if(ImGui::Button("Bind menu key")) { recorder.cancel();begin_menu_key_recording(); }
            ImGui::SameLine();
            if(ImGui::Button("Clear menu key")) { Command command;command.kind=CommandKind::menu_key;command.action=0;submit_command(command); }
        }
        ImGui::TextWrapped("Hides only this panel, including in the build editor. Insert still controls ReSkate's menu. Press the bound key again to show this panel while ReSkate's menu is open.");
        if(ImGui::Button("Load Jish's recommended preset")) {
            recorder.cancel();recorder_feedback.clear();load_recommended_preset();
            model=menu_snapshot();config=model.config;
        }
        ImGui::TextWrapped("Applies Jish's tested bindings and camera settings, then saves automatically. First Person activation and stock controls stay as they are.");
        if(ImGui::Checkbox("Compatibility mode (look controls only)",&config.look_only)) {
            Command command;command.kind=CommandKind::look_only;command.enabled=config.look_only;submit_command(command);config=menu_snapshot().config;
        }
        if(config.look_only) ImGui::TextWrapped("Right-stick camera look, glance look, First Person activation and comfort options remain available. Camera position, FOV and Spring are left to ReSkate. Head attachment, bail head-following, transition smoothing and character turning are disabled. Saved settings are preserved.");
        const auto comfort=comfort_snapshot();
        bool enabled=model.requested_enabled.value_or(comfort.initialized ? comfort.master:model.active);
        if(ImGui::Checkbox("Enable First Person",&enabled)) { Command command; command.kind=CommandKind::set_enabled; command.enabled=enabled; submit_command(command); }
        if(native_key_overlap(config.menu_key,config.native)) ImGui::TextWrapped("Panel hotkey overlaps a ReSkate binding. Open this panel with Insert and choose another panel key.");
        if(config.native.camera_cycle_available && config.third_person_bailing)
            ImGui::TextWrapped("First Person In Camera Cycle is automatically disabled while Third person while bailing is enabled. Your previous camera-cycle preference is restored when you turn this off.");
        else if(config.native.camera_cycle_available && config.cycle_restore_pending)
            ImGui::TextWrapped("Restoring your First Person In Camera Cycle preference...");
        if(config.native.active) ImGui::TextWrapped("ReSkate hotkeys also control First Person. A conflicting mod chord is paused to avoid a double toggle; other mod bindings remain available.");
        if(model.requested_enabled) ImGui::TextWrapped("View change queued until the native camera is ready.");
        if(!model.status.empty()) ImGui::TextWrapped("%s",model.status.c_str());
        if(!game_camera_suspended() && *game_camera_status()) ImGui::TextWrapped("%s",game_camera_status());
        ImGui::Spacing(); section("COMFORT OPTIONS");
        const auto comfort_change=[&]() {
            Command command; command.kind=CommandKind::comfort_options;
            command.third_person_bailing=config.third_person_bailing; command.on_board_only=config.on_board_only;
            submit_command(command);
        };
        ImGui::BeginDisabled(!comfort.bail_available && !config.third_person_bailing);
        if(ImGui::Checkbox("Third person while bailing",&config.third_person_bailing)) comfort_change();
        ImGui::EndDisabled();
        if(config.native.active) ImGui::TextWrapped("ReSkate stabilization works during normal play. During this mod's head attachment or bail head-following in full mode, native stabilization and Follow flips are temporarily bypassed; saved ReSkate settings are preserved.");
        if(!config.look_only && !config.third_person_bailing) ImGui::TextWrapped("First-person bails follow full head rotation and position. Right-stick look resumes when native movement control returns; glances pause until the bail ends.");
        if(!comfort.bail_available) ImGui::TextWrapped("Unavailable: native bail/movement contracts did not match. See the mod log.");
        ImGui::BeginDisabled(!comfort.board_available);
        if(ImGui::Checkbox("First person on board only",&config.on_board_only)) comfort_change();
        ImGui::EndDisabled();
        if(!comfort.board_available) ImGui::TextWrapped("Unavailable: riding, mounting and walking signals need in-game verification.");
        if(config.native.board_only) ImGui::TextWrapped("ReSkate also has Third person on foot enabled. Disable it in ReSkate to use first person while walking.");
        if(comfort.suppressed) ImGui::TextWrapped("%s",config.third_person_bailing && comfort.bailing ?
            "First Person armed - third person while bailing.":"First Person armed - third person while off board.");
        if(comfort.suspended) ImGui::TextWrapped("Player state unavailable; automatic switching suspended.");
        ImGui::Spacing(); section("ON-FOOT STICK LOOK");
        const auto stick_change=[&]() { Command command;command.kind=CommandKind::stick_settings;
            command.stick=config.stick;submit_command(command); };
        ImGui::BeginDisabled(!stick_runtime_ready() && !config.stick.enabled);
        if(ImGui::Checkbox("Right-stick look on foot",&config.stick.enabled)) stick_change();
        ImGui::EndDisabled();
        if(!stick_runtime_ready()) ImGui::TextWrapped("Native stick-look input/facing bridge unavailable; see the mod log.");
        ImGui::BeginDisabled(!config.stick.enabled);
        ImGui::BeginDisabled(config.look_only);
        int mode=config.stick.turn_character ? 1:0;
        if(ImGui::Combo("Look mode",&mode,"Camera only\0Camera and character\0")) {
            config.stick.turn_character=mode==1;stick_change();
        }
        ImGui::EndDisabled();
        if(ImGui::SliderFloat("Horizontal sensitivity",&config.stick.yaw_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        if(ImGui::SliderFloat("Vertical sensitivity",&config.stick.pitch_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        float deadzone=config.stick.deadzone*100;
        if(ImGui::SliderFloat("Stick deadzone",&deadzone,1,50,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) {
            config.stick.deadzone=deadzone*.01f;stick_change();
        }
        if(ImGui::Checkbox("Invert Y",&config.stick.invert_y)) stick_change();
        if(ImGui::Checkbox("Allow looking during jumps and climbing",&config.stick.look_during_parkour)) stick_change();
        ImGui::BeginDisabled(config.look_only);
        if(ImGui::Checkbox("Attach head position during mounting / jumping",&config.stick.lock_mount_jump)) stick_change();
        if(ImGui::Checkbox("Attach head position during hippy jumps",&config.stick.lock_hippy_jump)) stick_change();
        ImGui::TextWrapped("To see the effect of these attachment switches, turn Disable Spring on foot off. With it enabled, camera Spring is already disabled during off-board movement, so attachment changes may be less noticeable.");
        ImGui::TextWrapped("Right-stick look pauses when the native mounting flag is active, as it does on board. Hippy-jump attachment leaves ollies and ordinary board airtime unchanged.");
        ImGui::TextWrapped("Leave Allow looking during jumps and climbing off for the previous native view during climbing and enabled jump attachments. Turn it on to use right-stick/glance looking during those actions. This controls looking, not head-position attachment. Looking during special actions turns only the camera.");
        ImGui::Separator();
        if(ImGui::Checkbox("Smooth foot/board transitions",&config.stick.smooth_transitions)) stick_change();
        ImGui::BeginDisabled(!config.stick.smooth_transitions);
        if(ImGui::SliderFloat("Getting on board smoothing",&config.stick.mount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        if(ImGui::SliderFloat("Getting off board smoothing",&config.stick.dismount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Higher values soften the transition for longer. These controls are independent of camera and look Spring. Bails switch immediately.");
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Release the stick to centre after entering first person or changing mode. On board, the right stick remains available for tricks.");
        ImGui::Spacing(); section("CONTROLLER BINDS");
        if(ImGui::BeginTable("bindings",3,ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Action",ImGuiTableColumnFlags_WidthStretch,.85f);
            ImGui::TableSetupColumn("Controller combo",ImGuiTableColumnFlags_WidthStretch,1);
            ImGui::TableSetupColumn("Controls",ImGuiTableColumnFlags_WidthFixed,128*dpi);
            ImGui::TableHeadersRow();
            for(unsigned i=0;i<5;++i) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow(); ImGui::TableNextColumn();
                ImGui::TextWrapped("%s",i==0 ? "First Person":dingosdk::look_action_labels[i-1]);
                if((i==0 && model.conflicts.toggle) || (i && model.conflicts.glance[i-1])) ImGui::TextWrapped("Paused: overlaps another control.");
                if(i) {
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    float angle=config.look[i-1].angle;
                    if(ImGui::SliderFloat("##angle",&angle,1,dingosdk::look_angle_limit(i-1),"%.0f degrees",ImGuiSliderFlags_AlwaysClamp)) {
                        Command command; command.kind=CommandKind::angle; command.action=i; command.value=angle; submit_command(command);
                    }
                }
                ImGui::TableNextColumn();
                const auto mask=i==0 ? config.toggle:config.look[i-1].combo;
                const auto label=recorder.recording && recorder.action==i ? std::string("Recording..."):dingosdk::controller_combo_label(mask,controller.style);
                ImGui::TextWrapped("%s",label.c_str());
                ImGui::TableNextColumn();
                if(recorder.recording && recorder.action==i) {
                    if(ImGui::Button("Cancel",{-FLT_MIN,0})) { recorder.cancel(); recorder_feedback="Recording cancelled. Your binding is unchanged."; }
                } else {
                    ImGui::BeginDisabled(recorder.recording || !controller.available);
                    if(ImGui::Button("Record",{72*dpi,0})) { recorder.begin(i,GetTickCount64()); recorder_feedback.clear(); }
                    ImGui::EndDisabled(); ImGui::SameLine(); ImGui::BeginDisabled(recorder.recording || !mask);
                    if(ImGui::Button("Clear",{-FLT_MIN,0})) { Command command; command.kind=CommandKind::binding; command.action=i; submit_command(command); }
                    ImGui::EndDisabled();
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if(ImGui::Checkbox("Toggle glance controls",&config.toggle_glances)) {
            Command command;command.kind=CommandKind::toggle_glances;command.enabled=config.toggle_glances;submit_command(command);
        }
        if(recorder.recording) ImGui::TextWrapped("%s",recorder.capture.ready ? "Hold the buttons together, then release them to save.":"Release all controller buttons first.");
        else if(!controller.available) ImGui::TextWrapped("Connect a controller and keep the game focused to record.");
        else if(config.toggle_glances) ImGui::TextWrapped("Press a glance combo to look and hold; press it again to return. Another direction replaces the current glance. Menus, leaving First Person and interrupted controls clear the glance.");
        else ImGui::TextWrapped("Hold a look combo to glance; release to return. Opposite directions cancel.");
        if(!recorder_feedback.empty()) ImGui::TextWrapped("%s",recorder_feedback.c_str());
        ImGui::Spacing(); section("CAMERA SETTINGS");
        if(ImGui::Checkbox("Link look smoothing to camera spring",&config.link_smoothing)) {
            Command command; command.kind=CommandKind::link_smoothing; command.enabled=config.link_smoothing;
            submit_command(command); config=menu_snapshot().config;
        }
        if(!config.link_smoothing) {
            ImGui::TextUnformatted("Hotkey look smoothing");
            const auto look_change=[&]() { Command command; command.kind=CommandKind::look_settings;
                command.spring=config.look_spring; submit_command(command); };
            if(ImGui::Checkbox("Smooth hotkey looks",&config.look_spring.enabled)) look_change();
            const auto strength=[&](const char* label,float& value) {
                if(ImGui::SliderFloat(label,&value,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) look_change();
            };
            strength("Look up smoothing",config.look_spring.up); strength("Look down smoothing",config.look_spring.down);
            strength("Look left smoothing",config.look_spring.left); strength("Look right smoothing",config.look_spring.right);
            ImGui::TextWrapped("These values only affect hotkey turns and return to centre.");
            ImGui::TextWrapped("Changing linkage during a look takes effect after returning to centre.");
            ImGui::Separator();
        }
        ImGui::BeginDisabled(config.look_only);
        bool game_fov=config.fov==0;
        if(ImGui::Checkbox("Use game's default FOV",&game_fov)) { config.fov=game_fov ? 0.0f:90.0f; settings_change(config); }
        ImGui::BeginDisabled(game_fov);
        float fov=config.fov>0 ? config.fov:90;
        if(ImGui::SliderFloat("Field of view",&fov,40,120,"%.0f degrees",ImGuiSliderFlags_AlwaysClamp)) { config.fov=fov; settings_change(config); }
        ImGui::EndDisabled();
        if(ImGui::TreeNode("Spring arm and offsets")) {
            if(ImGui::Checkbox("Spring",&config.spring.enabled)) settings_change(config);
            if(ImGui::Checkbox("Disable Spring on foot",&config.disable_spring_on_foot)) {
                Command command;command.kind=CommandKind::spring_on_foot;command.enabled=config.disable_spring_on_foot;
                submit_command(command);config=menu_snapshot().config;
            }
            ImGui::TextWrapped("Leave this off if you want to see the difference between the mounting/jump and hippy-jump attachment settings.");
            if(config.disable_spring_on_foot) ImGui::TextWrapped("Camera Spring turns off while walking or doing parkour and restores your Spring setting on board. Linked look smoothing follows it; independent look and foot/board transition settings stay separate.");
            const auto slider=[&](const char* label,float& value,float low,float high,const char* format) {
                if(ImGui::SliderFloat(label,&value,low,high,format,ImGuiSliderFlags_AlwaysClamp)) settings_change(config);
            };
            ImGui::TextUnformatted("Position offset (metres)");
            slider("Right",config.spring.offset[0],-3,3,"%.3f"); slider("Up",config.spring.offset[1],-3,3,"%.3f"); slider("Forward",config.spring.offset[2],-3,3,"%.3f");
            ImGui::TextUnformatted("Rotation offset (degrees)");
            slider("Pitch",config.spring.rotation[0],-180,180,"%.1f"); slider("Yaw",config.spring.rotation[1],-180,180,"%.1f"); slider("Roll",config.spring.rotation[2],-180,180,"%.1f");
            ImGui::TextUnformatted("Smoothing strength");
            slider("Moving up",config.spring.up,0,100,"%.0f%%"); slider("Moving down",config.spring.down,0,100,"%.0f%%");
            slider("Moving left",config.spring.left,0,100,"%.0f%%"); slider("Moving right",config.spring.right,0,100,"%.0f%%");
            if(ImGui::Button("Reset arm settings")) { config.spring={}; settings_change(config); }
            ImGui::TextWrapped("Higher smoothing follows more slowly. Offsets also work with Spring off.");
            ImGui::TreePop();
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if(!model.feedback.empty()) ImGui::TextWrapped("%s",model.feedback.c_str());
        if(model.unsaved && model.feedback.starts_with("Applied, but not saved")) {
            if(ImGui::Button("Retry saving")) { Command command; command.kind=CommandKind::retry_save; submit_command(command); }
        }
        if(!graphics_error.empty()) ImGui::TextWrapped("%s",graphics_error.c_str());
        keyboard=io.WantCaptureKeyboard;
    }
    ImGui::End(); ImGui::PopStyleVar(3); ImGui::PopFont(); ImGui::PopStyleColor(colours);
    io.MouseDrawCursor=bounds.contains(static_cast<int>(io.MousePos.x),static_cast<int>(io.MousePos.y)) || (io.WantCaptureMouse && ImGui::IsMouseDown(0));
    set_panel_bounds(bounds,true,ImGui::IsMouseDown(0)&&io.WantCaptureMouse,keyboard);
    ImGui::Render();
}
void draw_panel(HWND window,bool visible) {
    RECT client{}; GetClientRect(window,&client);
    PanelFrame frame; frame.window=window; frame.visible=visible;
    frame.display_size={static_cast<float>(client.right),static_cast<float>(client.bottom)};
    frame.focused=GetAncestor(window,GA_ROOT)==GetForegroundWindow();
    frame.dpi=static_cast<float>(GetDpiForWindow(window))/96.0f;
    static LARGE_INTEGER frequency{},last{}; LARGE_INTEGER now{};
    if(!frequency.QuadPart) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);
    frame.delta_time=last.QuadPart ? std::clamp(static_cast<float>(double(now.QuadPart-last.QuadPart)/double(frequency.QuadPart)),.001f,.1f):1.0f/60;
    last=now;
    if(frame.visible && frame.focused) frame.controller=recording_controller();
    draw_panel_frame(frame);
}
}
