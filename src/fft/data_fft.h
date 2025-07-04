﻿#pragma once
#include <memory>
#include <vector>
#include <CL/cl_platform.h>

using v_fft = std::vector<cl_float2>;
using v_fft_many = std::vector<std::vector<cl_float2>>;
using v_fft_many_am = std::vector<std::vector<float>>;

struct data_fft
{
    std::shared_ptr<v_fft> data_one = nullptr;
    std::shared_ptr<v_fft_many> data_many = nullptr;
    std::shared_ptr<std::vector<float>> data_one_am = nullptr;
    std::shared_ptr<v_fft_many_am> data_many_am = nullptr;

};
