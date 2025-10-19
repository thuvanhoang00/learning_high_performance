#pragma once
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sstream>
#include "logging.h"

namespace common{
struct SocketCfg{
    std::string ip_;
    std::string iface_;
    int port_ = -1;
    bool is_udp_ = false;
    bool is_listening_ = false;
    bool needs_so_timestamp_ = false;
    auto toString() const
    {
        std::stringstream ss;
        ss << "SocketCfg[ip:" << ip_
           << " iface:" << iface_
           << " port:" << port_
           << " is_udp:" << is_udp_
           << " is_listening:" << is_listening_
           << " needs_SO_timestamp:" << needs_so_timestamp_
           << "]";
        return ss.str();
    }
};


constexpr int MaxTCPServerBacklog = 1024;
auto getIfaceIP(const std::string& iface)->std::string;
auto setNonBlocking(int fd) -> bool;
auto setNoDelay(int fd) -> bool;
auto setSOTimestamp(int fd) -> bool;
auto disableNagle(int fd) -> bool; // disbale Nagle's algorithm and associated delays
auto join(int fd, const std::string& ip) -> bool;
auto createSocket(Logger& logger, const SocketCfg& socket_cfg)->int;
auto wouldBlock()->bool;


};