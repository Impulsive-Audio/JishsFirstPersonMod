#include "settings.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory(){return std::filesystem::current_path()/"activation-test-data";}
std::filesystem::path data_directory(){return module_directory();}
}
namespace {
unsigned requests{};bool last{},accept=true;
bool native_queue(bool enabled) { ++requests;last=enabled;return accept; }
void require(bool result,const char* text) { if(!result) throw std::runtime_error(text); }
}
int main() {
    using namespace jishsfirstpersonmod;
    std::filesystem::create_directories(data_directory());
    const auto isolated=(data_directory()/"test-user").wstring();
    SetEnvironmentVariableW(L"LOCALAPPDATA",isolated.c_str());
    require(initialize_settings(),"test settings startup failed");
    set_activation_sink(native_queue);
    Command command;command.kind=CommandKind::set_enabled;command.enabled=true;
    require(submit_command(command) && requests==1 && last,"companion toggle did not reach native queue directly");
    require(!take_enabled_command(),"companion activation incorrectly waited for or duplicated the input tick");
    require(menu_snapshot().requested_enabled==true,"queued menu state missing");
    publish_runtime(true,true,"Native First Person active");
    require(menu_snapshot().active && !menu_snapshot().requested_enabled,"native activation echo not reflected");
    command.enabled=false;
    require(submit_command(command) && requests==2 && !last,"companion disable did not use native queue");
    publish_runtime(false,true,"Native First Person off");
    require(!menu_snapshot().active && !menu_snapshot().requested_enabled,"native disable echo not reflected");
    accept=false;command.enabled=true;
    require(!submit_command(command) && !menu_snapshot().requested_enabled && !take_enabled_command(),"rejected native request left phantom activation pending");
    std::cout<<"Native menu activation forwarding and acknowledgement passed without a controller tick\n";
}
