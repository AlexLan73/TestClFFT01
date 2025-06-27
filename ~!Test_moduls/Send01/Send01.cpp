// Send01.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(20000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(sock, (sockaddr*)&addr, sizeof(addr));
    std::cout << "Подключено! Отправляю сообщение...\n";

    const char* msg = "TEST от C++";
    send(sock, msg, strlen(msg), 0);
    std::cout << "Отправлено: " << msg << "\n";

    char buffer[1024];
    int len = recv(sock, buffer, sizeof(buffer), 0);
    buffer[len] = '\0';
    std::cout << "Ответ сервера: " << buffer << "\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}