#include "socket_utils.h"

// using namespace common;

namespace common{
auto getIfaceIP(const std::string& iface)->std::string{
    char buf[NI_MAXHOST] = {'\0'};
    ifaddrs* ifaddr = nullptr;
    if(getifaddrs(&ifaddr) != -1){
        for(ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next){
            getnameinfo(ifa->ifa_addr, sizeof(sockaddr_in), buf, sizeof(buf), nullptr, 0, NI_NUMERICHOST);
            break;
        }
        freeifaddrs(ifaddr);
    }
    return buf;
}

auto setNonBlocking(int fd)->bool{
    const auto flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return false;

    if(flags & O_NONBLOCK) return true;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

auto join(int fd, const std::string& ip)->bool{
    const ip_mreq mrep{{inet_addr(ip.c_str())}, {htonl(INADDR_ANY)}};
    return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mrep, sizeof(mrep)) != -1;
}

auto createSocket(const SocketCfg& socket_cfg)->int{
    std::string time_str;
    const auto ip = socket_cfg.ip_.empty() ? getIfaceIP(socket_cfg.iface_) : socket_cfg.ip_;
    const int input_flags = (socket_cfg.is_listening_ ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
    const addrinfo hints(input_flags, AF_INET, socket_cfg.isUdp_ ? SOCK_DGRAM :SOCK_STREAM, socket_cfg.isUdp_ ? IPPROTO_UDP : IPPROTO_TCP, 0 , 0, nullptr, nullptr);
    
    addrinfo* result = nullptr;   
    const auto rc = getaddrinfo(ip.c_str(), std::to_string(socket_cfg.port_).c_str(), &hints, &result);

    int socket_fd = -1;
    int one = 1;
    for (addrinfo *rp = result; rp; rp = rp->ai_next) {
        socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        setNonBlocking(socket_fd);

        // if(!socket_cfg.isUdp_) disable Nagle for TCP socket

        if (!socket_cfg.is_listening_)
        {
            // establish connection to the specified address
            connect(socket_fd, rp->ai_addr, rp->ai_addrlen);
        }

        if(socket_cfg.is_listening_){
            // allow re-using the address in the call to bind()
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof(one));
        }

        if(socket_cfg.is_listening_){
            // bind to the specified port number
            const sockaddr_in addr{AF_INET, htons(socket_cfg.port_), {htons(INADDR_ANY)}, {}};
            bind(socket_fd, socket_cfg.isUdp_ ? reinterpret_cast<const struct sockaddr*>(&addr):rp->ai_addr, sizeof(addr));
        }

        if(!socket_cfg.isUdp_ && socket_cfg.is_listening_){
            // listen for incoming TCP connection
            listen(socket_fd, MaxTCPServerBacklog);
        }

        if(socket_cfg.needs_so_timestamp_){
            // setSOtimestamp;
        }

    }
}
}