#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../include/protocol.hpp"

#pragma comment(lib, "ws2_32.lib")

void sendRequest(SOCKET sock, MessageType type, uint8_t lockerId, const char* pin = nullptr) {
    MessageHeader header{};
    header.type = type;
    header.lockerId = lockerId;
    header.status = StatusCode::SUCCESS;

    PackageData pkgData{};
    if (pin) {
        std::strncpy(pkgData.pin, pin, sizeof(pkgData.pin) - 1);
        header.dataLength = sizeof(PackageData);
    } else {
        header.dataLength = 0;
    }

    // Trimitem antetul
    send(sock, reinterpret_cast<const char*>(&header), sizeof(MessageHeader), 0);

    // Trimitem corpul pachetului dacă există
    if (header.dataLength > 0) {
        send(sock, reinterpret_cast<const char*>(&pkgData), sizeof(PackageData), 0);
    }

    // Citim răspunsul de la server
    MessageHeader responseHeader{};
    int bytesReceived = recv(sock, reinterpret_cast<char*>(&responseHeader), sizeof(MessageHeader), 0);
    if (bytesReceived == sizeof(MessageHeader)) {
        std::cout << "[Client] Response received - Status Code: " << static_cast<uint16_t>(responseHeader.status) << "\n";
    } else {
        std::cout << "[Client] Failed to receive proper response.\n";
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to Parcel Locker Server.\n";

    // Test 1: Depunere cu succes pe lockerul 1
    std::cout << "\n--- Test: Normal Deposit (Locker 1) --- \n";
    sendRequest(clientSocket, MessageType::DEPOSIT_PACKAGE, 1, "4321");

    // Test 2: Încercare de depunere pe același locker ocupat (Așteptat: BOX_FULL)
    std::cout << "\n--- Test: Box Full Error (Locker 1) --- \n";
    sendRequest(clientSocket, MessageType::DEPOSIT_PACKAGE, 1, "9999");

    // Test 3: Ridicare cu PIN greșit (Așteptat: UNAUTHORIZED)
    std::cout << "\n--- Test: Wrong PIN Error (Locker 1) --- \n";
    sendRequest(clientSocket, MessageType::PICKUP_PACKAGE, 1, "0000");

    // Test 4: Accesare ID invalid de locker (Așteptat: NOT_FOUND)
    std::cout << "\n--- Test: Invalid Locker ID Error (Locker 99) --- \n";
    sendRequest(clientSocket, MessageType::DEPOSIT_PACKAGE, 99, "1234");

    // Test 5: Ridicare cu PIN corect (Așteptat: SUCCESS)
    std::cout << "\n--- Test: Normal Pickup (Locker 1) --- \n";
    sendRequest(clientSocket, MessageType::PICKUP_PACKAGE, 1, "4321");

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}