#pragma once
#include "..\..\wrap_thread.h"
#include "..\..\sockets/DuplexNode.h"

class socket_test : public wrap_thread
{
public:
    socket_test(int id, unsigned short listen_port, unsigned short connect_port); //: wrap_thread(id);

    std::shared_ptr<DuplexNode> sockets_; // = std::make_unique<>()
    void run();

private:
    io_context io_;

    //void generation_beam(size_t n, size_t m = 1);
    //void test_1();
    //void test_2();

    //boost::thread worker_thread_;
    //std::vector<boost::thread> v_thread_;
    //boost::condition_variable pause_cond_;
    //v_fft data;

};

