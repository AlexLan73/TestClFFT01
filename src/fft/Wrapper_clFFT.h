#pragma once

#include <clocale>
#include <iostream>
#include <string>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include "cl_fft_base.h"

namespace my_fft
{
    class Wrapper_clFFT :public my_fft::cl_fft_base
    {
    public:
        Wrapper_clFFT();
        ~Wrapper_clFFT();

    private:

    };

}

