#include "stick_runtime.h"
#include "stock_bridge.h"
#include "game_camera_ownership.h"
#include <Xinput.h>
#include <stdexcept>
#include <iostream>
ULONGLONG qa_now=10;bool qa_focus=true,qa_menu=false,qa_active=true,qa_spectator=false;bool qa_capture=false;
HWND qa_window(){return qa_focus?reinterpret_cast<HWND>(1):nullptr;}
DWORD qa_pid(HWND,DWORD* pid){*pid=GetCurrentProcessId();return 1;}
ULONGLONG qa_clock(){return qa_now;}
#define GetForegroundWindow qa_window
#define GetWindowThreadProcessId qa_pid
#define GetTickCount64 qa_clock
#include "../src/stick_input_filter.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
#undef GetTickCount64
namespace jishsfirstpersonmod {
thread_local bool raw_controller_read{};
void log(const std::string&) noexcept{}
bool stock_first_person_active(){return qa_active;}
bool stock_first_person_requested(){return qa_active;}
bool game_camera_suspended(){return qa_spectator;}
ComfortSnapshot comfort_snapshot(){ComfortSnapshot value;value.core=1;value.entity=2;value.context=3;value.rig=4;return value;}
bool overlay_available(bool& visible) noexcept{visible=qa_menu;return true;}
bool stick_captures_input(unsigned){return qa_capture;}
std::uintptr_t resolve_stock_handler(std::uintptr_t){return 0;}
}
using namespace jishsfirstpersonmod;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
XINPUT_STATE sample;
DWORD WINAPI input(DWORD,XINPUT_STATE* out){*out=sample;return ERROR_SUCCESS;}
unsigned hid_calls{};
void hid(const StockHidRead& read,DWORD n){++hid_calls;if(qa_menu&&read.buffer&&n)static_cast<unsigned char*>(read.buffer)[0]=0;}
int main(){
 emote_ready=true;originals[0]=input;original_hid=hid;
 dingosdk::FirstPersonLookControls bindings;for(unsigned i=0;i<4;++i)bindings[i].combo=0x100|(1u<<i);
 auto publish=[&](){publish_glance_filter(bindings,true,{1,2,3,4},1);};
 auto poll=[&](unsigned buttons,unsigned slot=0){sample.Gamepad.wButtons=buttons;publish();XINPUT_STATE out;SetLastError(123);check(filtered_state<0>(slot,&out)==ERROR_SUCCESS&&GetLastError()==123,"API result/error changed");return out.Gamepad.wButtons;};
 poll(0);qa_now=20;check(poll(0x1101)==0x1100,"unrelated button or LB consumed");
 qa_now=30;check(poll(0x1101)==0x1100&&!glance_emote_active(),"held first press duplicated");
 qa_now=40;poll(0x100);qa_now=50;check(poll(0x101)==0x101&&glance_emote_active(),"double tap not forwarded");
 qa_now=70;check(poll(0x101)==0x101&&glance_emote_active(),"second hold lost");qa_now=80;poll(0x100);check(!glance_emote_active(),"release stuck");
 clear_glance_filter();qa_now=100;poll(0x101,0);qa_now=110;poll(0x100,0);qa_now=120;check(poll(0x101,1)==0x100&&!glance_emote_active(),"different slots paired");
 for(unsigned interruption=0;interruption<4;++interruption){clear_glance_filter();qa_now+=100;poll(0x101);qa_now+=10;poll(0x100);
  if(interruption==0)qa_menu=true;if(interruption==1)qa_focus=false;if(interruption==2)qa_active=false;if(interruption==3)qa_spectator=true;
  qa_now+=10;check(poll(0x101)==0x101,"interrupted gameplay filtered");qa_menu=false;qa_focus=qa_active=true;qa_spectator=false;qa_now+=10;check(poll(0x101)==0x100&&!glance_emote_active(),"interruption retained tap");}
 clear_glance_filter();publish();raw_controller_read=true;check(poll(0x101)==0x101,"mod sampler was filtered");raw_controller_read=false;
 for(auto kind:{dingosdk::PlayStationPad::dualshock4,dingosdk::PlayStationPad::dualsense})for(bool bluetooth:{false,true}){
  clear_glance_filter();std::vector<std::uint8_t> report(bluetooth?78:64,0);const unsigned start=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?2:3):1;
  const unsigned button=start+(kind==dingosdk::PlayStationPad::dualsense?7:4);report[0]=bluetooth?(kind==dingosdk::PlayStationPad::dualsense?0x31:0x11):1;
  for(unsigned i=0;i<4;++i)report[start+i]=128;report[button+1]=1; // L1
  StockHidRead read{reinterpret_cast<void*>(10),report.data(),static_cast<unsigned>(report.size()),kind};
  auto submit=[&](unsigned hat,unsigned sequence){report[button]=0x20|hat; // preserve Cross
    if(kind==dingosdk::PlayStationPad::dualsense)report[start+6]=sequence;else report[start+6]=sequence<<2;
    sony_report_crc(report);qa_now+=10;publish();filtered_stock_hid(read,static_cast<DWORD>(report.size()));};
  submit(0,1);check((report[button]&15)==8&&(report[button]&0xf0)==0x20&&report[button+1]==1,"Sony first tap damaged other buttons");
  if(bluetooth) {
    std::uint32_t checksum{};std::memcpy(&checksum,report.data()+74,4);
    // Independent expected values computed with Python zlib.crc32 over 0xA1 + payload.
    check(checksum==(kind==dingosdk::PlayStationPad::dualsense ? 0x350f57b5u:0x797b0b63u),"Bluetooth first-press checksum invalid");
  }
  const auto edited=report;filtered_stock_hid(read,static_cast<DWORD>(report.size()));check(report==edited&&!glance_emote_active(),"duplicate shared completion became tap");
  submit(8,2);submit(0,3);check((report[button]&15)==0&&glance_emote_active(),"Sony double tap blocked");submit(0,4);check((report[button]&15)==0,"Sony second hold blocked");submit(8,5);check(!glance_emote_active(),"Sony release stuck");
  clear_glance_filter();private_controller_handle(read.file,true);submit(0,6);check((report[button]&15)==0,"private Sony reader filtered");private_controller_handle(read.file,false);
  if(bluetooth){auto expected=report;sony_report_crc(expected);check(expected==report,"Bluetooth checksum stale");}
 }
 auto changed=bindings;clear_glance_filter();qa_now+=10;poll(0x101);qa_now+=10;poll(0x100);changed[0].combo=0;
 publish_glance_filter(changed,true,{1,2,3,4},1);auto bypass=classify(1,0,0x101);check(!bypass.consume&&!bypass.emote,"changed binding still blocked");
 // Raw Input uses the same selective edit, including when existing stick filtering also edits the report.
 for(bool capture:{false,true}) {
  clear_glance_filter();qa_capture=capture;
  const unsigned bytes=offsetof(RAWINPUT,data.hid.bRawData)+64;
  std::vector<std::uint64_t> storage((bytes+7)/8);auto* record=reinterpret_cast<RAWINPUT*>(storage.data());
  record->header.dwType=RIM_TYPEHID;record->header.dwSize=bytes;record->header.hDevice=reinterpret_cast<HANDLE>(30);
  record->data.hid.dwSizeHid=64;record->data.hid.dwCount=1;devices[record->header.hDevice]=dingosdk::PlayStationPad::dualsense;
  auto* report=record->data.hid.bRawData;
  auto raw=[&](unsigned hat,unsigned sequence){std::fill_n(report,64,0);report[0]=1;report[1]=report[2]=128;report[3]=210;report[4]=150;report[7]=sequence;report[8]=hat;report[9]=1;qa_now+=10;publish();filter_record(record,bytes);};
  raw(0,1);check((report[8]&15)==8&&report[9]==1,"Raw Input glance leaked or LB changed");
  if(capture)check(report[3]==128&&report[4]==128,"existing stick filter lost");else check(report[3]==210&&report[4]==150,"glance filter changed sticks");
  filter_record(record,bytes);check(!glance_emote_active(),"duplicate Raw Input advanced tap state");
  raw(0,2);check((report[8]&15)==8&&!glance_emote_active(),"fresh held Raw Input became double tap");
  raw(8,3);raw(0,4);check((report[8]&15)==0&&glance_emote_active(),"Raw Input double tap blocked");
  const auto saved=std::vector<unsigned char>(report,report+64);raw_controller_read=true;filter_record(record,bytes);raw_controller_read=false;check(std::equal(saved.begin(),saved.end(),report),"private raw report changed");
 }
 qa_capture=false;
 clear_glance_filter();qa_now+=10;publish();classify(1,0,0x101);qa_now+=10;classify(1,0,0x100);qa_now+=101;
 check(!classify(1,0,0x101).consume,"stale camera eligibility consumed input");
 publish_glance_filter(bindings,true,{9,2,3,4},1);check(!classify(1,0,0x101).consume,"stale skater owner consumed input");
 publish();qa_now+=10;classify(1,0,0x101);qa_now+=10;classify(1,0,0x100);
 publish_glance_filter(bindings,true,{1,2,3,4},2);check(!classify(1,0,0x101).emote,"controller-set change paired taps");
 check(hid_calls>0,"original shared HID handler bypassed");
 std::cout<<"Actual XInput/shared HID adapters, private reads, interruptions and duplicated reports passed\n";
}
