#include <iostream>
#include <winsock2.h>
#include <cstring>
#include <ws2tcpip.h>
#include <limits>
#include "protocol.hpp"

#pragma comment(lib, "ws2_32.lib")

void printMenu() {
    std::cout << "\n=== PARCEL LOCKER CLIENT ===\n";
    std::cout << "1. Status Check / Login\n";
    std::cout << "2. Deposit Package\n";
    std::cout << "3. Pickup Package\n";
    std::cout << "0. Exit\n";
    std::cout << "Choose an option: ";
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to Parcel Locker Server on port " << SERVER_PORT << ".\n";

    while (true) {
        printMenu();
        
        long long choiceInput;
        std::cin >> choiceInput;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "[CLIENT ERROR] Invalid input! Please enter a valid number for the menu.\n";
            continue;
        }

        if (choiceInput == 0) {
            break;
        }

        if (choiceInput < 1 || choiceInput > 3) {
            std::cout << "[CLIENT ERROR] Invalid choice. Please choose an option between 0 and 3.\n";
            continue;
        }

        ClientMessage msg;
        msg.type = static_cast<uint8_t>(choiceInput);

        long long lockerIdInput;
        std::cout << "Enter Locker ID (0 - " << (MAX_LOCKERS - 1) << "): ";
        std::cin >> lockerIdInput;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "[CLIENT ERROR] Invalid Locker ID format! Please enter a numeric value.\n";
            continue;
        }

        if (lockerIdInput < 0 || lockerIdInput > 255) {
            std::cout << "[CLIENT ERROR] Locker ID out of range! Please enter a value between 0 and " << (MAX_LOCKERS - 1) << ".\n";
            continue;
        }

        msg.lockerId = static_cast<uint8_t>(lockerIdInput);

        if (choiceInput == 2 || choiceInput == 3) {
            std::cout << "Enter 4-digit PIN: ";
            std::string pinStr;
            std::cin >> pinStr;
            std::snprintf(msg.pin, sizeof(msg.pin), "%s", pinStr.c_str());
        } else {
            std::memset(msg.pin, 0, sizeof(msg.pin));
        }

        send(sock, (char*)&msg, sizeof(msg), 0);

        ServerResponse response;
        int bytesReceived = recv(sock, (char*)&response, sizeof(response), 0);
        if (bytesReceived > 0) {
            std::cout << "[SERVER RESPONSE] Status Code: " << response.status;
            if (msg.type == 1 && response.status == 200) {
                std::cout << " | Locker is: " << (response.isOccupied ? "OCCUPIED" : "FREE");
            }
            std::cout << "\n";
        } else {
            std::cerr << "Server disconnected or error receiving response.\n";
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}