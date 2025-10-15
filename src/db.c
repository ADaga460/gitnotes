#include "../include/db.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

sqlite3* init_db(void) {
    // Ensure data directory exists
    struct stat st = {0};
    if (stat("data", &st) == -1) {
        mkdir("data", 0755);
    }

    sqlite3 *db;
    int rc = sqlite3_open("data/clisuite.db", &db);
    if (rc) {
        fprintf(stderr, "DB open failed: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    const char *init_sql =
        "CREATE TABLE IF NOT EXISTS todos ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "task TEXT NOT NULL,"
        "priority INTEGER DEFAULT 0,"
        "done INTEGER DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT,"
        "content TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    if (exec_query(db, init_sql) != SQLITE_OK) {
        fprintf(stderr, "DB init failed.\n");
    }

    return db;
}

void close_db(sqlite3 *db) {
    if (db) sqlite3_close(db);
}

int exec_query(sqlite3 *db, const char *query) {
    char *err = NULL;
    int rc = sqlite3_exec(db, query, 0, 0, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    return rc;
}
