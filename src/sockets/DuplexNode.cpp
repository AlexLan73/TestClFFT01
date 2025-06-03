#include "DuplexNode.h"

DuplexNode::DuplexNode(boost::asio::io_context& io, unsigned short listen_port, unsigned short connect_port)
    : io_(io),
    acceptor_(io, tcp::endpoint(tcp::v4(), listen_port)),
    socket_server_(io),
    socket_client_(io),
    connect_port_(connect_port)
{
}

void DuplexNode::start()
{
    start_accept();
    start_connect();
}

void DuplexNode::start_accept()
{
    std::cout << "Сервер слушает порт " << acceptor_.local_endpoint().port() << std::endl;
    acceptor_.async_accept(socket_server_, [this](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Подключение принято на сервере" << std::endl;
            std::thread(&DuplexNode::read_from_socket, this, std::ref(socket_server_)).detach();
        }
        else {
            std::cerr << "Ошибка accept: " << ec.message() << std::endl;
        }
        start_accept(); // слушать дальше
        });
}
void DuplexNode::start_connect()
{
    auto address = boost::asio::ip::make_address("127.0.0.1");
    tcp::endpoint endpoint(address, connect_port_);

    std::cout << "Клиент пытается подключиться к порту " << connect_port_ << std::endl;
    socket_client_.async_connect(endpoint, [this](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "Клиент подключен" << std::endl;
            std::thread(&DuplexNode::write_to_socket, this, std::ref(socket_client_)).detach();
            std::thread(&DuplexNode::read_from_socket, this, std::ref(socket_client_)).detach();
        }
        else {
            std::cerr << "Ошибка connect: " << ec.message() << std::endl;
            // Попробуем переподключиться через 1 секунду
            std::this_thread::sleep_for(std::chrono::seconds(1));
            start_connect();
        }
        });
}

void DuplexNode::read_from_socket(tcp::socket& socket) {
    try {
        char data[1024];
        while (true) {
            boost::system::error_code ec;
            size_t len = socket.read_some(buffer(data), ec);
            if (ec == boost::asio::error::eof) {
                std::cout << "Соединение закрыто" << std::endl;
                break;
            }
            else if (ec) {
                std::cerr << "Ошибка чтения: " << ec.message() << std::endl;
                break;
            }
            std::cout << "Принято: " << std::string(data, len) << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Исключение в чтении: " << e.what() << std::endl;
    }
}

void DuplexNode::write_to_socket(tcp::socket& socket) {
    try {
        int counter = 0;
        while (true) {
            std::string message = "Сообщение " + std::to_string(counter++);
            boost::system::error_code ec;
            write(socket, buffer(message), ec);
            if (ec) {
                std::cerr << "Ошибка записи: " << ec.message() << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    catch (std::exception& e) {
        std::cerr << "Исключение в записи: " << e.what() << std::endl;
    }
}

