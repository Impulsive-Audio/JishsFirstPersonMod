#pragma once
#include "mod.h"
#include "binding_conflicts.h"
#include <mutex>
#include <optional>

namespace jishsfirstpersonmod {
enum class CommandKind { look_only, set_enabled, binding, angle, camera_settings, retry_save, link_smoothing, look_settings, comfort_options, stick_settings, recommended_preset, menu_key, spring_on_foot, toggle_glances, cycle_restore };
struct Command {
    CommandKind kind{};
    unsigned action{}; // 0 toggle, 1..4 directional glances
    std::uint32_t mask{};
    float value{};
    bool enabled{};
    bool third_person_bailing{},on_board_only{};
    dingosdk::first_person::Settings spring;
    StickSettings stick;
    Config preset;
};
struct MenuSnapshot {
    Config config;
    std::uint64_t revision{};
    bool active{}, camera_available{}, unsaved{},binding_conflict{};
    BindingConflicts conflicts;
    std::optional<bool> requested_enabled;
    std::string status,feedback;
};
class SettingsStore {
public:
    SettingsStore(std::filesystem::path path,Config initial);
    bool submit(const Command& command,std::uint64_t now);
    MenuSnapshot snapshot() const;
    std::optional<bool> take_enabled();
    void publish_runtime(bool active,bool available,const std::string& status);
    void flush_due(std::uint64_t now);
    void refresh_reserved(const std::array<std::uint32_t,3>& reserved);
    void refresh_native_controls(const NativeControls& native);
    void set_feedback(const std::string& text);
    void mark_enabled_queued(bool accepted);
    void acknowledge_master(bool enabled);
private:
    void update_binding_conflicts(); // Caller holds mutex_, except during construction.
    std::filesystem::path path_;
    mutable std::mutex mutex_;
    MenuSnapshot model_;
    std::optional<bool> enabled_;
    std::uint64_t due_{};
};
struct InitialSettings { Config config; std::string message; bool save{}; };
Config import_profile(const std::filesystem::path& database);
InitialSettings load_initial_settings(const std::filesystem::path& target,
    const std::filesystem::path& legacy_directory,const std::filesystem::path& profile);
bool initialize_settings();
MenuSnapshot menu_snapshot();
bool submit_command(const Command& command);
bool load_recommended_preset();
using CameraSink=bool(*)(const Config&,const Config&,std::string&);
void set_camera_sink(CameraSink sink);
using ActivationSink=bool(*)(bool);
void set_activation_sink(ActivationSink sink);
using BackgroundFlush=void(*)();
void set_background_flush(BackgroundFlush flush);
bool submit_stock_command(const Command& command);
std::optional<bool> take_enabled_command();
void publish_runtime(bool active,bool available,const std::string& status);
void publish_native_controls(const NativeControls& native);
void acknowledge_master(bool enabled);
struct BindingRecorder {
    dingosdk::ControllerComboCapture capture;
    unsigned action{};
    std::uint64_t deadline{};
    bool recording{};
    void begin(unsigned index,std::uint64_t now);
    void cancel() noexcept;
    std::optional<std::uint32_t> update(dingosdk::ControllerInput input,bool visible,bool focused,std::uint64_t now);
};
dingosdk::ControllerInput recording_controller();
}
