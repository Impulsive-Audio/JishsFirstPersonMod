#pragma once
#include "Engine/Game/Input/playstation_report.h"
#include <span>
namespace jishsfirstpersonmod {
inline bool neutral_sony_stick(dingosdk::PlayStationPad kind,std::span<std::uint8_t> report) {
    if(!dingosdk::parse_playstation_report(kind,report.size()>64,report.data(),report.size())) return false;
    const auto id=report[0];
    const unsigned start=id==0x31 ? 2:id==0x11 ? 3:1;
    report[start+2]=report[start+3]=128;
    if((id==0x31 || id==0x11) && report.size()>=78) {
        std::uint32_t crc=0xffffffff;
        const auto feed=[&](std::uint8_t byte) { crc^=byte;for(unsigned i=0;i<8;++i) crc=(crc>>1)^((crc&1) ? 0xedb88320:0); };
        feed(0xa1);for(std::size_t i=0;i<report.size()-4;++i) feed(report[i]);
        crc=~crc;for(unsigned i=0;i<4;++i) report[report.size()-4+i]=static_cast<std::uint8_t>(crc>>(8*i));
    }
    return true;
}
}
