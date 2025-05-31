#pragma once
#include "data_fft.h"
#include "CalcTimeOpenCl.h"

struct fft_data_time
{
    data_fft data_fft_;
    calc_time_opencl calc_time_opencl_;
};

