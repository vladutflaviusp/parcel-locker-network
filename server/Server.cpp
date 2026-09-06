#include "Server.hpp"
#include "protocol.hpp"
#include <iostream>
#include <algorithm>

Server::Server() : serverSocket(INVALID_SOCKET) {}

Server::~Server() {
    for (SOCKET sock : clientSockets) {
        closesocket(sock);
    }
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }
    WSACleanup();
}

bool Server::initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("WSAStartup failed.");
        return false;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        LOG_ERROR("Socket creation failed.");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        LOG_ERROR("Bind failed.");
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        LOG_ERROR("Listen failed.");
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    LOG_INFO("Parcel Locker Network - Clean Architecture Server running on port " + std::to_string(PORT) + "...");
    return true;
}

void Server::run() {
    fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        SOCKET maxFd = serverSocket;
        for (SOCKET sock : clientSockets) {
            FD_SET(sock, &readfds);
            if (sock > maxFd) {
                maxFd = sock;
            }
        }

        int activity = select(maxFd + 1, &readfds, NULL, NULL, NULL);
        if (activity == SOCKET_ERROR) {
            LOG_ERROR("Select error.");
            break;
        }

        if (FD_ISSET(serverSocket, &readfds)) {
            sockaddr_in clientAddr;
            int clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
            
            if (clientSocket != INVALID_SOCKET) {
                clientSockets.push_back(clientSocket);
                LOG_INFO("New client connected. Total clients: " + std::to_string(clientSockets.size()));
            }
        }

        for (auto it = clientSockets.begin(); it != clientSockets.end();) {
            SOCKET sock = *it;
            if (FD_ISSET(sock, &readfds)) {
                if (!handleClientMessage(sock)) {
                    closesocket(sock);
                    it = clientSockets.erase(it);
                    LOG_INFO("Client disconnected. Total clients: " + std::to_string(clientSockets.size()));
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }
}

bool Server::handleClientMessage(SOCKET clientSocket) {
    ClientMessage clientMsg;
    int bytesReceived = recv(clientSocket, (char*)&clientMsg, sizeof(clientMsg), 0);
    
    if (bytesReceived <= 0) {
        return false;
    }

    LOG_INFO("Received message type: " + std::to_string(clientMsg.type) + " for locker ID: " + std::to_string(clientMsg.lockerId));

    ServerResponse response;
    response.isOccupied = 0;

    // Validăm ID-ul primit
    if (clientMsg.lockerId >= MAX_LOCKERS) {
        response.status = static_cast<uint16_t>(StatusCode::NOT_FOUND);
        LOG_WARN("-> Invalid locker ID " + std::to_string(clientMsg.lockerId) + " (Out of bounds).");
    }
    else if (clientMsg.type == 1) {
        // Status Check - preluat corect din LockerManager
        response.status = static_cast<uint16_t>(StatusCode::SUCCESS);
        response.isOccupied = lockerManager.isOccupied(clientMsg.lockerId) ? 1 : 0;
    } 
    else if (clientMsg.type == 2) {
        // Deposit
        StatusCode res = lockerManager.deposit(clientMsg.lockerId, clientMsg.pin);
        response.status = static_cast<uint16_t>(res);
        if (res == StatusCode::SUCCESS) {
            LOG_INFO("-> Package deposited in locker " + std::to_string(clientMsg.lockerId) + ".");
        } else {
            LOG_WARN("-> Deposit failed for locker " + std::to_string(clientMsg.lockerId) + ".");
        }
    } 
    else if (clientMsg.type == 3) {
        // Pickup
        StatusCode res = lockerManager.pickup(clientMsg.lockerId, clientMsg.pin);
        response.status = static_cast<uint16_t>(res);
        if (res == StatusCode::SUCCESS) {
            LOG_INFO("-> Package picked up from locker " + std::to_string(clientMsg.lockerId) + ".");
        } else {
            LOG_WARN("-> Pickup failed for locker " + std::to_string(clientMsg.lockerId) + ".");
        }
    } 
    else {
        response.status = static_cast<uint16_t>(StatusCode::BAD_REQUEST);
    }

    int bytesSent = send(clientSocket, (char*)&response, sizeof(response), 0);
    if (bytesSent == SOCKET_ERROR) {
        LOG_ERROR("Failed to send response to client.");
        return false;
    }

    return true;
}