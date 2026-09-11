// Server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include "Database.hpp"
#include "LockerManager.hpp"
#include "Logger.hpp"

class Server {
private:
    SOCKET serverSocket;
    std::vector<SOCKET> clientSockets;
    Database db;
    LockerManager lockerManager;
    int port;

    bool handleClientMessage(SOCKET clientSocket);

public:
    Server(int port = 8080);
    ~Server();

    bool initialize();
    void run();
};

#endif