// SendToServerCharp.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _WIN32_WINNT 0x0A00  // для windows 11

#include <iostream>
#include <memory>
#include <algorithm> // для std::swap
#include <ranges>

#include <boost/asio.hpp>
#include <yaml-cpp/yaml.h>

#include "concurrentqueue.h"
#include <thread>
#include <vector>
#include <tuple>


#include <string>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "src/manual_reset_event.h"
#include "src/ReadWriteYaml.h"
#include "src/IpAddressOne.h"
#include "src/MSocket01.h"
#include "src/TcpDuplex.h"


using namespace moodycamel;
using DataTuple = std::tuple<std::vector<uint8_t>, std::vector<uint8_t>>;
moodycamel::ConcurrentQueue<DataTuple> queueSend;


void print_map_ip_address_one(ip_address_one ip)
{
    // Вывод прочитанных данных
        std::cout << "Id: " << ip.id << ", Name: " << ip.name << ", IP: " << ip.ip_address
            << ", Port1: " << ip.port1 << ", Port2: " << ip.port2 << "\n";
}
void print_map_ip_address(std::map<int, ip_address_one> *data)
{
    // Вывод прочитанных данных
    for (const auto& ip : *data | std::views::values) 
        print_map_ip_address_one(ip);
}

manual_reset_event waitHandle(false);
std::stop_token stoken;

// Поток-производитель (отправка данных)
void producer()
{
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

// Поток-потребитель (обработка данных)
void consumer()
{
    moodycamel::ConsumerToken token(queueSend); // Оптимизация для одного потребителя

    while (!stoken.stop_requested()) 
    {

        // В другом потоке:
        if (waitHandle.wait_one(500))
        {
            std::cout << "waitHandle.WaitOne = TRUe   \n";

            if (queueSend.size_approx() == 0)
            {
                waitHandle.reset();
                continue;
            }
        }
        else
        {
            std::cout << "waitHandle.WaitOne = FALSE   \n";

        }
        DataTuple data;

        if (queueSend.try_dequeue(token, data)) 
        {
           auto& [vec1, vec2] = data;
           std::cout << "READ  SIZE =  " << queueSend.size_approx() << "  \n";


//                // Пример обработки данных
//                if (!vec1.empty()) {
//                    std::cout << "First byte of vector1: 0x"
//                        << std::hex << static_cast<int>(vec1[0]) << "\n";
//                }
        }


//            // Таймаут, событие не произошло
//            if (queueSend.size_approx() == 0)
//            {
////                std::cout << "!!!  SIZE = 0   \n";
//
//            }
//            else
//                std::cout << "!!!  REPEAT = 0   \n";

//        }
//        else 
//        {
///*
//            DataTuple data;
//            if (queueSend.try_dequeue(data)) {
//                // data содержит кортеж (buffer1, buffer2)
//                auto& [buffer1, buffer2] = data;
//                // Обработка данных...
//            }
//*/
//        }


        //if (queueSend.try_dequeue(token, data)) {
        //    auto& [vec1, vec2] = data;

        //    std::cout << "Received vectors: "
        //        << "Size1=" << vec1.size()
        //        << " Size2=" << vec2.size() << "\n";

        //    // Пример обработки данных
        //    if (!vec1.empty()) {
        //        std::cout << "First byte of vector1: 0x"
        //            << std::hex << static_cast<int>(vec1[0]) << "\n";
        //    }
        //}
        //else 
        //if (queueSend.size_approx() == 0) {
        //    break;
        //}
    }
    std::cout << "!=!=!= = =S=I=Z=E= === =0= =  \n";

}



int main()
{
//    stoken = std::stop_token();


/*
    std::jthread producerThread(producer);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::jthread consumerThread(consumer);

    producerThread.join();
//    auto _c = queueSend.size_approx();
//    auto _dd = 1;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    stoken.stop_requested();
    consumerThread.join();
    */





    std::cout << "Test SOCKET ! \n";
    std::string _pathYaml = R"(E:\C#\OpenCLDeskTop\Core\DeskTop\ipAddresses.yaml)";
    std::map<int, ip_address_one> data_file_yaml;
    try 
    {
        ReadWriteYaml rw(_pathYaml);
        // Чтение
        data_file_yaml = rw.ReadYaml();
//        print_map_ip_address(&data_file_yaml);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << '\n';
    }

    ip_address_one _one_ip_address = data_file_yaml[0];
    std::swap(_one_ip_address.port1, _one_ip_address.port2);
    data_file_yaml[0] = _one_ip_address;

    std::shared_ptr<TcpDuplex> tcp_duplex = std::make_shared<TcpDuplex>(data_file_yaml[0]);
    tcp_duplex->start_thread();
    tcp_duplex->test_data_socket();

    std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    tcp_duplex->~TcpDuplex();


    //_one_ip_address = data_file_yaml[0];
    //std::swap(_one_ip_address.port1, _one_ip_address.port2);
    //print_map_ip_address_one(_one_ip_address);

// Send to Server Charp

    try
    {

        boost::asio::io_context io_context;
        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(_one_ip_address.ip_address, std::to_string(_one_ip_address.port2));
        boost::asio::connect(socket, endpoints);

        YAML::MSocket01 convert_msg = YAML::MSocket01();

        for (int i = 0; i < 10; ++i)
        {
            YAML::IMSocket01 im_socket01{ "start", 0 };
            std::string yamlStr = convert_msg.serializeToYaml(im_socket01);
            uint32_t net_length = static_cast<uint32_t>(yamlStr.size());

            // Отправляем длину и сообщение
            boost::asio::write(socket, boost::asio::buffer(&net_length, 4));
            boost::asio::write(socket, boost::asio::buffer(yamlStr));

            std::cout << "Sent: Text='" << im_socket01.Text << "', Number=" << im_socket01.Number << std::endl;

            // Читаем длину ответа
            uint32_t responseNetLength = 0;
            boost::asio::read(socket, boost::asio::buffer(&responseNetLength, sizeof(responseNetLength)));
            uint32_t responseLength = responseNetLength;

            // Читаем YAML-ответ
            std::vector<char> buf(responseLength);
            boost::asio::read(socket, boost::asio::buffer(buf.data(), responseLength));

             std::string yamlReceived(buf.begin(), buf.end());
             yamlReceived.erase( std::ranges::remove_if(yamlReceived, [](char c) 
                 {  return (c >= 0x00 && c <= 0x1F) || c == 0x7F; }).begin(), yamlReceived.end());

            if(yamlReceived=="ok")
            {
                std::cout << " From Server: " << yamlReceived << std::endl;
            }else
            {
                
            }
            
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Client exception: " << e.what() << std::endl;
    }


    return 0;
}

