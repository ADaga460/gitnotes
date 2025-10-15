#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "../include/db.h"

void print_help(void);

void handle_todo(int argc, char *argv[], sqlite3 *db) {
    if (argc < 3) { printf("Usage: clisuite todo [add|list|done]\n"); return; }

    if (strcmp(argv[2], "add") == 0) {
        if (argc < 4) { printf("Provide task text.\n"); return; }
        char sql[512];
        snprintf(sql, sizeof(sql),
                 "INSERT INTO todos(task,priority,done) VALUES('%s',0,0);",
                 argv[3]);
        exec_query(db, sql);
        printf("Added todo: %s\n", argv[3]);
    } 
    else if (strcmp(argv[2], "list") == 0) {
        const char *sql = "SELECT id,task,done FROM todos;";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            printf("Todos:\n");
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                const unsigned char *task = sqlite3_column_text(stmt, 1);
                int done = sqlite3_column_int(stmt, 2);
                printf(" [%c] %d: %s\n", done ? 'x' : ' ', id, task);
            }
        }
        sqlite3_finalize(stmt);
    } 
    else if (strcmp(argv[2], "done") == 0) {
        if (argc < 4) { printf("Provide todo ID.\n"); return; }
        char sql[128];
        snprintf(sql, sizeof(sql),
                 "UPDATE todos SET done=1 WHERE id=%d;", atoi(argv[3]));
        exec_query(db, sql);
        printf("Marked todo %s as done.\n", argv[3]);
    } 
    else {
        printf("Unknown todo command.\n");
    }
}

void print_help(void) {
    printf("Usage:\n");
    printf("  clisuite todo [add|list|done]\n");
    printf("  clisuite help\n");
}

int main(int argc, char *argv[]) {
    sqlite3 *db = init_db();
    if (!db) return 1;

    if (argc < 2) { print_help(); close_db(db); return 0; }

    if (strcmp(argv[1], "todo") == 0) {
        handle_todo(argc, argv, db);
    } else if (strcmp(argv[1], "help") == 0) {
        print_help();
    } else {
        printf("Unknown command.\n");
    }

    close_db(db);
    return 0;
}
