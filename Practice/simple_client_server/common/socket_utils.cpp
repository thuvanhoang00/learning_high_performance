#include "socket_utils.h"
#include <iostream>
#include <source_location>

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

auto createSocket(Logger& logger, const SocketCfg& socket_cfg)->int{
    std::string time_str;
    const auto ip = socket_cfg.ip_.empty() ? getIfaceIP(socket_cfg.iface_) : socket_cfg.ip_;
    logger.log("%:% %() % cfg:%\n", __FILENAME__, __LINE__, __FUNCTION__,
               getCurrentTimeStr(&time_str), socket_cfg.toString());

    int input_flags = (socket_cfg.is_listening_ ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
    const addrinfo hints{input_flags
        , AF_INET, socket_cfg.is_udp_ ? SOCK_DGRAM :SOCK_STREAM
        , socket_cfg.is_udp_ ? IPPROTO_UDP : IPPROTO_TCP
        , 0 
        , 0
        , nullptr
        , nullptr};
    
    addrinfo* result = nullptr;   
    std::cout << "ip: " << ip << ", iface: " << socket_cfg.iface_ << std::endl;
    const auto rc = getaddrinfo(ip.c_str(), std::to_string(socket_cfg.port_).c_str(), &hints, &result);
    ASSERT(!rc, std::string(__FILENAME__) + " getaddrinfo() failed. error:" + std::string(gai_strerror(rc)) + " errno:" + strerror(errno));

    int socket_fd = -1;
    int one = 1;
    for (addrinfo *rp = result; rp; rp = rp->ai_next) {
        ASSERT((socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1, std::string(__FILENAME__) +"socket() failed. errno:" + std::string(strerror(errno)));
        ASSERT(setNonBlocking(socket_fd), std::string(__FILENAME__) + std::string(__FILENAME__) +"setNonBlocking() failed. errno:"+std::string(strerror(errno)));

        // if(!socket_cfg.is_udp_) disable Nagle for TCP socket
        if(!socket_cfg.is_udp_){ // disable Nagle for TCP socket
            ASSERT(disableNagle(socket_fd), std::string(__FILENAME__) + std::string(__FILENAME__) +"disableNagle() failed. errno:" + std::string(strerror(errno)));
        }

        if (!socket_cfg.is_listening_)
        {
            // establish connection to the specified address
            ASSERT(connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 1, std::string(__FILENAME__) + std::string(__FILENAME__) +"connect() failed. errno:" +std::string(strerror(errno)));
        }

        if(socket_cfg.is_listening_){
            // allow re-using the address in the call to bind()
            ASSERT(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&one), sizeof(one)) == 0, std::string(__FILENAME__) +"setsockopt() SO_REUSEADDR failed. errno:" + std::string(strerror(errno)));
        }

        if(socket_cfg.is_listening_){
            // bind to the specified port number
            const sockaddr_in addr{AF_INET, htons(socket_cfg.port_), {htons(INADDR_ANY)}, {}};
            ASSERT(bind(socket_fd, socket_cfg.is_udp_ ? reinterpret_cast<const struct sockaddr *>(&addr) : rp->ai_addr, sizeof(addr)) == 0, std::string(__FILENAME__) +"bind() failed. errno:" + std::string(strerror(errno)));
        }

        if(!socket_cfg.is_udp_ && socket_cfg.is_listening_){
            // listen for incoming TCP connection
            ASSERT(listen(socket_fd, MaxTCPServerBacklog) == 0, std::string(__FILENAME__) + "listen() failed. errno:"+std::string(strerror(errno)));
        }

        if(socket_cfg.needs_so_timestamp_){
            // setSOtimestamp;
            ASSERT(setSOTimestamp(socket_fd), std::string(__FILENAME__) + "setSOTimestamp() failed. errno:" + std::string(strerror(errno)));
        }
    }
    
    return socket_fd;
}

auto wouldBlock()->bool{
    return errno == EWOULDBLOCK || errno == EINPROGRESS;
}

auto setSOTimestamp(int fd)->bool{
    int one = 1;
    return (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, reinterpret_cast<void*>(&one), sizeof(one)) != -1);
}

auto disableNagle(int fd) -> bool
{
    int one = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void*>(&one), sizeof(one)) != -1;
}

auto setNoDelay(int fd)->bool{
    int one = 1;
    return (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<void*>(&one), sizeof(one)) != -1);
}
} // end namespace
