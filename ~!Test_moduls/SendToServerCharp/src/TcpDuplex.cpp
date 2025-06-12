#include "TcpDuplex.h"

#include <boost/exception/info.hpp>

TcpDuplex::TcpDuplex(const ip_address_one& ip_address)
    : ip_address_one_(ip_address), token_(queue_send_), socket_(io_context_),
    resolver_(io_context_)
//: queue_send_(), token_(queue_send_)  // Инициализация очереди (если нужно)
{
    //    ip_address_one_ = ip_address;
//    socket_ = boost::asio::ip::tcp::socket(io_context_); // (io_context);
//    resolver_ = boost::asio::ip::tcp::resolver(io_context_);
//    endpoints_ = resolver_.resolve(ip_address_one_.ip_address, std::to_string(ip_address_one_.port2));
    // Разрешаем адрес один раз при создании объекта
    endpoint_ = *resolver_.resolve(
        ip_address_one_.ip_address,
        std::to_string(ip_address_one_.port2)
    ).begin();

    // Устанавливаем соединение
    socket_.connect(endpoint_);

}


TcpDuplex::~TcpDuplex()
{
    is_working_read_ = false;
    is_working_send_ = false;
    f_read_thread_.join();
    f_send_thread_.join();
}

void TcpDuplex::run_read(std::stop_token stoken)
{
    return;
/*
    DataTuple data;
    // Попытка извлечь элемент из очереди (без блокировки)
    if (queueSend_.try_dequeue(data))
    {
        auto& [vec1, vec2] = data;
        // Здесь обработайте vec1 и vec2 по необходимости
    }
*/
    while ((!stoken.stop_requested()) && is_working_read_) {
        // Работаем, пока не поступил запрос на остановку
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Working run_read ..." << std::endl;
    }
    std::cout << "!!!!!  END run_read ..." << std::endl;

}

void TcpDuplex::run_send(std::stop_token stoken)
{

    while ((!stoken.stop_requested()) && is_working_send_) 
    {
        // В другом потоке:
        if (wait_handle_.wait_one(500))
        {           //   std::cout << "waitHandle.WaitOne = TRUe   \n";
            if (queue_send_.size_approx() == 0)
            {
                wait_handle_.reset();
                continue;
            }
        }

        std::string data_;
        if (!queue_send_.try_dequeue(data_)) continue;

        is_working_send_ = (data_ != stop_working_);

        try 
        {
            // Отправка данных

//            uint32_t net_length = static_cast<uint32_t>(yamlStr.size());
            // Отправляем длину и сообщение
//            boost::asio::write(socket, boost::asio::buffer(&net_length, 4));
//            boost::asio::write(socket, boost::asio::buffer(yamlStr));

            uint32_t net_length = static_cast<uint32_t>(data_.size());
            boost::asio::write(socket_, boost::asio::buffer(&net_length, 4));
            boost::asio::write(socket_, boost::asio::buffer(data_));

            // Прием ответа
            uint32_t response_length;
            boost::asio::read(socket_, boost::asio::buffer(&response_length, 4));
            response_length = response_length;

            std::vector<char> buf(response_length);
            boost::asio::read(socket_, boost::asio::buffer(buf));

            // Обработка ответа
            std::string response(buf.begin(), buf.end());
            response.erase(std::remove_if(response.begin(), response.end(),
                [](char c) { return (c >= 0x00 && c <= 0x1F) || c == 0x7F; }),
                response.end());
        }
        catch (const std::exception& e) {
            std::cerr << "Send error: " << e.what() << "\n";
            reconnect();  // Метод для восстановления соединения
        }
        
//
// ////////////////////////////////////////////////////////////////////////////////////////////
////      Передача
//
//
//
//
//
//         try
//         {
//
//             //////boost::asio::io_context io_context;
//             //////boost::asio::ip::tcp::socket socket(io_context);
//             //////boost::asio::ip::tcp::resolver resolver(io_context);
//             //////boost::asio::connect(socket, endpoints_);
//
////             boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints = resolver.resolve(
////                 ip_address_one_.ip_address, std::to_string(ip_address_one_.port2));
//////////             auto endpoints = resolver.resolve(
//////////                 ip_address_one_.ip_address, std::to_string(ip_address_one_.port2));
//////////             //             boost::asio::connect(socket_, endpoints_);
//////////////                 YAML::IMSocket01 im_socket01{ "start", 0 };
////////////                 std::string yamlStr = convert_msg.serializeToYaml(im_socket01);
//                 uint32_t net_length = static_cast<uint32_t>(data_.size());
//
//                 // Отправляем длину и сообщение
//                 boost::asio::write(socket, boost::asio::buffer(&net_length, 4));
//                 boost::asio::write(socket, boost::asio::buffer(data_));
//
//                 std::cout << "Sent: " << data_ << "  READ  SIZE =  " << queue_send_.size_approx() << "  \n";
//
//
//                 // Читаем длину ответа
//                 uint32_t responseNetLength = 0;
//                 boost::asio::read(socket, boost::asio::buffer(&responseNetLength, sizeof(responseNetLength)));
//                 uint32_t responseLength = responseNetLength;
//
//                 // Читаем YAML-ответ
//                 std::vector<char> buf(responseLength);
//                boost::asio::read(socket, boost::asio::buffer(buf.data(), responseLength));
//
//                 std::string yamlReceived(buf.begin(), buf.end());
//                 yamlReceived.erase(std::ranges::remove_if(yamlReceived, [](char c)
//                     {  return (c >= 0x00 && c <= 0x1F) || c == 0x7F; }).begin(), yamlReceived.end());
//
//                 if (yamlReceived == "ok")
//                 {
/////                     std::cout << " From Server: " << yamlReceived << std::endl;
//                 }
//                 else
//                 {
//
//                 }
//
//         }
//         catch (std::exception& e)
//         {
//             std::cerr << "Client exception: " << e.what() << std::endl;
//         };
//
//

/////////////////////////////////////////////////////////////////////////////////////////
        // Работаем, пока не поступил запрос на остановку
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//        std::cout << "Working run_send..." << std::endl;
    }


    std::cout << " +++++++ END run_send..." << std::endl;

}

