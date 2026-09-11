#ifndef LOCKER_MANAGER_HPP
#define LOCKER_MANAGER_HPP

#include "Database.hpp"
#include "protocol.hpp"
#include <string>

class LockerManager {
private:
    Database& db;

public:
    explicit LockerManager(Database& database) : db(database) {}

    bool isOccupied(int lockerId) {
        std::string query = "SELECT is_occupied FROM lockers WHERE id = " + std::to_string(lockerId) + ";";
        sqlite3_stmt* stmt;
        bool occupied = false;

        if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                occupied = sqlite3_column_int(stmt, 0) != 0;
            }
            sqlite3_finalize(stmt);
        }
        return occupied;
    }

    StatusCode deposit(int lockerId, const std::string& pin) {
        if (isOccupied(lockerId)) {
            return StatusCode::LockerAlreadyOccupied;
        }

        std::string query = "INSERT OR REPLACE INTO lockers (id, is_occupied, pin) VALUES (?, 1, ?);";
        sqlite3_stmt* stmt;
        StatusCode status = StatusCode::Error;

        if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, lockerId);
            sqlite3_bind_text(stmt, 2, pin.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                status = StatusCode::SUCCESS;
            }
            sqlite3_finalize(stmt);
        }
        return status;
    }

    StatusCode pickup(int lockerId, const std::string& pin) {
        if (!verifyPin(lockerId, pin)) {
            return StatusCode::InvalidPin;
        }

        std::string query = "UPDATE lockers SET is_occupied = 0, pin = '' WHERE id = ?;";
        sqlite3_stmt* stmt;
        StatusCode status = StatusCode::Error;

        if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, lockerId);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                status = StatusCode::SUCCESS;
            }
            sqlite3_finalize(stmt);
        }
        return status;
    }

private:
    bool verifyPin(int lockerId, const std::string& pin) {
        std::string query = "SELECT pin FROM lockers WHERE id = ? AND is_occupied = 1;";
        sqlite3_stmt* stmt;
        bool valid = false;

        if (sqlite3_prepare_v2(db.getHandle(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, lockerId);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* storedPin = sqlite3_column_text(stmt, 0);
                if (storedPin && pin == reinterpret_cast<const char*>(storedPin)) {
                    valid = true;
                }
            }
            sqlite3_finalize(stmt);
        }
        return valid;
    }
};

#endif