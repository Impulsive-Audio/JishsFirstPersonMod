#include "menu.h"
#include "bail_collision.h"
#include "first_person_hair.h"
#include "comfort_policy.h"
#include "player_state_contract.h"
#include "stick_runtime.h"
#include "stick_input_filter.h"
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
void help_marker(const char* text) {
    if(ImGui::GetItemRectMax().x+ImGui::GetStyle().ItemSpacing.x+ImGui::CalcTextSize("(?)").x<=
       ImGui::GetWindowPos().x+ImGui::GetWindowContentRegionMax().x) ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize()*24);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
void fit_control(const char* label,bool has_help=false) {
    const float help_width=has_help ? ImGui::CalcTextSize("(?)").x+ImGui::GetStyle().ItemSpacing.x:0;
    ImGui::SetNextItemWidth(std::max(ImGui::GetFontSize()*4,
        ImGui::GetContentRegionAvail().x-ImGui::CalcTextSize(label).x-ImGui::GetStyle().ItemInnerSpacing.x-help_width));
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
    } else if(was_recording && !recorder.recording) recorder_feedback="Recording cancelled. Buttons unchanged.";
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
        ImGui::TextWrapped("Show or hide this panel while ReSkate's menu is open. Insert opens ReSkate's menu.");
        if(ImGui::Button("Load Jish's recommended preset")) {
            recorder.cancel();recorder_feedback.clear();load_recommended_preset();
            model=menu_snapshot();config=model.config;
        }
        help_marker("Keeps your current First Person on/off choice, ReSkate controls and panel key.");
        ImGui::TextWrapped("Applies Jish's recommended controls and camera settings. Saves automatically.");
        if(ImGui::Checkbox("Compatibility mode (look controls only)",&config.look_only)) {
            Command command;command.kind=CommandKind::look_only;command.enabled=config.look_only;submit_command(command);config=menu_snapshot().config;
        }
        help_marker("Keeps right-stick look, button glances, First Person controls and comfort options. ReSkate handles camera position, field of view and Spring. Head attachment, first-person bail movement, board transition smoothing and character turning are off. Your settings are kept for full mode.");
        ImGui::TextWrapped("Keeps look controls and comfort options. ReSkate handles the camera.");
        const auto comfort=comfort_snapshot();
        bool enabled=model.requested_enabled.value_or(comfort.initialized ? comfort.master:model.active);
        if(ImGui::Checkbox("Enable First Person",&enabled)) { Command command; command.kind=CommandKind::set_enabled; command.enabled=enabled; submit_command(command); }
        if(native_key_overlap(config.menu_key,config.native)) ImGui::TextWrapped("Panel key is already used by ReSkate. Press Insert and choose another key.");
        if(config.native.active) ImGui::TextWrapped("ReSkate's First Person hotkey also works here.");
        if(model.requested_enabled) ImGui::TextWrapped("Waiting for the camera to change view.");
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
        help_marker("First person returns when you regain control. Button glances pause until the bail ends. With this off in full mode, the camera follows your head during bails. ReSkate stabilization and Follow flips pause while the mod follows your head; your saved settings are kept.");
        ImGui::EndDisabled();
        ImGui::TextWrapped("Switches to third person during bails.");
        if(!config.third_person_bailing) {
            ImGui::BeginDisabled(config.look_only || !comfort.bail_available);
            if(ImGui::Checkbox("Third person while getting up",&config.third_person_recovery)) {
                Command command;command.kind=CommandKind::third_person_recovery;command.enabled=config.third_person_recovery;submit_command(command);
            }
            help_marker("Keeps the fall in first person, then lets ReSkate handle the camera while you get up. First person returns when you regain control.");
            ImGui::EndDisabled();
            ImGui::TextWrapped("Switches to third person while getting up. First person returns when you regain control.");
            if(config.look_only) ImGui::TextWrapped("Turn off Compatibility mode to use this option.");
        }
        if(config.native.camera_cycle_available && config.third_person_camera_switching()) {
            ImGui::TextWrapped("First Person In Camera Cycle stays off while this is enabled. Your previous setting returns when you turn it off.");
            help_marker("Your camera-cycle preference is kept across restarts. Changes made in ReSkate while this is on become the setting restored later. The First Person checkbox and hotkey still work.");
        } else if(config.native.camera_cycle_available && config.cycle_restore_pending)
            ImGui::TextWrapped("Restoring your First Person In Camera Cycle setting...");
        if(!config.look_only && !config.third_person_bailing) ImGui::TextWrapped("The camera follows your head during bails. Stick look returns when you regain control.");
        ImGui::BeginDisabled(config.look_only || config.third_person_bailing || !bail_collision_available());
        if(ImGui::Checkbox("Camera collision during bails",&config.bail_camera_collision)) {
            Command command;command.kind=CommandKind::bail_collision;command.enabled=config.bail_camera_collision;submit_command(command);
        }
        help_marker("Also applies while getting up if you stay in first person. The camera may move slightly away from your head near surfaces.");
        ImGui::EndDisabled();
        ImGui::TextWrapped("Keeps the camera clear of floors and objects during first-person bails.");
        if(config.look_only) ImGui::TextWrapped("Turn off Compatibility mode to use bail camera collision.");
        else if(config.third_person_bailing) ImGui::TextWrapped("Turn off Third person while bailing to use bail camera collision.");
        else if(!bail_collision_available()) ImGui::TextWrapped("Bail camera collision is unavailable. See the mod log.");
        if(!comfort.bail_available) ImGui::TextWrapped("Automatic bail switching is unavailable. See the mod log.");
        ImGui::BeginDisabled(!first_person_hair_available());
        if(ImGui::Checkbox("Bald in first person",&config.bald_first_person)) {
            Command command;command.kind=CommandKind::bald_hair;command.enabled=config.bald_first_person;submit_command(command);
        }
        help_marker("Temporarily uses the smooth bald hairstyle on your screen. Other players see your original hair. Your hair returns in third person and when this is turned off. Hair colour and saved outfits are kept.");
        ImGui::EndDisabled();
        ImGui::TextWrapped("Keeps long hair out of view. Other players see your original hair.");
        if(!first_person_hair_available()) ImGui::TextWrapped("Temporary bald hairstyle is unavailable. See the mod log.");
        ImGui::BeginDisabled(!comfort.board_available);
        if(ImGui::Checkbox("First person on board only",&config.on_board_only)) comfort_change();
        help_marker("Uses third person while walking. First person returns when you get on the board.");
        ImGui::EndDisabled();
        if(!comfort.board_available) ImGui::TextWrapped("Board-only switching is unavailable. See the mod log.");
        if(config.native.board_only) ImGui::TextWrapped("ReSkate's Third person on foot is also enabled. Turn it off to use first person while walking.");
        if(comfort.suppressed) ImGui::TextWrapped("%s",config.third_person_bailing && comfort.bailing ?
            "First person will return after the bail.":config.third_person_recovery && comfort.bailing ?
            "First person will return when you regain control.":"First person will return when you get on the board.");
        if(comfort.suspended) ImGui::TextWrapped("Automatic camera switching is paused. Waiting for your skater.");
        ImGui::Spacing(); section("ON-FOOT STICK LOOK");
        const auto stick_change=[&]() { Command command;command.kind=CommandKind::stick_settings;
            command.stick=config.stick;submit_command(command); };
        ImGui::BeginDisabled(!stick_runtime_ready() && !config.stick.enabled);
        if(ImGui::Checkbox("Right-stick look on foot",&config.stick.enabled)) stick_change();
        help_marker("Move the right stick to look around while walking in first person. Looking pauses while getting on the board.");
        ImGui::EndDisabled();
        if(!stick_runtime_ready()) ImGui::TextWrapped("Right-stick look is unavailable. See the mod log.");
        ImGui::BeginDisabled(!config.stick.enabled);
        ImGui::BeginDisabled(config.look_only);
        int mode=config.stick.turn_character ? 1:0;
        fit_control("Look mode",true);
        if(ImGui::Combo("Look mode",&mode,"Camera only\0Camera and character\0")) {
            config.stick.turn_character=mode==1;stick_change();
        }
        help_marker("Camera only lets you look without turning your skater. Camera and character turns your skater too. Compatibility mode uses camera only.");
        ImGui::EndDisabled();
        fit_control("Horizontal sensitivity");
        if(ImGui::SliderFloat("Horizontal sensitivity",&config.stick.yaw_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        fit_control("Vertical sensitivity");
        if(ImGui::SliderFloat("Vertical sensitivity",&config.stick.pitch_speed,1,720,"%.0f deg/s",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        float deadzone=config.stick.deadzone*100;
        fit_control("Stick deadzone",true);
        if(ImGui::SliderFloat("Stick deadzone",&deadzone,1,50,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) {
            config.stick.deadzone=deadzone*.01f;stick_change();
        }
        help_marker("Ignore small stick movements to prevent camera drift. Increase this if the camera moves by itself.");
        if(ImGui::Checkbox("Invert Y",&config.stick.invert_y)) stick_change();
        if(ImGui::Checkbox("Allow looking during jumps and climbing",&config.stick.look_during_parkour)) stick_change();
        help_marker("Enables right-stick look and button glances during climbing and supported jumps. Leave this off to keep ReSkate's normal view during climbing and attached jumps. These actions turn only the camera. This does not change head attachment, and look still pauses while getting on the board.");
        ImGui::TextWrapped("Allows camera look during jumps and climbing.");
        ImGui::BeginDisabled(config.look_only);
        if(ImGui::Checkbox("Attach head position during mounting / jumping",&config.stick.lock_mount_jump)) stick_change();
        help_marker("Follows your head position while getting on the board and during on-foot jumps. Disable Spring on foot can make the difference less noticeable. ReSkate stabilization and Follow flips pause while the mod follows your head; your settings are kept.");
        if(ImGui::Checkbox("Attach head position during hippy jumps",&config.stick.lock_hippy_jump)) stick_change();
        help_marker("Follows your head position during hippy jumps. Ollies and other board jumps are unchanged. Disable Spring on foot can make the difference less noticeable.");
        ImGui::TextWrapped("Makes the camera follow your head position during these actions.");
        ImGui::Separator();
        if(ImGui::Checkbox("Smooth foot/board transitions",&config.stick.smooth_transitions)) stick_change();
        help_marker("Adjusts getting on and off the board separately. These settings are separate from camera Spring and button-glance smoothing.");
        ImGui::BeginDisabled(!config.stick.smooth_transitions);
        fit_control("Getting on board smoothing");
        if(ImGui::SliderFloat("Getting on board smoothing",&config.stick.mount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        fit_control("Getting off board smoothing");
        if(ImGui::SliderFloat("Getting off board smoothing",&config.stick.dismount_smoothing,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) stick_change();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Higher values make getting on and off the board smoother. Bails switch immediately.");
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        ImGui::TextWrapped("Centre the stick after entering first person or changing look mode. On board, it still controls tricks.");
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
                if((i==0 && model.conflicts.toggle) || (i && model.conflicts.glance[i-1])) ImGui::TextWrapped("Paused: buttons already used.");
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
                    if(ImGui::Button("Cancel",{-FLT_MIN,0})) { recorder.cancel(); recorder_feedback="Recording cancelled. Buttons unchanged."; }
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
        if((config.toggle&0x100) && (config.toggle&15)) ImGui::TextWrapped("LB/L1 + D-pad First Person buttons do not trigger emotes.");
        help_marker("Opening menus, leaving first person or interrupted controls clears a held glance. With toggle off, hold the buttons to glance; opposite directions cancel.");
        if(glance_emote_filter_ready()) ImGui::TextWrapped("Hold LB/L1 and tap a bound D-pad direction to glance. Double tap to emote; hold the second tap for a held emote.");
        else ImGui::TextWrapped("Glance emote blocking is unavailable. See the mod log.");
        if(recorder.recording) ImGui::TextWrapped("%s",recorder.capture.ready ? "Hold the buttons together, then release them to save.":"Release all controller buttons first.");
        else if(!controller.available) ImGui::TextWrapped("Connect a controller and keep the game focused to record.");
        else if(config.toggle_glances) ImGui::TextWrapped("Press once to hold a glance. Press again to return. Another direction changes the glance.");
        else ImGui::TextWrapped("Hold the buttons to glance. Release to return.");
        if(!recorder_feedback.empty()) ImGui::TextWrapped("%s",recorder_feedback.c_str());
        ImGui::Spacing(); section("CAMERA SETTINGS");
        if(ImGui::Checkbox("Link look smoothing to camera spring",&config.link_smoothing)) {
            Command command; command.kind=CommandKind::link_smoothing; command.enabled=config.link_smoothing;
            submit_command(command); config=menu_snapshot().config;
        }
        help_marker("Uses camera Spring settings for button glances. Turn this off to adjust glance smoothing separately. Changes made during a glance take effect when you return to centre.");
        if(!config.link_smoothing) {
            ImGui::TextUnformatted("Hotkey look smoothing");
            const auto look_change=[&]() { Command command; command.kind=CommandKind::look_settings;
                command.spring=config.look_spring; submit_command(command); };
            if(ImGui::Checkbox("Smooth hotkey looks",&config.look_spring.enabled)) look_change();
            const auto strength=[&](const char* label,float& value) {
                fit_control(label);
                if(ImGui::SliderFloat(label,&value,0,100,"%.0f%%",ImGuiSliderFlags_AlwaysClamp)) look_change();
            };
            strength("Look up smoothing",config.look_spring.up); strength("Look down smoothing",config.look_spring.down);
            strength("Look left smoothing",config.look_spring.left); strength("Look right smoothing",config.look_spring.right);
            ImGui::TextWrapped("Smooths button glances and their return to centre.");
            ImGui::Separator();
        }
        ImGui::BeginDisabled(config.look_only);
        bool game_fov=config.fov==0;
        if(ImGui::Checkbox("Use game's default FOV",&game_fov)) { config.fov=game_fov ? 0.0f:90.0f; settings_change(config); }
        ImGui::BeginDisabled(game_fov);
        float fov=config.fov>0 ? config.fov:90;
        fit_control("Field of view");
        if(ImGui::SliderFloat("Field of view",&fov,40,120,"%.0f degrees",ImGuiSliderFlags_AlwaysClamp)) { config.fov=fov; settings_change(config); }
        ImGui::EndDisabled();
        if(ImGui::TreeNode("Spring arm and offsets")) {
            if(ImGui::Checkbox("Spring",&config.spring.enabled)) settings_change(config);
            help_marker("Smooths how the camera follows your skater. Position offsets move the camera; rotation offsets change where it points. Higher smoothing values make it follow more slowly.");
            if(ImGui::Checkbox("Disable Spring on foot",&config.disable_spring_on_foot)) {
                Command command;command.kind=CommandKind::spring_on_foot;command.enabled=config.disable_spring_on_foot;
                submit_command(command);config=menu_snapshot().config;
            }
            help_marker("Turns camera Spring off while walking, jumping and climbing, then restores your Spring setting on board. Linked glance smoothing follows this; separate glance and board transition smoothing are unchanged. Leave this off to see jump attachment changes more clearly.");
            if(config.disable_spring_on_foot) ImGui::TextWrapped("Spring is off on foot and returns on board. Linked glance smoothing follows this setting.");
            const auto slider=[&](const char* label,float& value,float low,float high,const char* format) {
                fit_control(label);
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
            ImGui::TextWrapped("Higher values make the camera follow more slowly. Offsets work with Spring off.");
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
