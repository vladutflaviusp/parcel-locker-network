#include <iostream>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../include/protocol.hpp"

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "WSAStartup failed: " << wsaResult << "\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Parcel Locker Network - Multiplexed Server is running on port " << SERVER_PORT << "...\n";

    std::vector<SOCKET> clientSockets;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);

        FD_SET(serverSocket, &readfds);
        SOCKET maxSocket = serverSocket;

        for (SOCKET sock : clientSockets) {
            FD_SET(sock, &readfds);
            if (sock > maxSocket) {
                maxSocket = sock;
            }
        }

        int activity = select(0, &readfds, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR) {
            std::cerr << "Select failed: " << WSAGetLastError() << "\n";
            break;
        }

        if (FD_ISSET(serverSocket, &readfds)) {
            sockaddr_in clientAddr{};
            int clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
            
            if (clientSocket != INVALID_SOCKET) {
                clientSockets.push_back(clientSocket);
                std::cout << "New client connected. Total clients: " << clientSockets.size() << "\n";
            }
        }

        for (auto it = clientSockets.begin(); it != clientSockets.end(); ) {
            SOCKET sock = *it;
            if (FD_ISSET(sock, &readfds)) {
                char buffer[512];
                int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);

                if (bytesReceived <= 0) {

                    closesocket(sock);
                    it = clientSockets.erase(it);
                    std::cout << "Client disconnected. Total clients: " << clientSockets.size() << "\n";
                } else {
                    std::cout << "Received " << bytesReceived << " bytes from client.\n";
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    closesocket(serverSocket);
    for (SOCKET sock : clientSockets) {
        closesocket(sock);
    }
    WSACleanup();

    return 0;
}