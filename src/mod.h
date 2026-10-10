#pragma once
#include <Windows.h>
#include <filesystem>
#include <string>
#include "Engine/Game/Input/controller_bindings.h"
#include "Engine/Game/Skater/first_person_spring.h"
#include "stick_look.h"
#include "native_controls.h"

namespace jishsfirstpersonmod {
struct Config {
    NativeControls native; // Observed settings/bindings, never saved in the mod INI.
    unsigned menu_key=VK_F9;
    std::uint32_t toggle{};
    dingosdk::FirstPersonLookControls look = dingosdk::default_look_controls;
    dingosdk::first_person::Settings spring;
    float fov{};
    bool link_smoothing=true, look_smoothing_initialized=false;
    dingosdk::first_person::Settings look_spring;
    bool look_only=false;
    bool third_person_bailing=false, on_board_only=false;
    bool disable_spring_on_foot=false;
    StickSettings stick;
    std::array<std::uint32_t,3> reserved{};
};
inline dingosdk::first_person::Settings glance_settings(const Config& config) {
    auto settings=config.link_smoothing ? config.spring:config.look_spring;
    settings.offset={}; settings.rotation={}; return settings;
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
GameplayInput read_controller();
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
