//#include <iostream>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main() {
    try {
//        boost::asio::io_service io_service;
        boost::asio::io_context io_service;

        boost::asio::ip::make_address("127.0.0.1");
        // Создаем endpoint на порту 1000
        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 20000));

        std::cout << "Server is running on port 10000...\n";

        for (;;) {
            tcp::socket socket(io_service);

            // Ожидаем подключения клиента
            acceptor.accept(socket);
            std::cout << "Client connected: " << socket.remote_endpoint() << std::endl;

            // Буфер для получения данных
            char data[1024];

            // Читаем данные от клиента (синхронно)
            boost::system::error_code error;
            size_t length = socket.read_some(boost::asio::buffer(data), error);

            if (!error) {
                std::cout << "Received: " << std::string(data, length) << std::endl;

                // Отправляем ответ клиенту
                std::string response = "Server received your message\n";
                boost::asio::write(socket, boost::asio::buffer(response), error);
            }
            else {
                std::cerr << "Error on receive: " << error.message() << std::endl;
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
