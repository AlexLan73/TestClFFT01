#pragma once
#include "data_fft.h"
#include "fft_data_time.h"

class a_print_base
{
public:
    virtual ~a_print_base() = default;

    virtual void print_data_test(const data_fft& data) {}
    virtual void print_data_test(const fft_data_time& data) {}
    virtual void print_data_test(const calc_time_opencl& times) {}
};