#pragma once
#include "Engine/Core/Hooks/hooks.h"
#include <vector>
#include <mutex>
namespace dingosdk {
class OwnedHookLedger {
    std::vector<void*> targets_;
public:
    void remember(void* target) { targets_.push_back(target); }
    template<class Remove> bool rollback(Remove remove) {
        bool ok=true;std::vector<void*> retained;
        for(auto i=targets_.rbegin();i!=targets_.rend();++i) {
            if(remove(*i)!=HookOk) { retained.push_back(*i);ok=false; }
        }
        targets_=std::move(retained);return ok;
    }
    std::size_t size() const { return targets_.size(); }
};
inline std::mutex owned_hook_mutex;
inline OwnedHookLedger owned_hook_ledger;
inline HookStatus owned_hook_prepare(void* target,void* replacement,void** original=nullptr) {
    std::lock_guard lock(owned_hook_mutex);
    const auto status=hook_prepare(target,replacement,original);
    if(status==HookOk) owned_hook_ledger.remember(target);
    return status;
}
inline bool rollback_owned_hooks() {
    std::lock_guard lock(owned_hook_mutex);
    return owned_hook_ledger.rollback([](void* target) { return hook_remove(target); });
}
inline bool restore_owned_slot(void* volatile* slot,void* replacement,void* original) {
    const auto current=InterlockedCompareExchangePointer(slot,original,replacement);
    return current==replacement || current==original;
}
}
