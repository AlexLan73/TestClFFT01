#pragma once
#define _WIN32_WINNT 0x0A00  // для windows 11
#include <boost/asio.hpp>
#include <thread>
#include <iostream>

using namespace boost::asio;
using ip::tcp;

class DuplexNode {
public:
    DuplexNode(io_context& io, unsigned short listen_port, unsigned short connect_port);

    void start();

    void start_accept();

    void start_connect();

    void read_from_socket(tcp::socket& socket);

    void write_to_socket(tcp::socket& socket);
private:
    io_context& io_;
    tcp::acceptor acceptor_;
    tcp::socket socket_server_;
    tcp::socket socket_client_;
    unsigned short connect_port_;
};

/*
//int main() {
//    io_service io;
//
//    // Программа А: слушает 10000, подключается к 10001
//    DuplexNode node(io, 10000, 10001);
//
//    node.start();
//
//    io.run();
//
//    return 0;
//}
*/