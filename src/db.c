#include "../include/db.h"
#include "../include/git_integration.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

sqlite3* init_db(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "Not in a git repository. Run 'gitnote init' first.\n");
        return NULL;
    }

    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s/gitnote/gitnote.db", git_dir);

    sqlite3 *db;
    int rc = sqlite3_open(db_path, &db);
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