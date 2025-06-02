#pragma once

#include <clocale>
#include <iostream>
#include <string>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <atomic>

#include "cl_fft_base.h"

namespace my_fft
{
    class wrapper_cl_fft    //:public my_fft::cl_fft_base
    {
    public:
        wrapper_cl_fft();
        void inicial_vector(size_t n, size_t m = 1);
        ~wrapper_cl_fft();
        void test_boost();
        void start_thread();
        void stop_thread();
        void pause();
        void resume();
        void thread_func();
        std::unique_ptr<cl_fft_base> cl_fft_base_;


    private:
        void generation_beam(size_t n, size_t m=1);

        boost::thread worker_thread_;
        std::atomic<bool> running_;
        std::atomic<bool> paused_{ false };
        boost::mutex pause_mutex_;
        boost::condition_variable pause_cond_;
        v_fft data;

    };

}


