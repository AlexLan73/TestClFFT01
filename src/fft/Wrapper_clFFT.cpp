#include "Wrapper_clFFT.h"

my_fft::wrapper_cl_fft::wrapper_cl_fft():cl_fft_base()
{
    //SetConsoleOutputCP(CP_UTF8); //_setmode(_fileno(stdout), _O_U8TEXT); //std::wcout << L" ---  Старт программы Wrapper_clFFT  --- \n";

    running_ = false;
}

void my_fft::wrapper_cl_fft::inicial_vector(size_t n, size_t m)
{
    generation_beam(n, m);
    fft_data_time _fft_data_time = fft_data_time();
    std::any any_fft_data_time = _fft_data_time;
//    calculate(std::move(data), any_fft_data_time, n, m);
    auto correct_data = std::make_shared<v_fft>(data);

    calculate(std::move(correct_data), any_fft_data_time, n, m);
    _fft_data_time = std::any_cast<fft_data_time>(any_fft_data_time);

    auto dd = 1;
}

my_fft::wrapper_cl_fft::~wrapper_cl_fft()
{
    stop_thread();

}

void my_fft::wrapper_cl_fft::test_boost(){}

void my_fft::wrapper_cl_fft::start_thread() {
    if (running_) return; // уже запущен
    running_ = true;
    worker_thread_ = boost::thread(&wrapper_cl_fft::thread_func, this);
}

void my_fft::wrapper_cl_fft::stop_thread() {
    if (!running_) return;
    running_ = false;
    worker_thread_.interrupt(); // прервать поток
    if (worker_thread_.joinable())
        worker_thread_.join();
}

void my_fft::wrapper_cl_fft::pause() {
    paused_ = true;
}

void my_fft::wrapper_cl_fft::resume() {
    paused_ = false;
    pause_cond_.notify_one();
}

void my_fft::wrapper_cl_fft::thread_func()
{
    try {
        while (running_) {
            // Проверка на паузу
            boost::unique_lock<boost::mutex> lock(pause_mutex_);
            while (paused_) {
                pause_cond_.wait(lock);
                std::cout << "!!!!!! Working..." << std::endl;

            }
            lock.unlock();

            // Основная работа потока
            std::cout << "Working..." << std::endl;

            // Спать 500 мс, с возможностью прерывания
            boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
        }
    }
    catch (boost::thread_interrupted&) {
        std::cout << "Thread interrupted, exiting..." << std::endl;
    }
}

// Заполняем вектор
void my_fft::wrapper_cl_fft::generation_beam(size_t n0, size_t m0)
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
 
 #include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include <atomic>

class ThreadController {
public:
    ThreadController() : running_(false) {}

    void start() {
        if (running_) return; // уже запущен
        running_ = true;
        worker_thread_ = boost::thread(&ThreadController::threadFunc, this);
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        worker_thread_.interrupt(); // прервать поток
        if (worker_thread_.joinable())
            worker_thread_.join();
    }

    void pause() {
        paused_ = true;
    }

    void resume() {
        paused_ = false;
        pause_cond_.notify_one();
    }

    ~ThreadController() {
        stop();
    }

private:
    void threadFunc() {
        try {
            while (running_) {
                // Проверка на паузу
                boost::unique_lock<boost::mutex> lock(pause_mutex_);
                while (paused_) {
                    pause_cond_.wait(lock);
                }
                lock.unlock();

                // Основная работа потока
                std::cout << "Working..." << std::endl;

                // Спать 500 мс, с возможностью прерывания
                boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
            }
        }
        catch (boost::thread_interrupted&) {
            std::cout << "Thread interrupted, exiting..." << std::endl;
        }
    }

    boost::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> paused_{false};
    boost::mutex pause_mutex_;
    boost::condition_variable pause_cond_;
};

int main() {
    ThreadController controller;

    controller.start();

    boost::this_thread::sleep_for(boost::chrono::seconds(2));

    std::cout << "Pausing thread..." << std::endl;
    controller.pause();

    boost::this_thread::sleep_for(boost::chrono::seconds(2));

    std::cout << "Resuming thread..." << std::endl;
    controller.resume();

    boost::this_thread::sleep_for(boost::chrono::seconds(2));

    std::cout << "Stopping thread..." << std::endl;
    controller.stop();

    return 0;
}

 
 
 */
