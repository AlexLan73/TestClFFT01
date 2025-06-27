#pragma once

#include <clocale>
#include <iostream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <boost/asio.hpp>
#include <fcntl.h>
#include <io.h>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <atomic>
#include <map>
#include <memory> // std::unique_ptr

#include "../src/fft/cl_fft_base.h"
#include "sockets/DuplexNode.h"
#include "wrap_thread.h"
#include "sockets/Test/socket_test.h"

class test_th_a: public wrap_thread
{
public:
    test_th_a(int id, int n): wrap_thread(id)
    {
        i = 0;
        this->n = n;
        is_ = true;
    }
    bool logic_while() override
    {
        return (i < n);
    }
    void my_func() override
    {
        i++;
        printf("thread id: %d  i: %d \n", id_, i);

        boost::this_thread::sleep_for(boost::chrono::milliseconds(750));
    }
    int i = 0;
    int n;
    bool is_ = true;
};

namespace wrapper
{
    class streams_sockets    //:public wrapper::cl_fft_base
    {
    public:
        streams_sockets();
        void inicial_vector(size_t n, size_t m = 1);
        ~streams_sockets();
        void test_boost();
        void test_sockets();
        std::unique_ptr<cl_fft_base> cl_fft_base_;
        std::shared_ptr<socket_test> sockets_a_; // = std::make_unique<>()
        std::shared_ptr<socket_test> sockets_b_;

    private:
        void generation_beam(size_t n, size_t m=1);
        void test_1();
        void test_2();

        boost::thread worker_thread_;
        std::vector<boost::thread> v_thread_;
        boost::condition_variable pause_cond_;
        v_fft data;
        io_context io_a;
        io_context io_b;
        std::map<int, std::unique_ptr<wrap_thread>> threads;
    };

}

// streams_sockets
