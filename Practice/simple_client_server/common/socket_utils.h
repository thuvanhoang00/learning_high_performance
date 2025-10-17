#pragma once
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <fcntl.h>

namespace common{
struct SocketCfg{
    std::string ip_;
    std::string iface_;
    int port_ = -1;
    bool isUdp_ = false;
    bool is_listening_ = false;
    bool needs_so_timestamp_ = false;
};

constexpr int MaxTCPServerBacklog = 1024;
auto getIfaceIP(const std::string& iface)->std::string;
auto setNonBlocking(int fd)->bool;
auto join(int fd, const std::string& ip) -> bool;
auto createSocket(const SocketCfg& socket_cfg)->int;



};