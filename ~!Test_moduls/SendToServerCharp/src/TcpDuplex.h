#pragma once
#define _WIN32_WINNT 0x0A00  // для windows 11

#include <string>
#include <iostream>
#include <functional>
#include <utility>
#include <thread>
#include <stop_token>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <atomic>


#include <algorithm> // для std::swap
#include <ranges>

#include <boost/asio.hpp>
#include <yaml-cpp/yaml.h>

#include "concurrentqueue.h"
#include <vector>
#include <tuple>


//#include "ReadWriteYaml.h"
#include "IpAddressOne.h"
#include "MSocket01.h"

#include "manual_reset_event.h"

using namespace moodycamel;


class TcpDuplex
{
public:
//    using DataTuple = std::tuple<std::vector<uint8_t>, std::vector<uint8_t>>;
    ip_address_one ip_address_one_;


    TcpDuplex(const ip_address_one& ip_address);
    ~TcpDuplex();

    /**
         * \brief Запускает цикл чтения в отдельном потоке.
         * \param stoken Токен для корректной остановки потока.
         */
    void run_read(std::stop_token stoken);

    /**
     * \brief Запускает цикл отправки в отдельном потоке.
     * \param stoken Токен для корректной остановки потока.
     */
    void run_send(std::stop_token stoken);

    /**
     * \brief Запускает оба потока: чтения и отправки.
     */
    void start_thread();

    /**
     * \brief Запрашивает остановку обоих потоков.
     */
    void stop_thread();

    void stop_working();

    void reconnect();

    void send_data(const std::string& data);
    void read_data();
    // Метод для инициализации внешней функции
    void initialization_external_function(std::function<bool(const std::string&)> parser_func)
    {
        parser_external_function_ = std::move(parser_func);
    }
    virtual bool parser_read_string(const std::string& s_read);

    void test_data_socket();

private:
    void safe_request_stop(std::jthread& thread)
    {
        try {
            thread.request_stop();
        }
        catch (...) { } // Игнорируем исключения
    }
    const std::string stop_working_ = "stop working";
    std::atomic<bool> is_working_read_ = true;
    std::atomic<bool> is_working_send_ = true;
    std::atomic<bool> is_add_send_ = true;

    std::function<bool(const std::string&)> parser_external_function_;

    /// Поток для чтения данных.
    std::jthread f_read_thread_;

    /// Поток для отправки данных.
    std::jthread f_send_thread_;

    manual_reset_event wait_handle_ = manual_reset_event(false);

//    moodycamel::ConcurrentQueue<DataTuple> queueSend_;
    moodycamel::ConcurrentQueue<std::string> queue_send_;
    moodycamel::ProducerToken token_;    // токен производителя для оптимизации

//    boost::asio::io_context io_context_;
//    boost::asio::ip::tcp::socket socket(io_context);
//    boost::asio::ip::tcp::resolver resolver(io_context);
//    boost::asio::io_context io_context;
//    boost::asio::ip::tcp::socket socket_; // (io_context);
//    boost::asio::ip::tcp::resolver resolver_; // (io_context);
//    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints_;
    boost::asio::io_context io_context_;          // Контекст
    boost::asio::ip::tcp::socket socket_;         // Сокет
    boost::asio::ip::tcp::resolver resolver_;     // Резолвер
    boost::asio::ip::tcp::endpoint endpoint_;     // Кешированный endpoint
//    boost::asio::ip::tcp::endpoint endpoints_;     // Кешированный endpoint
    boost::asio::deadline_timer timer;
};

/*
class TcpDuplex
{
public:
using namespace moodycamel;
using DataTuple = std::tuple<std::vector<uint8_t>, std::vector<uint8_t>>;

private:
moodycamel::ConcurrentQueue<DataTuple> queueSend_;
 moodycamel::ProducerToken token_; // Оптимизация для одного производителя

};

TcpDuplex::TcpDuplex()
{
    token_ = moodycamel::ProducerToken(queueSend_);
}

void TcpDuplex::send_data(std::string)
{
    std::vector<uint8_t> buffer1{ 0xDE, 0xAD, 0xBE, 0xEF };
    std::vector<uint8_t> buffer2{ 0xCA, 0xFE, 0xBA, 0xBE };
    queueSend_.enqueue(token_, std::make_tuple(std::move(buffer1), std::move(buffer2)));
}

void TcpDuplex:: read_data()
{
    if (queueSend.try_dequeue(token, data))
    {
           auto& [vec1, vec2] = data;
    }
}


    moodycamel::ProducerToken token(queueSend); // Оптимизация для одного производителя

    for (int i = 0; i < 10; ++i) {
        std::vector<uint8_t> buffer1{ 0xDE, 0xAD, 0xBE, 0xEF };
        std::vector<uint8_t> buffer2{ 0xCA, 0xFE, 0xBA, 0xBE };

        // Используем перемещение для избежания копирования
        queueSend.enqueue(token, std::make_tuple(std::move(buffer1), std::move(buffer2)));
        std::cout <<  " "  << i  <<  " " <<"ADD  SIZE =  " << queueSend.size_approx() << "  \n";

//        queueSend.push({ bytesToSend, lengthBytes });
        waitHandle.set();

        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  \n";

}

****
if (queueSend.try_dequeue(token, data)) 
        {
           auto& [vec1, vec2] = data;
        

*/

