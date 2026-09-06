#ifndef SERVER_HPP
#define SERVER_HPP

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "LockerManager.hpp"
#include "Logger.hpp"

class Server {
private:
    SOCKET serverSocket;
    std::vector<SOCKET> clientSockets;
    LockerManager lockerManager;
    const int PORT = 8080;

    bool handleClientMessage(SOCKET clientSocket);

public:
    Server();
    ~Server();

    bool initialize();
    void run();
};

#endif