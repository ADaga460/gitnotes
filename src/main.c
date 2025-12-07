#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "db.h"
#include "git_integration.h"
#include "notes.h"
#include "sync.h"
#include <sys/stat.h>
#include "verify.h"
#include <_time.h>

void print_help(void);

void handle_todo(int argc, char *argv[], sqlite3 *db) {
    if (argc < 3) { 
        printf("Usage: gitnote todo [add|list|done|delete]\n"); 
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
        printf("Usage: gitnote note [add|list|show|edit|delete|search]\n");
        return;
    }

    if (strcmp(argv[2], "add") == 0) {
        if (argc < 4) { printf("Provide note title.\n"); return; }
        const char *content = argc > 4 ? argv[4] : "";
        add_note(argv[3], content);
    }
    else if (strcmp(argv[2], "edit") == 0) {
        if (argc < 4) { printf("Provide note ID.\n"); return; }
        const char *title = argc > 4 ? argv[4] : NULL;
        const char *content = argc > 5 ? argv[5] : NULL;
        if (!title && !content) {
            printf("Provide new title and/or content.\n");
            return;
        }
        edit_note(argv[3], title, content);
    }
    else if (strcmp(argv[2], "search") == 0) {
        if (argc < 4) { printf("Provide search query.\n"); return; }
        search_notes(argv[3]);
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
        printf("Usage: gitnote attach [path] [--note note_id | \"title\" \"content\"]\n");
        printf("       gitnote attach commit [hash] --note note_id\n");
        printf("Examples:\n");
        printf("  gitnote attach src/main.c \"Memory leak\" \"Line 45 leaks\"\n");
        printf("  gitnote attach src/ --note note_123\n");
        printf("  gitnote attach commit HEAD --note note_456\n");
        return;
    }

    const char *arg1 = argv[2];
    
    // Special case: explicit commit attachment
    if (strcmp(arg1, "commit") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Provide commit hash or HEAD.\n");
            return;
        }
        
        const char *note_id = NULL;
        for (int i = 4; i < argc; i++) {
            if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
                note_id = argv[++i];
                break;
            }
        }
        
        if (!note_id) {
            fprintf(stderr, "Provide --note [note_id] for commit attachments.\n");
            return;
        }
        
        const char *commit = argv[3];
        if (strcmp(commit, "HEAD") == 0) {
            commit = get_current_commit();
            if (!commit) {
                fprintf(stderr, "Could not get current commit.\n");
                return;
            }
        }
        attach_to_commit(commit, note_id, NULL);
        return;
    }
    
    const char *path = arg1;
    struct stat st;
    
    if (stat(path, &st) != 0) {
        fprintf(stderr, "Path not found: %s\n", path);
        return;
    }
    
    const char *target_type = S_ISDIR(st.st_mode) ? "dir" : "file";
    
    const char *note_id = NULL;
    
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--note") == 0 && i + 1 < argc) {
            note_id = argv[++i];
            break;
        }
    }
    
    if (!note_id) {
        if (argc < 4) {
            fprintf(stderr, "Provide note title or use --note [note_id].\n");
            return;
        }
        
        const char *title = argv[3];
        const char *content = argc > 4 ? argv[4] : "";
        
        char *git_dir = get_git_dir();
        if (!git_dir) {
            fprintf(stderr, "Not in a git repository.\n");
            return;
        }
        
        static char new_note_id[32];
        static int counter = 0;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        snprintf(new_note_id, sizeof(new_note_id), "note_%ld%03ld_%d", 
                 ts.tv_sec, ts.tv_nsec / 1000000, counter++);
        
        char note_path[512];
        snprintf(note_path, sizeof(note_path), "%s/gitnote/notes/%s.json", git_dir, new_note_id);
        
        FILE *f = fopen(note_path, "w");
        if (!f) {
            fprintf(stderr, "Could not create note file.\n");
            return;
        }
        
        time_t now = time(NULL);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fprintf(f, "{\n");
        fprintf(f, "  \"id\": \"%s\",\n", new_note_id);
        fprintf(f, "  \"title\": \"%s\",\n", title);
        fprintf(f, "  \"content\": \"%s\",\n", content);
        fprintf(f, "  \"created_at\": \"%s\"\n", timestamp);
        fprintf(f, "}\n");
        fclose(f);
        
        printf("\033[90mNote created: \033[0m%s\n", new_note_id);
        note_id = new_note_id;
    }
    
    attach_note_to_target(note_id, target_type, path);
}

