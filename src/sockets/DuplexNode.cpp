#include "DuplexNode.h"

//DuplexNode::DuplexNode(boost::asio::io_context& io, unsigned short listen_port, unsigned short connect_port)
//    : io_(io),
//    acceptor_(io, tcp::endpoint(tcp::v4(), listen_port)),
//    socket_server_(io),
//    socket_client_(io),
//    connect_port_(connect_port){}

DuplexNode::DuplexNode(io_context& io_context, unsigned short listen_port, unsigned short connect_port)
    : io_context_(io_context),
    acceptor_(io_context, tcp::endpoint(tcp::v4(), listen_port)),
    socket_(io_context),
    connect_port_(connect_port),
    resolver_(io_context)
{
}


void DuplexNode::start()
{
//    start_accept();
//    start_connect();
    do_accept();
    do_connect();

}

void DuplexNode::do_accept() {
    acceptor_.async_accept([self = shared_from_this()](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::cout << "Accepted connection from " << socket.remote_endpoint() << std::endl;
            self->start_read(std::move(socket));
        }
        else {
            std::cerr << "Accept error: " << ec.message() << std::endl;
        }
        // Продолжаем слушать
        self->do_accept();
        });
}

void DuplexNode::do_connect() {
    auto endpoints = resolver_.resolve("127.0.0.1", std::to_string(connect_port_));
    async_connect(socket_, endpoints,
        [self = shared_from_this()](boost::system::error_code ec, tcp::endpoint) {
            if (!ec) {
                std::cout << "Connected to peer on port " << self->connect_port_ << std::endl;
//                self->start_read(self->socket_);
                self->start_read(std::move(self->socket_));
                self->start_write();
            }
            else {
                std::cerr << "Connect error: " << ec.message() << ". Retrying in 1 second..." << std::endl;
                // Попытка переподключения через 1 секунду
                self->retry_timer_ = std::make_unique<steady_timer>(self->io_context_, std::chrono::seconds(1));
                self->retry_timer_->async_wait([self](auto) { self->do_connect(); });
            }
        });
}

////////////////////////
/*
void  DuplexNode::start_read(tcp::socket socket) {
    auto self = shared_from_this();
    async_read_until(socket, dynamic_buffer(read_msg_), '\n',
        [self, &socket](boost::system::error_code ec, std::size_t length) mutable {
            if (!ec) {
                std::string msg(self->read_msg_.substr(0, length - 1)); // без '\n'
                self->read_msg_.erase(0, length);
                std::cout << "Received: " << msg << std::endl;

                // Продолжаем читать
                self->start_read(socket);
            }
            else {
                std::cerr << "Read error: " << ec.message() << std::endl;
                socket.close();
            }
        });
}
*/
///////////////////////////////////

void DuplexNode::start_read(tcp::socket socket) {
    auto self = shared_from_this();
    async_read_until(socket, dynamic_buffer(read_msg_), '\n',
        [self, socket = std::move(socket)](boost::system::error_code ec, std::size_t length) mutable {
            if (!ec) {
                std::string msg(self->read_msg_.substr(0, length - 1));
                self->read_msg_.erase(0, length);
                std::cout << "Received: " << msg << std::endl;

                self->start_read(std::move(socket));
            }
            else {
                std::cerr << "Read error: " << ec.message() << std::endl;
                socket.close();
            }
        });
}

