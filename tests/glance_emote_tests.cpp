#include "glance_emote_policy.h"
#include "glance_controls.h"
#include "stock_hid_contract.h"
#include <iostream>
#include <stdexcept>
using namespace jishsfirstpersonmod;
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<std::endl;throw std::runtime_error(message);}}
int main(){
 dingosdk::FirstPersonLookControls controls{{{0x101,30},{0x102,40},{0x104,60},{0x108,75}}};
 for(unsigned dir:{1u,2u,4u,8u}){
  GlanceEmotePolicy p;p.update(controls,0,0);
  auto first=p.update(controls,0x100|dir|0x1000,10);check(first.consume==dir&&!first.emote,"first tap leaked or triggered emote");
  for(unsigned t=20;t<=900;t+=10)check(!p.update(controls,0x100|dir,t).emote,"first hold counted as double tap");
  p.update(controls,0x100,910);check(!p.update(controls,0x100|dir,920).emote,"long first hold counted as double tap");
  p.update(controls,0x100,930);auto second=p.update(controls,0x100|dir,950);check(second.emote&&second.forward==dir&&!second.consume,"second tap blocked");
  for(unsigned t=960;t<2000;t+=10){auto held=p.update(controls,0x100|dir,t);check(held.forward==dir&&!held.consume&&!held.emote,"second hold blocked or retriggered");}
  auto released=p.update(controls,0x100,2010);check(!released.forward&&!released.consume,"emote release stuck");
 }
 for(unsigned delta:{250u,251u}){GlanceEmotePolicy p;p.update(controls,0x101,100);p.update(controls,0x100,110);check(p.update(controls,0x101,100+delta).emote==(delta==250),"timing boundary wrong");}
 {GlanceEmotePolicy p;p.update(controls,0x101,10);p.update(controls,0x100,20);check(!p.update(controls,0x102,30).emote,"different directions paired");
  p.reset();p.update(controls,0x101,10);p.update(controls,0,20);check(!p.update(controls,0x101,30).emote,"released modifier paired");
  p.reset();p.update(controls,0x101,10);p.update(controls,0x100,20);check(!p.update(controls,0x103,30).emote,"diagonal paired");}
 {GlanceEmotePolicy a,b;a.update(controls,0x101,10);a.update(controls,0x100,20);check(!b.update(controls,0x101,30).emote,"controllers paired");}
 auto other=controls;other[0].combo=0x201;other[1].combo=0x302;GlanceEmotePolicy p;
 check(!p.update(other,0x201,0).consume&&!p.update(other,0x302,10).consume,"unrelated binding consumed");
 for(bool toggle:{false,true}){GlanceControlsState look;auto step=[&](unsigned buttons){return look.update(controls,{true,buttons,1},false,toggle);};
  step(0);check(step(0x101)[0]==30,"single glance did not start");step(0x100);look.centre_for_emote(15);
  check(step(0x101)==std::array<float,2>{}&&step(0x101)==std::array<float,2>{},"second tap or hold reactivated glance");
  step(0x100);check(step(0x101)[0]==30,"release did not rearm glance");look.reset();step(0);check(step(0x101)[0]==30,"reset did not recover");}
 std::array<unsigned char,54> prefix=stock_hid_prefix;
 const auto reader=[&](std::uintptr_t address,void* out,std::size_t size){
  if(address==stock_hid_rva&&size==prefix.size()){std::memcpy(out,prefix.data(),size);return true;}
  for(auto caller:stock_hid_callers)if(address==caller&&size==5){std::array<unsigned char,5> call{0xe8};auto displacement=static_cast<std::int32_t>(stock_hid_rva-caller-5);std::memcpy(call.data()+1,&displacement,4);std::memcpy(out,call.data(),5);return true;}return false;};
 check(verified_stock_hid(reader,0),"reviewed HID contract rejected");prefix[0]^=1;check(!verified_stock_hid(reader,0),"changed HID prologue accepted");
 check(!verified_stock_hid([](auto,auto,auto){return false;},0),"missing HID contract accepted");
 std::cout<<"Tap timing, holds, modifier, directions, controllers, glance rearm and HID contract guards passed\n";
}