void TcpDuplex::send_data(const std::string& data)
{
    if(!is_add_send_)
        return;

    queue_send_.enqueue(token_, data);

    if (data == stop_working_)
        is_add_send_ = false;

    wait_handle_.set();
//    std::this_thread::sleep_for(std::chrono::milliseconds(20));

}

void TcpDuplex::read_data()
{
    //stop_thread();
}

bool TcpDuplex::parser_read_string(const std::string& s_read)
{
    if (parser_external_function_) {
        // Вызов переданной функции
        return parser_external_function_(s_read);
    }
    else {
        // Обработка случая, когда функция не инициализирована
        std::cerr << "Parser function is not initialized." << std::endl;
        return false; // или любое другое значение по умолчанию
    }
    return false;
}

void TcpDuplex::test_data_socket()
{
    YAML::MSocket01 convert_msg = YAML::MSocket01();
    std::string message_ = "start";
    for (int i=0; i<50; i++)
    {
        std::cout << i << "   PUSH DATA" << std::endl;

        YAML::IMSocket01 im_socket01{ message_, i };
        std::string yamlStr = convert_msg.serializeToYaml(im_socket01);
        send_data(yamlStr);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        message_ += "~" + boost::to_string(i) + "~ ";
    }
    send_data(stop_working_);

}

void TcpDuplex::start_thread()
{
    f_read_thread_ = std::jthread([this](std::stop_token stoken) { this->run_read(stoken); });
    f_send_thread_ = std::jthread([this](std::stop_token stoken) {this->run_send(stoken); });
}

void TcpDuplex::stop_thread()
{
    if (f_read_thread_.joinable()) {
        f_read_thread_.request_stop();
        f_read_thread_.join(); // или дождаться завершения
    }

    if (f_send_thread_.joinable()) {
        f_send_thread_.request_stop();
        f_send_thread_.join();
    }
//    safe_request_stop(f_read_thread_);
//    safe_request_stop(f_send_thread_);

}

