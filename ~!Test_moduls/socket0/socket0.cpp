#define _WIN32_WINNT 0x0A00  // для windows 11
#pragma warning(disable : C26439)

#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <deque>
#include <memory>
#include <string>

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
            }
            else {
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
                    self->start_read(std::move(self->socket_));
                    self->start_write();
                }
                else {
                    std::cerr << "Connect error: " << ec.message() << ". Retrying in 1 second..." << std::endl;
                    // Попытка переподключения через 1 секунду
                    self->retry_timer_ = std::make_unique<asio::steady_timer>(self->io_context_, std::chrono::seconds(1));
                    self->retry_timer_->async_wait([self](auto) { self->do_connect(); });
                }
            });
    }

    void start_read(tcp::socket socket) {
        auto self = shared_from_this();
        asio::async_read_until(socket, asio::dynamic_buffer(read_msg_), '\n',
            [self, &socket](boost::system::error_code ec, std::size_t length) mutable {
                if (!ec) {
                    std::string msg(self->read_msg_.substr(0, length - 1)); // без '\n'
                    self->read_msg_.erase(0, length);
                    std::cout << "Received: " << msg << std::endl;

                    // Продолжаем читать
                    self->start_read(std::move(self->socket_));
                }
                else {
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

        SetConsoleCP(1251);
        SetConsoleOutputCP(1251);

    setlocale(LC_ALL, "Russian");  // NOLINT(cert-err33-c, concurrency-mt-unsafe)
    std::cout << " ПРИВЕТ !!!" << std::endl;

    //if (argc != 3) {
    //    std::cerr << "Usage: node <listen_port> <connect_port>\n";
    //    return 1;
    //}

    try {
        asio::io_context io_context;

        auto node = std::make_shared<Node>(io_context,
//            static_cast<unsigned short>(std::stoi(argv[1])),
//            static_cast<unsigned short>(std::stoi(argv[2])));
        static_cast<unsigned short>(10000),
            static_cast<unsigned short>(10001));

        node->start();

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
