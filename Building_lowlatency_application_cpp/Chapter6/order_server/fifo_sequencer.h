#pragma once
#include "common/thread_utils.h"
#include "common/macros.h"
#include "client_request.h"
namespace Exchange{
constexpr size_t ME_MAX_PENDING_REQUESTS = 1024;
class FIFOSequencer{
private:
    ClientRequestLFQueue *incoming_requests_ = nullptr;
    std::string time_str_;
    Logger *logger_ = nullptr;
    struct RecvTimeClientRequest{
        Nanos recv_time_ = 0;
        MEClientRequest request_;
        auto operator<(const RecvTimeClientRequest &rhs) const{
            return (recv_time_ < rhs.recv_time_);
        }
    };
    std::array<RecvTimeClientRequest, ME_MAX_PENDING_REQUESTS> pending_client_requests_;
    size_t pending_size_ = 0;
};
}