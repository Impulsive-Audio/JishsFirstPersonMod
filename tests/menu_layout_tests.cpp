#include "menu.h"
#include <imgui_internal.h>
#include "comfort_policy.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace jishsfirstpersonmod {
void log(const std::string&) noexcept {}
std::filesystem::path module_directory() { return std::filesystem::current_path(); }
std::filesystem::path data_directory() { return module_directory(); }
bool key_recording{};
void begin_menu_key_recording() { key_recording=true; }
void cancel_menu_key_recording() { key_recording=false; }
bool menu_key_recording() { return key_recording; }
std::string menu_key_label(unsigned) { return "F9"; }
std::vector<MenuInput> take_panel_input() { return {}; }
void hide_panel() {}
void set_panel_bounds(PanelBounds,bool,bool,bool,bool) {}
dingosdk::ControllerInput recording_controller() { return {}; }
ComfortSnapshot comfort_snapshot() { ComfortSnapshot s;s.initialized=true;s.board_available=true;s.bail_available=true;return s; }
bool stick_runtime_ready() { return true; }
bool glance_emote_filter_ready() { return true; }
bool bail_collision_available() noexcept { return true; }
bool game_camera_suspended() { return false; }
const char* game_camera_status() { return ""; }
bool spectator_ready=true;
bool spectator_available() noexcept { return spectator_ready; }
std::string spectator_status() { return spectator_ready ? "Spectator view: follow camera. Use the First Person binding to switch.":"First-person spectator view requires verified ReSkate 2.0.7."; }
}
int main() { try {
 using namespace jishsfirstpersonmod;
 for(float dpi:{1.0f,1.5f,2.0f}) {
  ImGui::CreateContext();load_panel_fonts();auto& io=ImGui::GetIO();
  unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);io.Fonts->SetTexID(1);
  PanelFrame frame;frame.dpi=dpi;frame.display_size={1000*dpi,800*dpi};
  draw_panel_frame(frame);
  ImGui::SetWindowSize("JishsFirstPersonMod",{360*dpi,680*dpi});
  for(int tab=0;tab<3;++tab) {
   // Click the real tab widget on the next frame.
   for(int pass=0;pass<4;++pass) {
    if(pass) { io.AddMousePosEvent((506+(tab+.5f)*112)*dpi,87*dpi+22);io.AddMouseButtonEvent(0,pass==1); }
    draw_panel_frame(frame);
   }
   auto* window=ImGui::FindWindowByName("JishsFirstPersonMod");
   if(!window) throw std::runtime_error("menu missing");
   if(window->ScrollMax.y>0) throw std::runtime_error("header/footer overflow at minimum panel width");
   if(tab==1) begin_menu_key_recording();
   if(tab==2 && key_recording) throw std::runtime_error("tab change did not cancel recording");
   for(int state=0;state<(tab==2 ? 2:1);++state) {
   if(state) { spectator_ready=false;draw_panel_frame(frame);draw_panel_frame(frame); }
   // Capture draw data and atlas for independent raster inspection.
   auto stem="menu-"+std::to_string(int(dpi*100))+"-"+std::to_string(tab)+(state ? "-disabled":"");
   std::ofstream out(stem+".json");out<<"{\"width\":"<<frame.display_size.x<<",\"height\":"<<frame.display_size.y<<",\"atlasWidth\":"<<width<<",\"atlasHeight\":"<<height<<",\"commands\":[";
   bool first=true;auto data=ImGui::GetDrawData();
   for(int l=0;l<data->CmdListsCount;++l) {auto list=data->CmdLists[l];for(const auto& cmd:list->CmdBuffer) {
    if(cmd.UserCallback) continue;
    for(unsigned i=0;i<cmd.ElemCount;i+=3) {
     if(!first) out<<',';first=false;out<<"["<<cmd.ClipRect.x<<','<<cmd.ClipRect.y<<','<<cmd.ClipRect.z<<','<<cmd.ClipRect.w;
     for(int j=0;j<3;++j) {auto v=list->VtxBuffer[list->IdxBuffer[cmd.IdxOffset+i+j]+cmd.VtxOffset];out<<','<<v.pos.x<<','<<v.pos.y<<','<<v.uv.x<<','<<v.uv.y<<','<<v.col;}
     out<<']';
    }
   }}out<<"]}";
   std::ofstream atlas(stem+".rgba",std::ios::binary);atlas.write(reinterpret_cast<char*>(pixels),width*height*4);
   }
   spectator_ready=true;
  }
  io.AddMousePosEvent(600*dpi,146.5f*dpi+53);io.AddMouseButtonEvent(0,true);draw_panel_frame(frame);
  io.AddMouseButtonEvent(0,false);draw_panel_frame(frame);
  draw_panel_frame(frame);
  auto* popup=ImGui::FindWindowByName("Save preset");
  if(!popup || !popup->Active) throw std::runtime_error("save name prompt missing");
  io.AddInputCharactersUTF8("My preset");draw_panel_frame(frame);
  auto* text=ImGui::GetInputTextState(popup->GetID("Name"));
  if(!text || std::string(text->TextA.Data)!="My preset" || !io.WantTextInput) throw std::runtime_error("preset text input/capture failed");
  ImGui::DestroyContext();
 }
 std::cout<<"Menu frames rendered at 100%, 150%, 200% DPI\n";
 } catch(const std::exception& e) { std::cerr<<e.what()<<"\n";return 1; }
}