void handle_show(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: gitnote show [type] [path] [--recursive]\n");
        printf("Types: commit, file, dir\n");
        printf("Examples:\n");
        printf("  gitnote show commit HEAD\n");
        printf("  gitnote show file src/main.c\n");
        printf("  gitnote show dir src/\n");
        printf("  gitnote show dir src/ --recursive\n");
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
        printf("[gitnote] post-commit hook triggered\n");
    }
    else if (strcmp(argv[2], "post-merge") == 0) {
        printf("[gitnote] post-merge hook triggered\n");
    }
}

void print_help(void) {
    printf("gitnote - Git-integrated productivity CLI\n\n");
    printf("Usage:\n");
    printf("  gitnote init                              - Initialize gitnote in git repo\n");
    printf("  gitnote install-hooks                     - Install git hooks\n");
    printf("  gitnote verify                            - Check for orphaned attachments\n");
    printf("  gitnote repair                            - Clean up orphaned attachments\n");
    printf("\n");
    printf("Notes (shared):\n");
    printf("  gitnote note add \"title\" \"content\"        - Create note\n");
    printf("  gitnote note edit <id> \"title\" \"content\" - Edit note\n");
    printf("  gitnote note list                         - List all notes\n");
    printf("  gitnote note show <id>                    - Show note details\n");
    printf("  gitnote note delete <id>                  - Delete note\n");
    printf("  gitnote note search \"query\"               - Search notes\n");
    printf("\n");
    printf("Attach notes:\n");
    printf("  gitnote attach <path> \"title\" \"content\"  - Create & attach note (auto-detects file/dir)\n");
    printf("  gitnote attach <path> --note <id>         - Attach existing note\n");
    printf("  gitnote attach commit <hash> --note <id>  - Attach to commit\n");
    printf("\n");
    printf("View notes:\n");
    printf("  gitnote show file <path>                  - Show notes for file\n");
    printf("  gitnote show dir <path> [--recursive]     - Show notes for directory\n");
    printf("  gitnote show commit <hash>                - Show notes for commit\n");
    printf("\n");
    printf("Todos (private):\n");
    printf("  gitnote todo add \"task\"                   - Add todo\n");
    printf("  gitnote todo list                         - List todos\n");
    printf("  gitnote todo done <id>                    - Mark todo done\n");
    printf("  gitnote todo delete <id>                  - Delete todo\n");
    printf("\n");
    printf("Maintenance:\n");
    printf("  gitnote migrate <old-path> <new-path>     - Migrate attachments after file rename\n");
    printf("  gitnote sync                              - Sync metadata to .gitnote/\n");
    printf("  gitnote pull                              - Pull remote metadata\n");
    printf("  gitnote reset [--tracked-only]            - Erase all gitnote data\n");
    printf("\n");
    printf("Examples:\n");
    printf("  # Quick attach - create note and attach in one command\n");
    printf("  gitnote attach src/main.c \"Memory leak\" \"Line 45 leaks memory\"\n");
    printf("  gitnote attach src/ \"TODO\" \"Refactor this module\"\n");
    printf("\n");
    printf("  # Use existing note\n");
    printf("  gitnote attach src/helper.c --note note_123456789\n");
    printf("\n");
    printf("  # View notes\n");
    printf("  gitnote show file src/main.c\n");
    printf("  gitnote show dir src/ --recursive\n");
    printf("\n");
    printf("  # Search and edit\n");
    printf("  gitnote note search \"memory\"\n");
    printf("  gitnote note edit note_123 \"FIXED\" \"No longer leaks\"\n");
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
        printf("WARNING: This will erase all gitnote data!\n");
        printf("Continue? (y/n): ");
        char confirm;
        scanf(" %c", &confirm);
        if (confirm == 'y' || confirm == 'Y') {
            reset_gitnote(tracked_only);
        } else {
            printf("Reset cancelled.\n");
        }
        return 0;
    }

    if (strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "verify") == 0) {
        verify_gitnote();
        return 0;
    }

    if (strcmp(argv[1], "repair") == 0) {
        repair_gitnote();
        return 0;
    }
    
    if (strcmp(argv[1], "migrate") == 0) {
        if (argc < 4) {
            printf("Usage: gitnote migrate <old-path> <new-path>\n");
            printf("Example: gitnote migrate src/old.c src/new.c\n");
            return 1;
        }
        migrate_attachment(argv[2], argv[3]);
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