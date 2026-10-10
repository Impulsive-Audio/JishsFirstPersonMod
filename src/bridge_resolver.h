#pragma once
#include "stock_contract.h"
#include <span>
#include <string>
#include <vector>
namespace jishsfirstpersonmod::compatibility {
struct Section { std::uint32_t start,size,flags; };
class Image {
public:
    std::vector<unsigned char> bytes;
    std::vector<Section> sections;
    std::uintptr_t status{};
    bool local_prefix{}; // live callback prefix; absolute branch ownership is checked separately
    bool parse(std::string& error);
    bool contains(std::uintptr_t rva,std::size_t size,unsigned flags) const;
};
struct Resolution {
    stock_contract::NativeContract contract{};
    std::uintptr_t read_callback{},queue_callback{};
    bool known{};
    std::string error;
    explicit operator bool() const { return error.empty() && contract.status_rva!=0; }
};
Resolution resolve(const Image&,const std::array<unsigned char,32>& hash,bool force_detection=false);
bool initialize(HMODULE module);
bool validate_callbacks(HMODULE module,std::uintptr_t read,std::uintptr_t queue);
bool enabled();
void enable();
void disable();
enum class Readiness { waiting,ready,incompatible };
inline Readiness camera_readiness(unsigned initialized,std::uintptr_t base,std::uintptr_t expected_base,
                                 std::uintptr_t callback,std::uintptr_t expected_callback) {
    if(!initialized) return Readiness::waiting;
    return initialized==1 && base==expected_base && callback==expected_callback ? Readiness::ready:Readiness::incompatible;
}
}
