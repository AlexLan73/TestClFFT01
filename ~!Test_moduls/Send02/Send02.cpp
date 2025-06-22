#include <iostream>
#include <boost/endian/conversion.hpp>
#include <boost/asio.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;



uint32_t hostToNetwork32(uint32_t host) {
    return ((host & 0xFF000000) >> 24) |
        ((host & 0x00FF0000) >> 8) |
        ((host & 0x0000FF00) << 8) |
        ((host & 0x000000FF) << 24);
}

void SendToDesktop(int port, const std::string& data) {
    try {
        asio::io_context io;
        tcp::socket socket(io);
        socket.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), port));

        // 1. Handshake (4 нулевых байта)
        uint32_t handshake = 0;
        asio::write(socket, asio::buffer(&handshake, sizeof(handshake)));

        // 2. Подготовка длины
        uint32_t length = hostToNetwork32(static_cast<uint32_t>(data.size()));
        std::cout << "Sending length (network order): " << length
            << " (0x" << std::hex << length << std::dec << ")\n";

        // 3. Отправка длины + данных
        std::vector<asio::const_buffer> buffers;
        buffers.push_back(asio::buffer(&length, sizeof(length)));
        buffers.push_back(asio::buffer(data));
        asio::write(socket, buffers);

        // 4. Получение ответа
        uint32_t response_len;
        asio::read(socket, asio::buffer(&response_len, sizeof(response_len)));
        response_len = hostToNetwork32(response_len);

        std::vector<char> buf(response_len);
        asio::read(socket, asio::buffer(buf));
        std::cout << "Server response: " << std::string(buf.begin(), buf.end()) << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SendToDesktop(20000, "Test message for port 20000");

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}