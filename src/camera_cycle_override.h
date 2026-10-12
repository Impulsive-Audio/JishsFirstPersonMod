#pragma once
#include <optional>
#include <cstdint>
#include <mutex>
namespace jishsfirstpersonmod {
// Queue completion is observed through the native model. An opposite request
// may still be pending when the user turns the comfort option back off.
class CameraCycleRequests {
    std::mutex mutex_;
    std::optional<bool> queued_,last_attempt_;
    std::uint64_t next_retry_{};
public:
    bool settled(bool observed,bool target) {
        std::lock_guard lock(mutex_);
        if(queued_ && *queued_!=target) return false;
        if(observed!=target) return false;
        queued_.reset();return true;
    }
    bool attempt(bool target,std::uint64_t now) {
        std::lock_guard lock(mutex_);
        if(last_attempt_==target && now<next_retry_) return false;
        last_attempt_=target;next_retry_=now+1000;return true;
    }
    void accepted(bool target) { std::lock_guard lock(mutex_);queued_=target; }
};
}
