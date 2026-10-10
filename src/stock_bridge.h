#pragma once
#include "mod.h"
namespace jishsfirstpersonmod {
bool start_stock_bridge(HMODULE module);
void stop_stock_bridge();
bool stock_bridge_ready();
bool stock_first_person_active();
bool stock_camera_available();
bool stock_first_person_requested();
bool request_stock_first_person(bool enabled);
bool request_user_first_person(bool enabled);
void poll_stock_camera(bool force=false);
int stock_camera_spring_enabled();
bool install_stock_camera_feed(std::uintptr_t base,HMODULE stock);
bool restore_stock_camera_feed();
void clear_stock_camera_blends();
void set_stock_glance(std::uintptr_t camera,float game_fov,float pitch,float yaw);
std::uintptr_t resolve_stock_handler(std::uintptr_t address);
}
