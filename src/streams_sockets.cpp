#include "streams_sockets.h"

wrapper::streams_sockets::streams_sockets()
{
    //SetConsoleOutputCP(CP_UTF8); //_setmode(_fileno(stdout), _O_U8TEXT); //std::wcout << L" ---  Старт программы Wrapper_clFFT  --- \n";
    cl_fft_base_ = std::make_unique<cl_fft_base>();
}

void wrapper::streams_sockets::inicial_vector(size_t n, size_t m)
{
    generation_beam(n, m);
    auto fft_data_time_ = fft_data_time();
    std::any any_fft_data_time = fft_data_time_;
    auto correct_data = std::make_shared<v_fft>(data);
    cl_fft_base_->calculate(std::move(correct_data), any_fft_data_time, n, m);
    fft_data_time_ = std::any_cast<fft_data_time>(any_fft_data_time);

    auto dd = 1;
}

wrapper::streams_sockets::~streams_sockets()
{

}

void wrapper::streams_sockets::test_boost()
{
    threads[1] = std::make_unique<test_th_a>(1, 10);
    threads[2] = std::make_unique<test_th_a>(2, 6);
    threads[3] = std::make_unique<test_th_a>(3, 12);

    // Запускаем потоки
    for (auto& pair : threads) {
        if (pair.second) {
            // Проверка, что поток существует
            if (!pair.second->is_thread()) {
                // Проверка, что поток можно запустить (не был присоединен или запущен ранее)
                std::cout << "Start POTOK  " << pair.first << std::endl;
                // Лямбда-функция для запуска run() в потоке
                pair.second->start();
            }
            else {
                std::cout << "Potok " << pair.first << " Error." << std::endl;
            }
        }
    }

    while (true)
    {
        if (bool is_test_join = std::ranges::all_of(
            threads | std::views::values,
            [](const auto& val) { return val->is_thread(); }
        )) break;
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    }
    std::cout << " -----1 -----" << std::endl;

//    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
//    std::cout << " -----2 -----" << std::endl;

    std::unordered_map<std::string, std::any> dict;
    dict["is_repeat"] = true;

    for (auto& val : threads | views::values)
        val->set_params(dict);
    std::cout << " -----3  SET TRUE -----" << std::endl;

    boost::this_thread::sleep_for(boost::chrono::milliseconds(10000));
    std::cout << " -----31111 -----" << std::endl;


    dict["is_repeat"] = false;
        for (auto& val : threads | views::values)
        val->set_params(dict);
    std::cout << " -----4 -----" << std::endl;


    // Ожидаем завершения потоков
    for (auto& pair : threads) 
    {
        if (pair.second && pair.second->is_thread()) {
            pair.second->thread_join();
        }
    }
    std::cout << " !!!!!!   END   TREAD   !!!!!!! " << std::endl;

    
}

void wrapper::streams_sockets::test_1()
{
    int n = 10;
    for (auto i=0; i<n; i++)
    {
        printf("test_1: %zo (mks)\n", i);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    }
}
void wrapper::streams_sockets::test_2()
{
    int n = 10;
    for (auto i = 0; i < n; i++)
    {
        printf("test_2: %zo (mks)\n", i);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    }

}

// Заполняем вектор
void wrapper::streams_sockets::generation_beam(size_t n0, size_t m0)
{
    const float pi_ = 3.14159265358979323846;
    const size_t N = n0; const size_t M = m0;
    data = v_fft(N*M); // Обычный вектор, не unique_ptr

    if(M == 1)
    {
        const float freq = 5 * 2 * pi_ / N;
#pragma omp parallel for

        for (size_t i = 0; i < N; ++i) 
        {
            data[i].x = std::sin(freq * i);
            data[i].y = 0.0f;
        }
    }else
    {
#pragma omp parallel for
        for (size_t m = 0; m < M; ++m) {
            for (size_t i = 0; i < N; ++i)
            {
                data[m * N + i].x = std::sin(2 * pi_ * i * (m + 1) / N);
                data[i].y = 0.0f;
            }
        }
    }
}


/*


 
 
 */
