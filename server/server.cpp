#include <iostream>
#include <vector>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../include/protocol.hpp"

#pragma comment(lib, "ws2_32.lib")

struct LockerServerState {
    bool isOccupied = false;
    char pin[5] = {0};
};

void sendResponse(SOCKET sock, MessageType type, uint8_t lockerId, StatusCode status) {
    MessageHeader response{};
    response.type = type;
    response.lockerId = lockerId;
    response.status = status;
    response.dataLength = 0;

    send(sock, reinterpret_cast<const char*>(&response), sizeof(MessageHeader), 0);
}

void handleMessage(SOCKET sock, const MessageHeader& header, std::vector<LockerServerState>& lockers) {
    std::cout << "Received message type: " << static_cast<int>(header.type) 
              << " for locker ID: " << static_cast<int>(header.lockerId) << "\n";

    PackageData pkgData{};
    bool hasData = (header.dataLength == sizeof(PackageData));
    if (hasData) {
        recv(sock, reinterpret_cast<char*>(&pkgData), sizeof(PackageData), 0);
    }

    if (header.lockerId >= MAX_LOCKERS) {
        sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::NOT_FOUND);
        return;
    }

    switch (header.type) {
        case MessageType::CLIENT_LOGIN: {
            std::cout << "-> Processing client login...\n";
            sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::SUCCESS);
            break;
        }

        case MessageType::DEPOSIT_PACKAGE: {
            if (lockers[header.lockerId].isOccupied) {
                std::cout << "-> Locker " << static_cast<int>(header.lockerId) << " is already full.\n";
                sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::BOX_FULL);
            } else {
                lockers[header.lockerId].isOccupied = true;
                std::memcpy(lockers[header.lockerId].pin, pkgData.pin, 5);
                std::cout << "-> Package deposited in locker " << static_cast<int>(header.lockerId) << " with PIN.\n";
                sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::SUCCESS);
            }
            break;
        }

        case MessageType::PICKUP_PACKAGE: {
            if (!lockers[header.lockerId].isOccupied) {
                std::cout << "-> Locker " << static_cast<int>(header.lockerId) << " is already empty.\n";
                sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::NOT_FOUND);
            } else if (std::memcmp(lockers[header.lockerId].pin, pkgData.pin, 5) != 0) {
                std::cout << "-> Incorrect PIN for locker " << static_cast<int>(header.lockerId) << ".\n";
                sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::UNAUTHORIZED);
            } else {
                lockers[header.lockerId].isOccupied = false;
                std::memset(lockers[header.lockerId].pin, 0, 5);
                std::cout << "-> Package picked up from locker " << static_cast<int>(header.lockerId) << ".\n";
                sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::SUCCESS);
            }
            break;
        }

        default:
            std::cout << "-> Unknown message type received.\n";
            sendResponse(sock, MessageType::SERVER_RESPONSE, header.lockerId, StatusCode::BAD_REQUEST);
            break;
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Parcel Locker Network - Multiplexed Server is running on port " << SERVER_PORT << "...\n";

    std::vector<LockerServerState> lockers(MAX_LOCKERS);
    std::vector<SOCKET> clientSockets;
    clientSockets.push_back(serverSocket);

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
                    handleMessage(sock, header, lockers);
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

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}