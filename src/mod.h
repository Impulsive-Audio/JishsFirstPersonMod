#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include <atomic>
#include "Engine/Game/Input/controller_bindings.h"
#include "Engine/Game/Skater/first_person_spring.h"
#include "stick_look.h"
#include "board_look.h"
#include "native_controls.h"

namespace jishsfirstpersonmod {
bool panel_gameplay_blocked();
inline std::atomic<int> active_camera_mode{}; // Current live camera mode, foot (1) or board (2).
struct Config {
    bool toggle_glances=false;
    bool remember_marker_glance=false;
    NativeControls native; // Observed settings/bindings, never saved in the mod INI.
    unsigned menu_key=VK_INSERT;
    std::uint32_t menu_combo{};
    std::uint32_t toggle{};
    dingosdk::FirstPersonLookControls look = dingosdk::default_look_controls;
    dingosdk::first_person::Settings spring;
    dingosdk::first_person::Vec3 board_offset{},foot_offset{}; // Per-mode camera position offsets.
    float fov{};
    bool link_smoothing=true, look_smoothing_initialized=false;
    dingosdk::first_person::Settings look_spring;
    dingosdk::first_person::Settings stick_spring;
    bool stick_smoothing_initialized=false;
    bool settings_migrated=false; // Read-time migration; not serialized.
    bool look_only=false;
    bool third_person_bailing=false, on_board_only=false;
    bool third_person_recovery=false;
    bool third_person_camera_switching() const { return third_person_bailing || (!look_only && third_person_recovery); }
    bool bail_camera_collision=true;
    bool bald_first_person=true;
    bool lock_body_flips=false;
    bool spectator_first_person=false;
    bool cycle_restore_pending=false,cycle_restore_enabled=false; // Native preference retained during comfort override.
    bool disable_spring_on_foot=false;
    StickSettings stick;
    BoardLookSettings board;
    std::array<std::uint32_t,3> reserved{};
};
// Swaps the live camera position offsets with the memory of the camera mode
// being entered: the outgoing mode saves the live offsets, the incoming mode's
// stored offsets become live. previous_mode 0 just adopts the incoming ones.
inline void adopt_mode_offsets(Config& config,int camera_mode,int previous_mode) {
    if((camera_mode!=1 && camera_mode!=2) || camera_mode==previous_mode) return;
    const dingosdk::first_person::Vec3 incoming=camera_mode==2 ? config.board_offset:config.foot_offset;
    if(previous_mode==1) config.foot_offset=config.spring.offset;
    else if(previous_mode==2) config.board_offset=config.spring.offset;
    config.spring.offset=incoming;
}
inline dingosdk::first_person::Settings glance_settings(const Config& config) {
    auto settings=config.link_smoothing ? config.spring:config.look_spring;
    settings.offset={}; settings.rotation={}; return settings;
}
inline void initialize_stick_smoothing(Config& config) {
    if(config.stick_smoothing_initialized) return;
    config.stick_spring=glance_settings(config);
    if(config.link_smoothing && config.disable_spring_on_foot && !config.look_only) config.stick_spring.enabled=false;
    config.stick_smoothing_initialized=true;
}
void log(const std::string& text) noexcept;
std::filesystem::path module_directory();
Config read_config(const std::filesystem::path& path);
void validate_config(const Config& config);
void write_config(const std::filesystem::path& path,const Config& config);
std::filesystem::path data_directory();
bool install_hooks(std::uintptr_t base);
bool install_pose_hooks(std::uintptr_t base);
bool native_tick_active() noexcept;
void set_hidden_joint(std::uintptr_t component, std::uint16_t joint, const std::array<float,3>& scale) noexcept;
void restore_hidden_head() noexcept;
void hide_current_head() noexcept;
bool begin_head_hide() noexcept;
void end_head_hide() noexcept;
struct GameplayInput:dingosdk::ControllerInput { float right_x{},right_y{},left_x{},left_y{}; };
GameplayInput read_controller(bool allow_menu=false);
void prepare_input();
void tick_mod(std::uintptr_t client) noexcept;
void camera_tick_begin() noexcept;
void camera_tick_end() noexcept;
bool camera_ready(std::uintptr_t context) noexcept;
void set_camera_phase_base(std::uintptr_t base);
void camera_native_return() noexcept;
bool take_toggle_request() noexcept;
bool take_reload_request() noexcept;
bool overlay_available(bool& visible) noexcept;
bool start_menu();
}
