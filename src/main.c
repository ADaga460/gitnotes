#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "db.h"
#include "git_integration.h"
#include "notes.h"

void print_help(void);

void handle_todo(int argc, char *argv[], sqlite3 *db) {
    if (argc < 3) { 
        printf("Usage: clisuite todo [add|list|done|delete]\n"); 
        return; 
    }

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
    else if (strcmp(argv[2], "delete") == 0) {
        if (argc < 4) { printf("Provide todo ID.\n"); return; }
        char sql[128];
        snprintf(sql, sizeof(sql), "DELETE FROM todos WHERE id=%d;", atoi(argv[3]));
        exec_query(db, sql);
        printf("Deleted todo %s.\n", argv[3]);
    }
    else {
        printf("Unknown todo command.\n");
    }
}

void handle_note(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite note [add|list|show|delete]\n");
        return;
    }

    if (strcmp(argv[2], "add") == 0) {
        if (argc < 4) { printf("Provide note title.\n"); return; }
        const char *content = argc > 4 ? argv[4] : "";
        add_note(argv[3], content);
    }
    else if (strcmp(argv[2], "list") == 0) {
        list_notes();
    }
    else if (strcmp(argv[2], "show") == 0) {
        if (argc < 4) { printf("Provide note ID.\n"); return; }
        show_note(argv[3]);
    }
    else if (strcmp(argv[2], "delete") == 0) {
        if (argc < 4) { printf("Provide note ID.\n"); return; }
        delete_note(argv[3]);
    }
    else {
        printf("Unknown note command.\n");
    }
}

void handle_attach(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite attach [commit-hash] [--note note_id] [--todo todo_id]\n");
        printf("       clisuite attach HEAD [--note note_id] [--todo todo_id]\n");
        return;
    }

    const char *commit = argv[2];
    if (strcmp(commit, "HEAD") == 0) {
        commit = get_current_commit();
        if (!commit) {
            fprintf(stderr, "Could not get current commit.\n");
            return;
        }
    }

    const char *note_id = NULL;
    const char *todo_id = NULL;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
            note_id = argv[++i];
        } else if (strcmp(argv[i], "--todo") == 0 && i + 1 < argc) {
            todo_id = argv[++i];
        }
    }

    attach_to_commit(commit, note_id, todo_id);
}

void handle_show(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite show [commit-hash|HEAD]\n");
        return;
    }

    const char *commit = argv[2];
    if (strcmp(commit, "HEAD") == 0) {
        commit = get_current_commit();
        if (!commit) {
            fprintf(stderr, "Could not get current commit.\n");
            return;
        }
    }

    show_commit_metadata(commit);
}

void handle_hook(int argc, char *argv[]) {
    if (argc < 3) return;

    if (strcmp(argv[2], "post-commit") == 0) {
        printf("[clisuite] post-commit hook triggered\n");
        // Future: auto-attach pending items
    }
    else if (strcmp(argv[2], "post-merge") == 0) {
        printf("[clisuite] post-merge hook triggered\n");
        // Future: merge metadata
    }
}

void print_help(void) {
    printf("clisuite - Git-integrated note-taking CLI\n\n");
    printf("Usage:\n");
    printf("  clisuite init                    - Initialize clisuite in git repo\n");
    printf("  clisuite install-hooks           - Install git hooks\n");
    printf("  clisuite todo [add|list|done|delete]\n");
    printf("  clisuite note [add|list|show|delete]\n");
    printf("  clisuite attach [commit] [--note id] [--todo id]\n");
    printf("  clisuite show [commit]           - Show commit metadata\n");
    printf("  clisuite help\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) { 
        print_help(); 
        return 0; 
    }

    if (strcmp(argv[1], "init") == 0) {
        if (init_git_structure() == 0) {
            sqlite3 *db = init_db();
            if (db) close_db(db);
        }
        return 0;
    }

    if (strcmp(argv[1], "install-hooks") == 0) {
        install_hooks();
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    sqlite3 *db = NULL;
    if (strcmp(argv[1], "todo") == 0) {
        db = init_db();
        if (!db) return 1;
        handle_todo(argc, argv, db);
    }
    else if (strcmp(argv[1], "note") == 0) {
        handle_note(argc, argv);
    }
    else if (strcmp(argv[1], "attach") == 0) {
        handle_attach(argc, argv);
    }
    else if (strcmp(argv[1], "show") == 0) {
        handle_show(argc, argv);
    }
    else if (strcmp(argv[1], "hook") == 0) {
        handle_hook(argc, argv);
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        print_help();
    }

    if (db) close_db(db);
    return 0;
}