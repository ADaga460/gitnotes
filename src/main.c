#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "db.h"
#include "git_integration.h"
#include "notes.h"
#include "sync.h"

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
        printf("Usage: clisuite attach [type] [path] --note [note_id]\n");
        printf("Types: commit, file, dir\n");
        printf("Examples:\n");
        printf("  clisuite attach commit HEAD --note note_123\n");
        printf("  clisuite attach file src/main.c --note note_456\n");
        printf("  clisuite attach dir src/ --note note_789\n");
        return;
    }

    const char *target_type = argv[2];
    const char *target_path = argc > 3 ? argv[3] : NULL;
    const char *note_id = NULL;

    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
            note_id = argv[++i];
        }
    }

    if (!note_id) {
        fprintf(stderr, "No note specified. Use --note [note_id]\n");
        return;
    }

    if (strcmp(target_type, "commit") == 0) {
        const char *commit = target_path;
        if (strcmp(commit, "HEAD") == 0) {
            commit = get_current_commit();
            if (!commit) {
                fprintf(stderr, "Could not get current commit.\n");
                return;
            }
        }
        attach_to_commit(commit, note_id, NULL);
    } 
    else if (strcmp(target_type, "file") == 0 || strcmp(target_type, "dir") == 0) {
        if (!target_path) {
            fprintf(stderr, "Provide %s path.\n", target_type);
            return;
        }
        attach_note_to_target(note_id, target_type, target_path);
    }
    else {
        fprintf(stderr, "Unknown target type: %s\n", target_type);
    }
}

void handle_show(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: clisuite show [type] [path] [--recursive]\n");
        printf("Types: commit, file, dir\n");
        printf("Examples:\n");
        printf("  clisuite show commit HEAD\n");
        printf("  clisuite show file src/main.c\n");
        printf("  clisuite show dir src/\n");
        printf("  clisuite show dir src/ --recursive\n");
        return;
    }

    const char *target_type = argv[2];
    const char *target_path = argc > 3 ? argv[3] : NULL;
    
    int recursive = 0;
    for (int i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--recursive") == 0 || strcmp(argv[i], "-r") == 0) {
            recursive = 1;
            break;
        }
    }

    if (strcmp(target_type, "commit") == 0) {
        const char *commit = target_path ? target_path : "HEAD";
        if (strcmp(commit, "HEAD") == 0) {
            commit = get_current_commit();
            if (!commit) {
                fprintf(stderr, "Could not get current commit.\n");
                return;
            }
        }
        show_commit_metadata(commit);
    }
    else if (strcmp(target_type, "dir") == 0) {
        if (!target_path) {
            fprintf(stderr, "Provide directory path.\n");
            return;
        }
        if (recursive) {
            show_directory_notes_recursive(target_path);
        } else {
            show_target_notes(target_type, target_path);
        }
    }
    else if (strcmp(target_type, "file") == 0) {
        if (!target_path) {
            fprintf(stderr, "Provide file path.\n");
            return;
        }
        show_target_notes(target_type, target_path);
    }
    else {
        fprintf(stderr, "Unknown target type: %s\n", target_type);
    }
}

void handle_hook(int argc, char *argv[]) {
    if (argc < 3) return;

    if (strcmp(argv[2], "post-commit") == 0) {
        printf("[clisuite] post-commit hook triggered\n");
    }
    else if (strcmp(argv[2], "post-merge") == 0) {
        printf("[clisuite] post-merge hook triggered\n");
    }
}

void print_help(void) {
    printf("clisuite - Git-integrated productivity CLI\n\n");
    printf("Usage:\n");
    printf("  clisuite init                              - Initialize clisuite in git repo\n");
    printf("  clisuite install-hooks                     - Install git hooks\n");
    printf("  clisuite todo [add|list|done|delete]       - Manage private todos\n");
    printf("  clisuite note [add|list|show|delete]       - Manage shared notes\n");
    printf("  clisuite attach [type] [path] --note [id]  - Attach note to commit/file/dir\n");
    printf("  clisuite show [type] [path] [--recursive]  - Show notes for commit/file/dir\n");
    printf("  clisuite sync                              - Sync metadata to .clisuite/\n");
    printf("  clisuite pull                              - Pull remote metadata\n");
    printf("  clisuite reset [--tracked-only]            - Erase all clisuite data\n");
    printf("  clisuite help\n");
    printf("\nExamples:\n");
    printf("  clisuite attach commit HEAD --note note_123\n");
    printf("  clisuite attach file src/main.c --note note_456\n");
    printf("  clisuite show file src/main.c\n");
    printf("  clisuite show dir src/ --recursive\n");
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
        install_sync_hooks();
        return 0;
    }

    if (strcmp(argv[1], "sync") == 0) {
        sync_to_tracked();
        return 0;
    }

    if (strcmp(argv[1], "pull") == 0) {
        sync_from_tracked();
        return 0;
    }

    if (strcmp(argv[1], "reset") == 0) {
        int tracked_only = 0;
        if (argc > 2 && strcmp(argv[2], "--tracked-only") == 0) {
            tracked_only = 1;
        }
        printf("WARNING: This will erase all clisuite data!\n");
        printf("Continue? (y/n): ");
        char confirm;
        scanf(" %c", &confirm);
        if (confirm == 'y' || confirm == 'Y') {
            reset_clisuite(tracked_only);
        } else {
            printf("Reset cancelled.\n");
        }
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