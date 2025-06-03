
#include <clocale>
#include <iostream>
#include <memory>
#include "src/streams_sockets.h"
#include "src/fft/CalcTimeOpenCl.h"
#include <locale>
#include <codecvt>
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <string>
#include <unordered_map>
#include <any>
#include <iostream>
#include <nvml.h>


int main()
{
    const double pi = 3.14159265358979323846;


    nvmlReturn_t result;
    result = nvmlInit();
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to initialize NVML: " << nvmlErrorString(result) << std::endl;
        return 1;
    }

    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device); // 0 — индекс GPU
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get handle for device 0: " << nvmlErrorString(result) << std::endl;
        nvmlShutdown();
        return 1;
    }

    unsigned int temp;
    result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get temperature: " << nvmlErrorString(result) << std::endl;
    }
    else {
        std::cout << "GPU Temperature: " << temp << " C" << std::endl;
    }

    nvmlShutdown();


    ///////////////////////////////////////////////////////////////

    //    SetConsoleOutputCP(CP_UTF8);
    //    _setmode(_fileno(stdout), _O_U8TEXT);

    //    std::wcout << L"Тест программы clFFT!" << std::endl;
    std::cout << "Test program clFFT!" << std::endl;
    setlocale(LC_ALL, "Russian");  // NOLINT(cert-err33-c, concurrency-mt-unsafe)
    //    std::cout << "Тест программы clFFT!" << std::endl;


    //    std::shared_ptr<Wrapper_clFFT> _wrapper_clfft = std::make_shared< Wrapper_clFFT>();
    std::unique_ptr<wrapper::streams_sockets> _wrapper_clfft = std::make_unique<wrapper::streams_sockets>();
    std::unordered_map<std::string, std::any> dict;
    dict["print"] = true;
    _wrapper_clfft->cl_fft_base_->set_params(dict);

//    _wrapper_clfft->start_thread_socket();

    _wrapper_clfft->test_boost();

    int ff = 1;

 //   _wrapper_clfft->inicial_vector(1 << 10, 10);

}

/*
    _wrapper_clfft->pause();
    _wrapper_clfft->start_thread();
    
    boost::this_thread::sleep_for(boost::chrono::milliseconds(3000));
    std::cout << "!!!!!!   boost::this_thread::sleep_for(boost::chrono::milliseconds(3000)) " << std::endl;

    _wrapper_clfft->resume();
//    _wrapper_clfft->stop_thread();
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));

    std::cout << "!!!!!!   +++++++++++1111  " << std::endl;

*/










    // Добавление разных типов
//    dict["print"] = true;
//    _wrapper_clfft->set_params(dict);

//    dict["double_value"] = 3.14;
//    dict["string_value"] = std::string("Hello");

//
//    const size_t N = 1 << 10;  //2048;
////    const size_t N = 1 << 18;  //
//    //std::unique_ptr<std::vector<cl_float2>> data_ = std::make_unique<std::vector<cl_float2>>(N);
//
////    std::vector<cl_float2>* inputData = new std::vector<cl_float2>(N);
//    // Заполнение сигнала, например, синусоидой
//
//
//    std::vector<cl_float2> data(N); // Обычный вектор, не unique_ptr
//
//    // 2. Заполняем вектор
//    const float freq = 5 * 2 * pi / N;
//    for (size_t i = 0; i < N; ++i) {
//        float val = std::sin(freq * i);
//        data[i].x = val;
//        data[i].y = 0.0f;
//    }
//
//    //// 3. Упаковываем вектор в std::any и создаём unique_ptr<std::any>
//    //auto correct_data_one = std::make_unique<std::any>(std::move(data));
//
//    //fft_data_time _fft_data_time1 = fft_data_time();
//    //std::any any_fft_data_time1 = _fft_data_time1;
//    ////_wrapper_clfft->calculate(std::move(correct_data), any_fft_data_time, N, M);
//    //// 4. Передаём в метод
//    //_wrapper_clfft->calculate(std::move(correct_data_one), any_fft_data_time1, N, 1);
//    //_fft_data_time1 = std::any_cast<fft_data_time>(any_fft_data_time1);
//
//
////
////    const int M = 10;
////    // 2. Заполняем вектор
////    std::vector<cl_float2> data(N*M);
////#pragma omp parallel for
////    for (size_t m = 0; m < M; ++m) {
////        for (size_t i = 0; i < N; ++i) 
////        {
////            float val = std::sin(2 * 3.14159265358979323846f * i * (m + 1) / N);;
////            data[m * N + i].x = val;
////            data[i].y = 0.0f;
////        }
////    }
////
////    // 3. Упаковываем вектор в std::any и создаём unique_ptr<std::any>
////    auto correct_data = std::make_unique<std::any>(std::move(data));
////
/////*
////    data_fft _data_fft1 = data_fft();
////    std::any any_data_fft1 = _data_fft1;
////
////    // 4. Передаём в метод
////    _wrapper_clfft->calculate(std::move(correct_data), any_data_fft1, N, M );
////    _data_fft1 = std::any_cast<data_fft>(any_data_fft1);
////*/
////   
////    fft_data_time _fft_data_time = fft_data_time();
////    std::any any_fft_data_time = _fft_data_time;
////    _wrapper_clfft->calculate( std::move(correct_data), any_fft_data_time, N, M); 
////    _fft_data_time = std::any_cast<fft_data_time>(any_fft_data_time);
//
//    int jj = 1;
//
//
//
//
//    //_wrapper_clfft->func_plan();
///*
//    calc_time_opencl _calc = calc_time_opencl();
//    std::any any_calc = _calc;
//    _wrapper_clfft->read_data(any_calc);
//    _calc = std::any_cast<calc_time_opencl>(any_calc);
//*/
//
////    data_fft _data_fft = data_fft();
////    std::any any_data_fft = _data_fft;
////    _data_fft = std::any_cast<data_fft>(any_data_fft);
//
///*
//    fft_data_time _fft_data_time = fft_data_time();
//    std::any any_fft_data_time = _fft_data_time;
//    _wrapper_clfft->read_data(any_fft_data_time);
//    _fft_data_time = std::any_cast<fft_data_time>(any_fft_data_time);
//*/
//
//}
//
//
