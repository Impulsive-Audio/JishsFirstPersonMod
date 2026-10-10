#include "settings.h"
#include <sqlite3.h>
#include <fstream>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <atomic>

namespace jishsfirstpersonmod {
SettingsStore::SettingsStore(std::filesystem::path path,Config initial):path_(std::move(path)) {
    validate_config(initial); model_.config=initial; model_.revision=1;update_binding_conflicts();
}
bool SettingsStore::submit(const Command& command,std::uint64_t now) {
    std::lock_guard lock(mutex_);
    try {
        if(command.kind==CommandKind::set_enabled) {
            enabled_=command.enabled; model_.requested_enabled=command.enabled; model_.feedback="Waiting for the camera update."; return true;
        }
        if(command.kind==CommandKind::retry_save) { model_.unsaved=true; due_=now; model_.feedback="Saving settings…"; return true; }
        auto next=model_.config;
        bool immediate{};
        switch(command.kind) {
        case CommandKind::spring_on_foot: next.disable_spring_on_foot=command.enabled;immediate=true;break;
        case CommandKind::menu_key:
            if(native_key_overlap(command.action,next.native)) throw std::runtime_error("That panel key is used by a native ReSkate control.");
            next.menu_key=command.action;immediate=true;break;
        case CommandKind::recommended_preset:
            next=command.preset;next.native=model_.config.native;next.reserved=model_.config.reserved;next.menu_key=model_.config.menu_key;
            validate_config(next);
            immediate=true;break;
        case CommandKind::stick_settings: next.stick=command.stick;break;
        case CommandKind::binding:
            if(command.action>4) throw std::runtime_error("Unknown controller action.");
            if(command.action==0) next.toggle=command.mask; else next.look[command.action-1].combo=command.mask;
            immediate=true; break;
        case CommandKind::angle:
            if(command.action<1 || command.action>4) throw std::runtime_error("Unknown look direction.");
            next.look[command.action-1].angle=command.value; break;
        case CommandKind::camera_settings: next.spring=command.spring; next.fov=command.value; break;
        case CommandKind::link_smoothing:
            if(!command.enabled && !next.look_smoothing_initialized) {
                next.look_spring=next.spring; next.look_spring.offset={}; next.look_spring.rotation={};
                next.look_smoothing_initialized=true;
            }
            next.link_smoothing=command.enabled; immediate=true; break;
        case CommandKind::look_settings:
            next.look_spring=command.spring; next.look_spring.offset={}; next.look_spring.rotation={};
            next.look_smoothing_initialized=true; break;
        case CommandKind::comfort_options:
            next.third_person_bailing=command.third_person_bailing; next.on_board_only=command.on_board_only;
            immediate=true; break;
        default: throw std::runtime_error("Unknown menu request.");
        }
        // Existing conflicts from a stock-side edit must not prevent fixing a
        // different row. Validate ranges globally and new chords individually.
        auto ranges=next; ranges.toggle=0; ranges.reserved={}; for(auto& look:ranges.look) look.combo=0;
        validate_config(ranges);
        if(command.kind==CommandKind::binding) {
            if(!dingosdk::valid_controller_combo(command.mask)) throw std::runtime_error("Unsupported controller buttons.");
            if(command.mask) {
                if(native_binding_overlap(command.mask,next.native)) throw std::runtime_error("That combo overlaps a native ReSkate control.");
                if(!next.native.active) for(auto reserved:next.reserved) if(reserved==command.mask) throw std::runtime_error("That combo is used by a stock Noclip or boost control.");
                if(command.action!=0 && next.toggle==command.mask) throw std::runtime_error("That combo is already assigned to First Person.");
                for(unsigned i=0;i<4;++i) if(command.action!=i+1 && next.look[i].combo==command.mask) throw std::runtime_error("That combo is already assigned to a look control.");
            }
        }
        model_.config=next; ++model_.revision; model_.unsaved=true;
        update_binding_conflicts();
        due_=now+(immediate ? 0:750);
        model_.feedback=command.kind==CommandKind::recommended_preset ?
            (model_.binding_conflict ? "Recommended preset applied. Marked hotkeys overlap native controls and are paused; re-record those bindings. Saving settings…":"Recommended preset applied. Saving settings…"):
            "Saving settings…"; return true;
    } catch(const std::exception& e) { model_.feedback=e.what(); return false; }
}
MenuSnapshot SettingsStore::snapshot() const { std::lock_guard lock(mutex_); return model_; }
void SettingsStore::set_feedback(const std::string& text) { std::lock_guard lock(mutex_); model_.feedback=text; }
void SettingsStore::mark_enabled_queued(bool accepted) {
    std::lock_guard lock(mutex_);enabled_.reset();
    if(!accepted) { model_.requested_enabled.reset();model_.feedback="Native First Person request rejected; see the stock menu status."; }
}
void SettingsStore::acknowledge_master(bool enabled) {
    std::lock_guard lock(mutex_);
    if(model_.requested_enabled==enabled) model_.requested_enabled.reset();
}
void SettingsStore::update_binding_conflicts() {
    model_.conflicts=find_binding_conflicts(model_.config);
    model_.binding_conflict=model_.conflicts.any();
    if(model_.binding_conflict) model_.feedback="Only conflicting bindings are paused. Clear or re-record the marked binding; right-stick look remains available.";
}
void SettingsStore::refresh_native_controls(const NativeControls& native) {
    std::lock_guard lock(mutex_);if(model_.config.native==native) return;
    model_.config.native=native;++model_.revision;
    update_binding_conflicts();
}
void SettingsStore::refresh_reserved(const std::array<std::uint32_t,3>& reserved) {
    for(auto value:reserved) if(!dingosdk::valid_controller_combo(value)) return;
    std::lock_guard lock(mutex_);
    if(model_.config.reserved==reserved) return;
    model_.config.reserved=reserved; ++model_.revision;
    update_binding_conflicts();
}
std::optional<bool> SettingsStore::take_enabled() { std::lock_guard lock(mutex_); const auto result=enabled_; enabled_.reset(); if(result) model_.requested_enabled.reset(); return result; }
void SettingsStore::publish_runtime(bool active,bool available,const std::string& status) {
    std::lock_guard lock(mutex_); model_.active=active; model_.camera_available=available; model_.status=status;
    if(model_.requested_enabled && *model_.requested_enabled==active) model_.requested_enabled.reset();
}
void SettingsStore::flush_due(std::uint64_t now) {
    Config next; std::uint64_t revision{};
    {
        std::lock_guard lock(mutex_);
        if(!model_.unsaved || now<due_) return;
        next=model_.config; revision=model_.revision;
    }
    try {
        write_config(path_,next);
        std::lock_guard lock(mutex_);
        if(revision==model_.revision) { model_.unsaved=false; model_.feedback=model_.binding_conflict ? "Settings saved. Marked hotkeys overlap native controls and are paused; re-record those bindings.":"Settings saved."; }
    } catch(const std::exception& e) {
        std::lock_guard lock(mutex_);
        model_.feedback=std::string("Applied, but not saved: ")+e.what();
        due_=~std::uint64_t{}; // Retry explicitly; never hammer an unwritable disk.
    }
}

Config import_profile(const std::filesystem::path& path) {
    sqlite3* raw{};
    const auto u8=path.u8string();
    if(sqlite3_open_v2(reinterpret_cast<const char*>(u8.c_str()),&raw,SQLITE_OPEN_READONLY,nullptr)!=SQLITE_OK) {
        if(raw) sqlite3_close(raw); throw std::runtime_error("The ReSkate profile is unavailable.");
    }
    const std::unique_ptr<sqlite3,decltype(&sqlite3_close)> database(raw,sqlite3_close);
    sqlite3_busy_timeout(raw,250);
    sqlite3_stmt* query{};
    if(sqlite3_prepare_v2(raw,"SELECT scope,name,value_json FROM player_settings WHERE scope IN ('options','/settings/bindings','/settings/first_person_look_angles','/settings/gameplay')",-1,&query,nullptr)!=SQLITE_OK)
        throw std::runtime_error("The profile has an unsupported settings schema.");
    const std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> statement(query,sqlite3_finalize);
    Config result;
    int status{};
    while((status=sqlite3_step(query))==SQLITE_ROW) {
        const std::string scope=reinterpret_cast<const char*>(sqlite3_column_text(query,0));
        const std::string name=reinterpret_cast<const char*>(sqlite3_column_text(query,1));
        const std::string text=reinterpret_cast<const char*>(sqlite3_column_text(query,2));
        auto numeric=[&]() { std::size_t used{}; const auto value=std::stod(text,&used);
            if(used!=text.size() || !std::isfinite(value)) throw std::runtime_error("Invalid saved camera value."); return value; };
        auto mask=[&]() {
            const auto value=numeric();
            if(value<0 || value>4294967295.0 || std::floor(value)!=value) throw std::runtime_error("Invalid saved controller mask.");
            const auto binding=static_cast<std::uint32_t>(value);
            // Keyboard activation belongs to ReSkate; do not import its tagged
            // value into the mod's controller-only recorder or reject the profile.
            if(native_keyboard_binding(binding)) return std::uint32_t{};
            if(!dingosdk::valid_controller_combo(binding)) throw std::runtime_error("Invalid saved controller mask.");
            return binding;
        };
        if(scope=="/settings/bindings") {
            if(name=="first_person") result.toggle=mask();
            const char* reserved[]{"noclip","forward_velocity","up_velocity"};
            for(unsigned i=0;i<3;++i) if(name==reserved[i]) result.reserved[i]=mask();
            for(unsigned i=0;i<4;++i) if(name==dingosdk::look_action_keys[i]) result.look[i].combo=mask();
        } else if(scope=="/settings/first_person_look_angles") {
            for(unsigned i=0;i<4;++i) if(name==dingosdk::look_action_keys[i]) result.look[i].angle=static_cast<float>(numeric());
        } else if(name.starts_with("ReSkate.FirstPerson.")) {
            const auto field=name.substr(20);
            if(field=="Spring") {
                if(text!="true" && text!="false") throw std::runtime_error("Invalid saved spring switch."); result.spring.enabled=text=="true";
            } else if(field=="Fov") result.fov=static_cast<float>(numeric());
            else {
                const char* offsets[]{"OffsetX","OffsetY","OffsetZ"}; const char* rotations[]{"Pitch","Yaw","Roll"};
                for(unsigned i=0;i<3;++i) {
                    if(field==offsets[i]) result.spring.offset[i]=static_cast<float>(numeric());
                    if(field==rotations[i]) result.spring.rotation[i]=static_cast<float>(numeric());
                }
                if(field=="Up") result.spring.up=static_cast<float>(numeric());
                if(field=="Down") result.spring.down=static_cast<float>(numeric());
                if(field=="Left") result.spring.left=static_cast<float>(numeric());
                if(field=="Right") result.spring.right=static_cast<float>(numeric());
            }
        }
    }
    if(status!=SQLITE_DONE) throw std::runtime_error("Reading the profile failed.");
    validate_config(result); return result;
}
InitialSettings load_initial_settings(const std::filesystem::path& target,const std::filesystem::path& legacy,const std::filesystem::path& profile) {
    if(std::filesystem::exists(target)) {
        try { return {read_config(target),"Saved settings loaded.",false}; }
        catch(const std::exception& e) { return {{},std::string("Saved settings could not be read; original file retained: ")+e.what(),false}; }
    }
    for(const auto name:{L"JishsFirstPersonMod.ini",L"MyMod.ini"}) {
        const auto path=legacy/name;
        if(std::filesystem::exists(path)) {
            try { return {read_config(path),"Previous mod settings imported.",true}; } catch(...) {}
        }
    }
    if(std::filesystem::exists(profile)) {
        try { return {import_profile(profile),"ReSkate first-person settings imported.",true}; }
        catch(const std::exception& e) { return {{},std::string("Using defaults; profile import failed: ")+e.what(),true}; }
    }
    return {{},"Choose Record to assign your controller controls.",true};
}
void BindingRecorder::begin(unsigned index,std::uint64_t now) { action=index; capture={}; deadline=now+30000; recording=true; }
void BindingRecorder::cancel() noexcept { recording=false; capture={}; }
std::optional<std::uint32_t> BindingRecorder::update(dingosdk::ControllerInput input,bool visible,bool focused,std::uint64_t now) {
    if(!recording) return {};
    if(!visible || !focused || !input.available || now>=deadline || (capture.device && capture.device!=input.device)) { cancel(); return {}; }
    const auto result=capture.update(input); if(result) recording=false; return result;
}

namespace {
SettingsStore* live_store{};
std::atomic<CameraSink> camera_sink{};
std::atomic<ActivationSink> activation_sink{};
std::atomic<BackgroundFlush> background_flush{};
HANDLE changed{};
std::filesystem::path profile_path;
std::optional<std::array<std::uint32_t,3>> profile_reserved() {
    sqlite3* raw{}; const auto u8=profile_path.u8string();
    if(sqlite3_open_v2(reinterpret_cast<const char*>(u8.c_str()),&raw,SQLITE_OPEN_READONLY,nullptr)!=SQLITE_OK) { if(raw) sqlite3_close(raw); return {}; }
    const std::unique_ptr<sqlite3,decltype(&sqlite3_close)> db(raw,sqlite3_close);
    sqlite3_busy_timeout(raw,50);
    sqlite3_stmt* statement{};
    if(sqlite3_prepare_v2(raw,"SELECT name,value_json FROM player_settings WHERE scope='/settings/bindings' AND name IN ('noclip','forward_velocity','up_velocity')",-1,&statement,nullptr)!=SQLITE_OK) return {};
    const std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> query(statement,sqlite3_finalize);
    std::array<std::uint32_t,3> result{}; const char* names[]{"noclip","forward_velocity","up_velocity"};
    int status{};
    try {
        while((status=sqlite3_step(statement))==SQLITE_ROW) {
            const std::string name=reinterpret_cast<const char*>(sqlite3_column_text(statement,0));
            const std::string text=reinterpret_cast<const char*>(sqlite3_column_text(statement,1));
            std::size_t used{}; const auto value=std::stod(text,&used);
            if(used!=text.size() || !std::isfinite(value) || value<0 || value>4294967295.0 || std::floor(value)!=value) return {};
            if(native_keyboard_binding(static_cast<std::uint32_t>(value))) continue;
            if(!dingosdk::valid_controller_combo(static_cast<std::uint32_t>(value))) return {};
            for(unsigned i=0;i<3;++i) if(name==names[i]) result[i]=static_cast<std::uint32_t>(value);
        }
    } catch(...) { return {}; }
    return status==SQLITE_DONE ? std::optional{result}:std::nullopt;
}
DWORD WINAPI saver(void*) noexcept {
    ULONGLONG next_profile{};
    for(;;) {
        WaitForSingleObject(changed,50);
        try {
            const auto now=GetTickCount64();
            if(now>=next_profile) { next_profile=now+1000; if(const auto reserved=profile_reserved()) live_store->refresh_reserved(*reserved); }
            live_store->flush_due(now);
            if(const auto flush=background_flush.load()) flush();
        } catch(...) {}
    }
}
}
bool initialize_settings() {
    const auto target=data_directory()/L"settings.ini";
    wchar_t local[32768]{};
    GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
    profile_path=std::filesystem::path(local)/L"ReSkate/profiles/offline/reskate.sqlite3";
    const auto initial=load_initial_settings(target,module_directory(),profile_path);
    live_store=new SettingsStore(target,initial.config);
    live_store->publish_runtime(false,false,"Waiting for the native camera.");
    log(initial.message);
    if(initial.save) {
        Command request; request.kind=CommandKind::retry_save; live_store->submit(request,GetTickCount64());
        live_store->flush_due(GetTickCount64());
    }
    if(!initial.save) live_store->set_feedback(initial.message);
    changed=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    const auto thread=changed ? CreateThread(nullptr,0,saver,nullptr,0,nullptr):nullptr;
    if(!thread) { log("ERROR: settings save worker could not start."); return false; }
    CloseHandle(thread); return true;
}
MenuSnapshot menu_snapshot() { return live_store ? live_store->snapshot():MenuSnapshot{}; }
bool submit_stock_command(const Command& command) {
    if(!live_store) return false;
    const auto accepted=live_store->submit(command,GetTickCount64());
    if(changed) SetEvent(changed); return accepted;
}
void set_camera_sink(CameraSink sink) { camera_sink=sink; }
void set_activation_sink(ActivationSink sink) { activation_sink=sink; }
void set_background_flush(BackgroundFlush flush) { background_flush=flush; }
bool submit_command(const Command& command) {
    const auto before=menu_snapshot().config;
    if(!submit_stock_command(command)) return false;
    if(command.kind==CommandKind::set_enabled) {
        if(const auto sink=activation_sink.load()) {
            const auto accepted=sink(command.enabled);
            live_store->mark_enabled_queued(accepted);return accepted;
        }
    }
    if(command.kind==CommandKind::camera_settings || command.kind==CommandKind::recommended_preset) {
        if(const auto sink=camera_sink.load()) {
            std::string error;
            if(!sink(before,menu_snapshot().config,error) && live_store)
                live_store->set_feedback("Applied in mod; stock sync failed: "+error);
        }
    }
    return true;
}
bool load_recommended_preset() {
    if(!live_store) return false;
    try {
        Command command;command.kind=CommandKind::recommended_preset;
        command.preset=read_config(module_directory()/L"presets/recommended.ini");
        return submit_command(command);
    } catch(const std::exception& error) {
        live_store->set_feedback(std::string("Recommended preset could not load: ")+error.what());return false;
    }
}
std::optional<bool> take_enabled_command() { return live_store ? live_store->take_enabled():std::optional<bool>{}; }
void publish_native_controls(const NativeControls& native) { if(live_store) live_store->refresh_native_controls(native); }
void publish_runtime(bool active,bool available,const std::string& status) { if(live_store) live_store->publish_runtime(active,available,status); }
void acknowledge_master(bool enabled) { if(live_store) live_store->acknowledge_master(enabled); }
}