void TcpDuplex::stop_working()
{
}
void TcpDuplex::reconnect()
{
    socket_.close();
    socket_.connect(endpoint_);
}


/*
 




 
 /////////////////////////////////////////////////////////////////////
 TcpDuplex::TcpDuplex()
    : socket_(io_context_),
      resolver_(io_context_)
{
    // Разрешаем адрес один раз при создании объекта
    endpoint_ = *resolver_.resolve(
        ip_address_one_.ip_address,
        std::to_string(ip_address_one_.port2)
    ).begin();

    // Устанавливаем соединение
    socket_.connect(endpoint_);
}



class TcpDuplex {
private:
    boost::asio::io_context io_context_;          // Контекст
    boost::asio::ip::tcp::socket socket_;         // Сокет
    boost::asio::ip::tcp::resolver resolver_;     // Резолвер
    boost::asio::ip::tcp::endpoint endpoint_;     // Кешированный endpoint
};

class TcpDuplex {
private:
    boost::asio::io_context io_context_;          // Контекст
    boost::asio::ip::tcp::socket socket_;         // Сокет
    boost::asio::ip::tcp::resolver resolver_;     // Резолвер
    boost::asio::ip::tcp::endpoint endpoint_;     // Кешированный endpoint
};

TcpDuplex::TcpDuplex()
    : socket_(io_context_),
      resolver_(io_context_)
{
    // Разрешаем адрес один раз при создании объекта
    endpoint_ = *resolver_.resolve(
        ip_address_one_.ip_address,
        std::to_string(ip_address_one_.port2)
    ).begin();

    // Устанавливаем соединение
    socket_.connect(endpoint_);
}

void TcpDuplex::run_send(std::stop_token stoken)
{
    while (!stoken.stop_requested() && is_working_send_)
    {
        if (queue_send_.empty()) {
            std::this_thread::sleep_for(500ms);
            continue;
        }

        std::string data_;
        if (!queue_send_.try_dequeue(data_)) continue;

        is_working_send_ = (data_ != stop_working_);

        try {
            // Отправка данных
            uint32_t net_length = htonl(data_.size());
            boost::asio::write(socket_, boost::asio::buffer(&net_length, 4));
            boost::asio::write(socket_, boost::asio::buffer(data_));

            // Прием ответа
            uint32_t response_length;
            boost::asio::read(socket_, boost::asio::buffer(&response_length, 4));
            response_length = ntohl(response_length);

            std::vector<char> buf(response_length);
            boost::asio::read(socket_, boost::asio::buffer(buf));

            // Обработка ответа
            std::string response(buf.begin(), buf.end());
            response.erase(std::remove_if(response.begin(), response.end(),
                [](char c){ return (c >= 0x00 && c <= 0x1F) || c == 0x7F; }),
                response.end());
        }
        catch (const std::exception& e) {
            std::cerr << "Send error: " << e.what() << "\n";
            reconnect();  // Метод для восстановления соединения
        }
    }
}

void TcpDuplex::reconnect()
{
    socket_.close();
    socket_.connect(endpoint_);
}

Ключевые улучшения
Единая инициализация:

Контекст, сокет и резолвер создаются один раз

Endpoint кешируется при старте

Сетевая оптимизация:

Используется постоянное соединение

Убраны повторные DNS-запросы

Добавлен механизм восстановления соединения

Упрощение логики:

Убран ненужный wait_handle

Используется стандартный sleep для ожидания

Упрощена обработка бинарных данных

Безопасность:

Корректное преобразование endianness через htonl/ntohl

Централизованная обработка ошибок

Производительность увеличится в 2-3 раза за счет исключения повторной инициализации сетевых ресурсов на каждой итерации.

Related
Как вынести повторяющиеся блоки и создание сокета в конструктор класса
Какие параметры или объекты можно передать через конструктор для настройки соединения
Можно ли объединить создание resolver, socket и io_context в один метод или класс-обертку
Какие переменные лучше сделать членами класса для избежания их повторного создания при каждом вызове run_send
Как обеспечить правильную инициализацию всех необходимых компонентов через конструктор


 /////////////////////////////////////////////////////////////////////
 */