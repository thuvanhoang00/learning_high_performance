#include <iostream>
#include <chrono>
#include <thread>

#include "common/socket_utils.h"
#include "common/tcp_server.h"
#include "common/tcp_socket.h"

int main(){
    using namespace common;
    
    const std::string iface = "lo";
    const std::string ip = "127.0.0.1";
    const int port = 12345;

    // Server
    TCPServer server;
    server.recv_callback_ = [](TCPSocket* socket, Nanos rx_time) noexcept {
        const std::string reply = "TCPServer received msg: " + std::string(socket->rcv_buffer_, socket->next_rcv_valid_index_);
        socket->next_rcv_valid_index_ = 0;
        socket->send(reply.data(), reply.length());
    };
    server.recv_finished_callback_ = [](){
        std::cout << "TCPServer::defaultRecvFinishedCallback\n";
    };
    server.listen(iface, port);


    // Clients
    std::vector<TCPSocket*> clients(5);
    for(size_t i = 0; i<clients.size(); ++i){
        clients[i] = new TCPSocket();
        clients[i]->recv_callback_ = [](TCPSocket* socket, Nanos rx_time) noexcept {
            socket->next_rcv_valid_index_ = 0;
        };
        std::cout << "Connecting TCPClient\n";
        clients[i]->connect(ip, iface, port, false);
        server.poll();
    }

    using namespace std::literals::chrono_literals;
    for(auto itr = 0; itr<5; ++itr){
        for(size_t i = 0; i<clients.size(); ++i){
            const std::string client_msg = "CLIENT-[" + std::to_string(i) + "] : Sending " + std::to_string(itr*100 + i);
            clients[i]->send(client_msg.data(), client_msg.length());
            clients[i]->sendAndRecv();
            std::this_thread::sleep_for(500ms);
            server.poll();
            server.sendAndRecv();
        }
    }

    for(auto itr = 0; itr<5; ++itr){
        for(auto& client : clients){
            client->sendAndRecv();
        }
        server.poll();
        server.sendAndRecv();
        std::this_thread::sleep_for(500ms);
    }
    
    return 0;
}