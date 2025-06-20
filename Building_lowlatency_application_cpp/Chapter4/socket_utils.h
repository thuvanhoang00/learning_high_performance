#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H
#include <iostream>
#include <string>
#include <unordered_set>
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
#include "macros.h"
#include "logging.h"
namespace thu{
    constexpr int MaxTCPServerBacklog = 1024;
    
    // Basic socket API
    auto getIfaceIP(const std::string &iface) -> std::string;
    auto setNonBlocking(int fd) -> bool;
    auto setNoDelay(int fd) -> bool;
    auto setSOTimestamp(int fd) -> bool;
    auto wouldBlock() -> bool;
    auto setMcastTTL(int fd, int ttl) noexcept -> bool;
    auto setTTL(int fd, int ttl) -> bool;
    auto join(int fd, const std::string &ip, const std::string &iface, int port) -> bool;
    auto createSocket(Logger &logger, const std::string &t_ip, const std::string &iface, int port, bool is_udp, bool is_blocking,
                        bool is_listening, int ttl, bool needs_so_timestamp) -> int;
}

#endif