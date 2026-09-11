#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <sqlite3.h>
#include <string>
#include <iostream>

class Database {
private:
    sqlite3* db;

public:
    Database(const std::string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
            db = nullptr;
        } else {
            createTable();
        }
    }

    ~Database() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool isOpen() const { return db != nullptr; }
    sqlite3* getHandle() const { return db; }

private:
    void createTable() {
        const char* sql = "CREATE TABLE IF NOT EXISTS lockers ("
                          "id INTEGER PRIMARY KEY, "
                          "is_occupied INTEGER NOT NULL, "
                          "pin TEXT NOT NULL);";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
        }
    }
};

#endif