#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;



        // Подключаемся к серверу localhost:12345
        tcp::resolver resolver(io_context);
//        tcp::resolver::query query("127.0.0.1", "20000");
//        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        auto endpoints = resolver.resolve("127.0.0.1", "20001");

        // Для подключения можно использовать asio::connect с этим range

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::string message = "Hi SERVER!";

        boost::system::error_code error;

        // Отправляем данные (аналог send)
        boost::asio::write(socket, boost::asio::buffer(message), error);

        if (!error) {
            std::cout << "Message sent: " << message << std::endl;

            // Получаем ответ от сервера
            std::vector<char> reply(1024);
            size_t reply_length = socket.read_some(boost::asio::buffer(reply), error);

            if (!error) {
                std::cout << "Reply from server: " << std::string(reply.data(), reply_length) << std::endl;
            }
            else {
                std::cerr << "Error during receive: " << error.message() << std::endl;
            }
        }
        else {
            std::cerr << "Error during send: " << error.message() << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}