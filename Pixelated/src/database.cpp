#include "database.h"
#include <sqlite3/sqlite3.h>
#include <iostream>

bool Database::open(const std::string& dbPath) {
    return sqlite3_open(dbPath.c_str(), &db) == SQLITE_OK;
}

bool Database::getImageByName(const std::string& name, std::vector<unsigned char>& outData) {
    const char* query = "SELECT data FROM images WHERE name = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    bool success = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int size = sqlite3_column_bytes(stmt, 0);
        outData.assign((const unsigned char*)blob, (const unsigned char*)blob + size);
        success = true;
    }

    sqlite3_finalize(stmt);
    return success;
}

std::vector<ImageEntry> Database::getAllImages() {
    std::vector<ImageEntry> images;
    const char* query = "SELECT name, category, data FROM images";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        return images;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const void* blob = sqlite3_column_blob(stmt, 2);
        int size = sqlite3_column_bytes(stmt, 2);

        if (name && category && blob) {
            ImageEntry entry;
            entry.name = name;
            entry.category = category;
            entry.data.assign((const unsigned char*)blob, (const unsigned char*)blob + size);
            images.push_back(std::move(entry));
        }
    }

    sqlite3_finalize(stmt);
    return images;
}

void Database::close() {
    if (db) sqlite3_close(db);
}