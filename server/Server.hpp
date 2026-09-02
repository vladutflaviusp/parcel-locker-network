#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "../include/protocol.hpp"
#include "LockerManager.hpp"

class Server {
private:
    SOCKET serverSocket;
    LockerManager lockerManager;
    std::vector<SOCKET> clientSockets;

    void sendResponse(SOCKET sock, MessageType type, uint8_t lockerId, StatusCode status);
    void handleMessage(SOCKET sock, const MessageHeader& header);

public:
    Server();
    ~Server();
    bool initialize();
    void run();
};