#pragma once

#include <any>
#include <chrono>
#include <iostream>

#include <fstream>
#include <numeric>
#include <CL/cl.h>
#include <clFFT.h>

#include <cmath>
#include <complex>

#include <stdexcept>
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <functional>


#include "CalcTimeOpenCl.h"
#include "data_fft.h"
#include "fft_data_time.h"

#include "SDataTimes.h"

using namespace std::chrono;

using namespace std;
using Complex = std::complex<float>;
//using v_fft = std::vector<cl_float2>;
//using v_fft_many = std::vector <vector<cl_float2>>;

namespace my_fft
{
    class ClFftBase
    {
    public:
        ClFftBase();
//        void load_data(std::any datax, size_t n, size_t m = 1);
        void Calculate(std::unique_ptr<std::any> datax, std::any& t, size_t n, size_t m = 1);
        void read_data(std::any& t);
        void process_type(const std::type_info& type);
        static calc_time_opencl calc_Time_OpenCl(cl_event event);
        ~ClFftBase();
    protected:
        size_t n_;      // point FFT
        size_t m_;      // beam
//        std::unique_ptr<std::any> data_;
        cl_int err_;
        cl_platform_id platform_;
        cl_device_id device_;
        cl_context context_;
        cl_command_queue queue_;
        cl_mem buffer_;
        clfftPlanHandle plan_;
        s_data_times s_data_times_;
    private:
        const double pi = 3.14159265358979323846;
//        void print_data_test(std::any);
        void print_data_test(data_fft data);
        void print_data_test(fft_data_time data);
        void print_data_test(calc_time_opencl times);
  //      cl_mem inputBuffer;
        //cl_mem outputBuffer;
//        std::vector<float> inputData;  // Реальная и мнимая части

        calc_time_opencl  time_opencl;

        static string time_average_value(system_clock::time_point* t_start, system_clock::time_point* t_end);
        std::unordered_map<std::type_index, std::function<void()>> typeActions_;
        v_fft input_data_one_;
//        std::shared_ptr<v_fft> d_vec_one = nullptr;
//        std::shared_ptr<v_fft_many> d_vec_many = nullptr;
    };

}

