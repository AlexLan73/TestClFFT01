// ReSharper disable CppClangTidyClangDiagnosticHeaderHygiene
#pragma once

#include <any>
#include <chrono>
#include <iostream>
#include <CL/cl.h>
#include <clFFT.h>

#include <complex>
#include <vector>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <functional>

#include "CalcTimeOpenCl.h"
#include "data_fft.h"
#include "fft_data_time.h"

#include "SDataTimes.h"
//#include <sstream>
//#include <iomanip>
//#include <ctime>
//#include <cstdio>
//#include <typeinfo>
//#include <stdexcept>
//#include <utility>
//#include <cmath>
//#include <fstream>
//#include <numeric>



using namespace std::chrono;  

using namespace std;
using Complex = std::complex<float>;

namespace my_fft
{
    class cl_fft_base  // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        cl_fft_base();
        void calculate(std::unique_ptr<std::any> data_signal, std::any& t, size_t n, size_t m = 1);
        ~cl_fft_base();
    protected:
        void process_fft_result(std::any& t);
        void process_type(const std::type_info& type);
        static calc_time_opencl calc_Time_OpenCl(cl_event event);

        size_t n_;      // point FFT
        size_t m_;      // beam
        cl_int err_;
        cl_platform_id platform_;
        cl_device_id device_;
        cl_context context_;
        cl_command_queue queue_;
        clfftPlanHandle plan_;
        s_data_times s_data_times_;
    private:
        const double pi_ = 3.14159265358979323846;
        void print_data_test(data_fft data);
        void print_data_test(fft_data_time data);
        void print_data_test(calc_time_opencl times);
        cl_mem input_buffer_;
        calc_time_opencl time_opencl_;

        static string time_average_value(system_clock::time_point* t_start, system_clock::time_point* t_end);
        std::unordered_map<std::type_index, std::function<void()>> typeActions_;
        v_fft input_data_one_;
    };

}

