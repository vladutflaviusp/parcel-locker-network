#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>

const int SERVER_PORT = 8080;
const int MAX_LOCKERS = 10;

enum class MessageType : uint8_t {
    CLIENT_LOGIN = 1,    
    DEPOSIT_PACKAGE = 2,  
    PICKUP_PACKAGE = 3,   
    SERVER_RESPONSE = 4  
};

enum class StatusCode : uint16_t {
    SUCCESS = 200,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    NOT_FOUND = 404,
    BOX_FULL = 507
};

#pragma pack(push, 1)
struct ClientMessage {
    uint8_t type;
    uint8_t lockerId;
    char pin[5];
};

struct ServerResponse {
    uint16_t status;
    uint8_t isOccupied;
};
#pragma pack(pop)

struct MessageHeader {
    MessageType type;
    uint8_t lockerId;    
    StatusCode status;   
    uint16_t dataLength; 
};

struct PackageData {
    char pin[5];         
    bool isOccupied;    
};

#endif