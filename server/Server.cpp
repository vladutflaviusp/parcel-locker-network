#include "Server.hpp"
#include <iostream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

Server::Server() : serverSocket(INVALID_SOCKET) {}

Server::~Server() {
    closesocket(serverSocket);
    WSACleanup();
}

bool Server::initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        return false;
    }

    clientSockets.push_back(serverSocket);
    std::cout << "Parcel Locker Network - Clean Architecture Server running on port " << SERVER_PORT << "...\n";
    return true;
}

void Server::sendResponse(SOCKET sock, MessageType type, uint8_t lockerId, StatusCode status) {
    MessageHeader response{};
    response.type = type;
    response.lockerId = lockerId;
    response.status = status;
    response.dataLength = 0;

    send(sock, reinterpret_cast<const char*>(&response), sizeof(MessageHeader), 0);
}

void Server::handleMessage(SOCKET sock, const MessageHeader& header) {
    std::cout << "Received message type: " << static_cast<int>(header.type) 
              << " for locker ID: " << static_cast<int>(header.lockerId) << "\n";

    PackageData pkgData{};
    bool hasData = (header.dataLength == sizeof(PackageData));
    if (hasData) {
        recv(sock, reinterpret_cast<char*>(&pkgData), sizeof(PackageData), 0);
    }

    if (lockerManager.isInvalidId(header.lockerId)) {
        sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::NOT_FOUND);
        return;
    }

    StatusCode status = StatusCode::BAD_REQUEST;

    switch (header.type) {
        case MessageType::CLIENT_LOGIN:
            std::cout << "-> Processing client login...\n";
            status = StatusCode::SUCCESS;
            break;

        case MessageType::DEPOSIT_PACKAGE:
            status = lockerManager.deposit(header.lockerId, pkgData.pin);
            if (status == StatusCode::SUCCESS) {
                std::cout << "-> Package deposited in locker " << static_cast<int>(header.lockerId) << ".\n";
            } else {
                std::cout << "-> Deposit failed for locker " << static_cast<int>(header.lockerId) << " (Status: " << static_cast<int>(status) << ").\n";
            }
            break;

        case MessageType::PICKUP_PACKAGE:
            status = lockerManager.pickup(header.lockerId, pkgData.pin);
            if (status == StatusCode::SUCCESS) {
                std::cout << "-> Package picked up from locker " << static_cast<int>(header.lockerId) << ".\n";
            } else {
                std::cout << "-> Pickup failed for locker " << static_cast<int>(header.lockerId) << " (Status: " << static_cast<int>(status) << ").\n";
            }
            break;

        default:
            std::cout << "-> Unknown message type received.\n";
            break;
    }

    sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, status);
}

void Server::run() {
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);

        SOCKET maxSocket = serverSocket;
        for (SOCKET sock : clientSockets) {
            FD_SET(sock, &readfds);
            if (sock > maxSocket) {
                maxSocket = sock;
            }
        }

        int activity = select(0, &readfds, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR) {
            std::cerr << "Select error: " << WSAGetLastError() << "\n";
            break;
        }

        if (FD_ISSET(serverSocket, &readfds)) {
            sockaddr_in clientAddr{};
            int clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

            if (clientSocket != INVALID_SOCKET) {
                clientSockets.push_back(clientSocket);
                std::cout << "New client connected. Total clients: " << clientSockets.size() - 1 << "\n";
            }
        }

        for (auto it = clientSockets.begin() + 1; it != clientSockets.end();) {
            SOCKET sock = *it;
            if (FD_ISSET(sock, &readfds)) {
                MessageHeader header{};
                int bytesReceived = recv(sock, reinterpret_cast<char*>(&header), sizeof(MessageHeader), 0);

                if (bytesReceived > 0) {
                    handleMessage(sock, header);
                    ++it;
                } else {
                    closesocket(sock);
                    it = clientSockets.erase(it);
                    std::cout << "Client disconnected. Total clients: " << clientSockets.size() - 1 << "\n";
                }
            } else {
                ++it;
            }
        }
    }
}