void  DuplexNode::start_write() {
    auto self = shared_from_this();
    std::thread([self]() {
        int counter = 0;
        while (true) {
            std::string msg = "Message " + std::to_string(counter++) + "\n";
            post(self->io_context_, [self, msg]() {
                bool write_in_progress = !self->write_msgs_.empty();
                self->write_msgs_.push_back(msg);
                if (!write_in_progress) {
                    self->do_write();
                }
                });
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        }).detach();
}

void DuplexNode::do_write() {
    auto self = shared_from_this();
    async_write(socket_, buffer(write_msgs_.front()),
        [self](boost::system::error_code ec, std::size_t) { //length
            if (!ec) {
                self->write_msgs_.pop_front();
                if (!self->write_msgs_.empty()) {
                    self->do_write();
                }
            }
            else {
                std::cerr << "Write error: " << ec.message() << std::endl;
                self->socket_.close();
            }
        });
}




//
//void DuplexNode::start_accept()
//{
//    std::cout << "Сервер слушает порт " << acceptor_.local_endpoint().port() << std::endl;
//    acceptor_.async_accept(socket_server_, [this](boost::system::error_code ec) {
//        if (!ec) {
//            std::cout << "Подключение принято на сервере" << std::endl;
//            std::thread(&DuplexNode::read_from_socket, this, std::ref(socket_server_)).detach();
//        }
//        else {
//            std::cerr << "Ошибка accept: " << ec.message() << std::endl;
//        }
//        start_accept(); // слушать дальше
//        });
//}
//void DuplexNode::start_connect()
//{
//    auto address = boost::asio::ip::make_address("127.0.0.1");
//    tcp::endpoint endpoint(address, connect_port_);
//
//    std::cout << "Клиент пытается подключиться к порту " << connect_port_ << std::endl;
//    socket_client_.async_connect(endpoint, [this](boost::system::error_code ec) {
//        if (!ec) {
//            std::cout << "Клиент подключен" << std::endl;
//            std::thread(&DuplexNode::write_to_socket, this, std::ref(socket_client_)).detach();
//            std::thread(&DuplexNode::read_from_socket, this, std::ref(socket_client_)).detach();
//        }
//        else {
//            std::cerr << "Ошибка connect: " << ec.message() << std::endl;
//            // Попробуем переподключиться через 1 секунду
//            std::this_thread::sleep_for(std::chrono::seconds(1));
//            start_connect();
//        }
//        });
//}
//
//void DuplexNode::read_from_socket(tcp::socket& socket) {
//    try {
//        char data[1024];
//        while (true) {
//            boost::system::error_code ec;
//            size_t len = socket.read_some(buffer(data), ec);
//            if (ec == boost::asio::error::eof) {
//                std::cout << "Соединение закрыто" << std::endl;
//                break;
//            }
//            else if (ec) {
//                std::cerr << "Ошибка чтения: " << ec.message() << std::endl;
//                break;
//            }
//            std::cout << "Принято: " << std::string(data, len) << std::endl;
//        }
//    }
//    catch (std::exception& e) {
//        std::cerr << "Исключение в чтении: " << e.what() << std::endl;
//    }
//}
//
//void DuplexNode::write_to_socket(tcp::socket& socket) {
//    try {
//        int counter = 0;
//        while (true) {
//            std::string message = "Сообщение " + std::to_string(counter++);
//            boost::system::error_code ec;
//            write(socket, buffer(message), ec);
//            if (ec) {
//                std::cerr << "Ошибка записи: " << ec.message() << std::endl;
//                break;
//            }
//            std::this_thread::sleep_for(std::chrono::seconds(2));
//        }
//    }
//    catch (std::exception& e) {
//        std::cerr << "Исключение в записи: " << e.what() << std::endl;
//    }
//}
//

/*
*****************************************************************************************
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <deque>
#include <memory>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

class Node : public std::enable_shared_from_this<Node> {
public:
    Node(asio::io_context& io_context,
         unsigned short listen_port,
         unsigned short connect_port)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), listen_port)),
          socket_(io_context),
          connect_port_(connect_port),
          resolver_(io_context)
    {
    }

    void start() {
        do_accept();
        do_connect();
    }

private:
    void do_accept() {
        acceptor_.async_accept([self = shared_from_this()](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "Accepted connection from " << socket.remote_endpoint() << std::endl;
                self->start_read(std::move(socket));
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            // Продолжаем слушать
            self->do_accept();
        });
    }

    void do_connect() {
        auto endpoints = resolver_.resolve("127.0.0.1", std::to_string(connect_port_));
        asio::async_connect(socket_, endpoints,
            [self = shared_from_this()](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    std::cout << "Connected to peer on port " << self->connect_port_ << std::endl;
                    self->start_read(self->socket_);
                    self->start_write();
                } else {
                    std::cerr << "Connect error: " << ec.message() << ". Retrying in 1 second..." << std::endl;
                    // Попытка переподключения через 1 секунду
                    self->retry_timer_ = std::make_unique<asio::steady_timer>(self->io_context_, std::chrono::seconds(1));
                    self->retry_timer_->async_wait([self](auto) { self->do_connect(); });
                }
            });
    }

    void start_read(tcp::socket& socket) {
        auto self = shared_from_this();
        asio::async_read_until(socket, asio::dynamic_buffer(read_msg_), '\n',
            [self, &socket](boost::system::error_code ec, std::size_t length) mutable {
                if (!ec) {
                    std::string msg(self->read_msg_.substr(0, length - 1)); // без '\n'
                    self->read_msg_.erase(0, length);
                    std::cout << "Received: " << msg << std::endl;

                    // Продолжаем читать
                    self->start_read(socket);
                } else {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    socket.close();
                }
            });
    }

    void start_write() {
        auto self = shared_from_this();
        std::thread([self]() {
            int counter = 0;
            while (true) {
                std::string msg = "Message " + std::to_string(counter++) + "\n";
                asio::post(self->io_context_, [self, msg]() {
                    bool write_in_progress = !self->write_msgs_.empty();
                    self->write_msgs_.push_back(msg);
                    if (!write_in_progress) {
                        self->do_write();
                    }
                });
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }).detach();
    }

    void do_write() {
        auto self = shared_from_this();
        asio::async_write(socket_, asio::buffer(write_msgs_.front()),
            [self](boost::system::error_code ec, std::size_t ) { //length
                if (!ec) {
                    self->write_msgs_.pop_front();
                    if (!self->write_msgs_.empty()) {
                        self->do_write();
                    }
                }
                else {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                    self->socket_.close();
                }
            });
    }

private:
    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_; // для исходящего подключения
    unsigned short connect_port_;
    tcp::resolver resolver_;

    std::string read_msg_;
    std::deque<std::string> write_msgs_;

    std::unique_ptr<asio::steady_timer> retry_timer_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: node <listen_port> <connect_port>\n";
        return 1;
    }

    try {
        asio::io_context io_context;

        auto node = std::make_shared<Node>(io_context,
            static_cast<unsigned short>(std::stoi(argv[1])),
            static_cast<unsigned short>(std::stoi(argv[2])));

        node->start();

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}


*/
