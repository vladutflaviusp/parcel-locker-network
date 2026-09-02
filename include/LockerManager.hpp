#pragma once

#include <vector>
#include <cstring>
#include "protocol.hpp"

struct LockerServerState {
    bool isOccupied = false;
    char pin[5] = {0};
};

class LockerManager {
private:
    std::vector<LockerServerState> lockers;

public:
    LockerManager() : lockers(MAX_LOCKERS) {}

    bool isInvalidId(uint8_t lockerId) const {
        return lockerId >= MAX_LOCKERS;
    }

    bool isOccupied(uint8_t lockerId) const {
        return lockers[lockerId].isOccupied;
    }

    StatusCode deposit(uint8_t lockerId, const char* pin) {
        if (lockers[lockerId].isOccupied) {
            return StatusCode::BOX_FULL;
        }
        lockers[lockerId].isOccupied = true;
        std::memcpy(lockers[lockerId].pin, pin, 5);
        return StatusCode::SUCCESS;
    }

    StatusCode pickup(uint8_t lockerId, const char* pin) {
        if (!lockers[lockerId].isOccupied) {
            return StatusCode::NOT_FOUND;
        }
        if (std::memcmp(lockers[lockerId].pin, pin, 5) != 0) {
            return StatusCode::UNAUTHORIZED;
        }
        lockers[lockerId].isOccupied = false;
        std::memset(lockers[lockerId].pin, 0, 5);
        return StatusCode::SUCCESS;
    }
};