#pragma once

#include <clocale>
#include <iostream>
#include <string>

#include <windows.h>
#include <fcntl.h>
#include <io.h>

#include "ClFftBase.h"

namespace my_fft
{
    class Wrapper_clFFT :public my_fft::ClFftBase
    {
    public:
        Wrapper_clFFT();
        ~Wrapper_clFFT();

    private:

    };

}

