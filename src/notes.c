#include "notes.h"
#include "git_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static char* generate_note_id(void) {
    static char id[32];
    static int counter = 0;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(id, sizeof(id), "note_%ld%03ld_%d", ts.tv_sec, ts.tv_nsec / 1000000, counter++);
    return id;
}

int add_note(const char *title, const char *content) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "\033[90mNot in a git repository.\033[0m\n");
        return -1;
    }
    
    char *note_id = generate_note_id();
    char note_path[512];
    snprintf(note_path, sizeof(note_path), 
             "%s/gitnote/notes/%s.json", git_dir, note_id);
    
    FILE *f = fopen(note_path, "w");
    if (!f) {
        fprintf(stderr, "\033[90mCould not create note file.\033[0m\n");
        return -1;
    }
    
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    fprintf(f, "{\n");
    fprintf(f, "  \"id\": \"%s\",\n", note_id);
    fprintf(f, "  \"title\": \"%s\",\n", title ? title : "Untitled");
    fprintf(f, "  \"content\": \"%s\",\n", content ? content : "");
    fprintf(f, "  \"created_at\": \"%s\"\n", timestamp);
    fprintf(f, "}\n");
    
    fclose(f);
    printf("\033[90mNote created: \033[0m%s\n", note_id);
    return 0;
}

void list_notes(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "\033[90mNot in a git repository.\033[0m\n");
        return;
    }
    
    char notes_dir[512];
    snprintf(notes_dir, sizeof(notes_dir), "%s/gitnote/notes", git_dir);
    
    DIR *dir = opendir(notes_dir);
    if (!dir) {
        printf("\033[90mNo notes found.\033[0m\n");
        return;
    }
    
    printf("\033[90mNotes:\033[0m\n");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".json")) {
            char note_path[512];
            snprintf(note_path, sizeof(note_path), "%s/%s", notes_dir, entry->d_name);
            
            FILE *f = fopen(note_path, "r");
            if (f) {
                char line[512];
                char title[256] = "Untitled";
                char id[64] = "";
                
                while (fgets(line, sizeof(line), f)) {
                    if (strstr(line, "\"id\"")) {
                        sscanf(line, "  \"id\": \"%[^\"]\"", id);
                    }
                    if (strstr(line, "\"title\"")) {
                        sscanf(line, "  \"title\": \"%[^\"]\"", title);
                    }
                }
                printf("  \033[1;92m[%s]\033[0m %s\n", id, title);
                fclose(f);
            }
        }
    }
    closedir(dir);
}

int migrate_attachment(const char *old_path, const char *new_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/gitnote/metadata", git_dir);
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        printf("\033[90mNo attachments to migrate.\033[0m\n");
        return 0;
    }
    
    int migrated = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, "attach_") || !strstr(entry->d_name, ".json"))
            continue;
        
        char attach_path[512];
        snprintf(attach_path, sizeof(attach_path), "%s/%s", metadata_dir, entry->d_name);
        
        FILE *f = fopen(attach_path, "r");
        if (!f) continue;
        
        char content[2048] = "";
        char line[512];
        char ttype[64] = "";
        char tpath[256] = "";
        char note_id[128] = "";
        int found_old = 0;
        
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "\"target_type\"")) {
                sscanf(line, "  \"target_type\": \"%[^\"]\"", ttype);
            }
            if (strstr(line, "\"target_path\"")) {
                sscanf(line, "  \"target_path\": \"%[^\"]\"", tpath);
            }
            if (strstr(line, "\"note_id\"")) {
                sscanf(line, "  \"note_id\": \"%[^\"]\"", note_id);
            }
        }
        fclose(f);
        
        if (strcmp(tpath, old_path) == 0) {
            found_old = 1;
            
            f = fopen(attach_path, "w");
            if (f) {
                fprintf(f, "{\n");
                fprintf(f, "  \"note_id\": \"%s\",\n", note_id);
                fprintf(f, "  \"target_type\": \"%s\",\n", ttype);
                fprintf(f, "  \"target_path\": \"%s\"\n", new_path);
                fprintf(f, "}\n");
                fclose(f);
                
                printf("\033[90mMigrated attachment: %s\033[0m\n", note_id);
                migrated++;
            }
        }
    }
    closedir(dir);
    
    if (migrated == 0) {
        printf("\033[90mNo attachments found for '%s'\033[0m\n", old_path);
    } else {
        printf("\033[1;92m✓\033[0m Migrated %d attachment(s) from '%s' to '%s'\n", 
               migrated, old_path, new_path);
    }
    
    return migrated;
}

void show_note(const char *note_id) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), 
             "%s/gitnote/notes/%s.json", git_dir, note_id);
    
    FILE *f = fopen(note_path, "r");
    if (!f) {
        printf("Note not found: %s\n", note_id);
        return;
    }
    
    printf("Note: %s\n", note_id);
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    
    fclose(f);
}

int delete_note(const char *note_id) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), 
             "%s/gitnote/notes/%s.json", git_dir, note_id);
    
    if (remove(note_path) == 0) {
        printf("\033[90mDeleted note: %s\033[0m\n", note_id);
        return 0;
    } else {
        printf("Could not delete note: %s\n", note_id);
        return -1;
    }
}

int edit_note(const char *note_id, const char *new_title, const char *new_content) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), "%s/gitnote/notes/%s.json", git_dir, note_id);
    
    FILE *f = fopen(note_path, "r");
    if (!f) {
        fprintf(stderr, "\033[90mNote not found: %s\033[0m\n", note_id);
        return -1;
    }
    
    char line[512];
    char old_title[256] = "";
    char old_content[512] = "";
    char created[64] = "";
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"title\"")) {
            sscanf(line, "  \"title\": \"%[^\"]\"", old_title);
        }
        if (strstr(line, "\"content\"")) {
            sscanf(line, "  \"content\": \"%[^\"]\"", old_content);
        }
        if (strstr(line, "\"created_at\"")) {
            sscanf(line, "  \"created_at\": \"%[^\"]\"", created);
        }
    }
    fclose(f);
    
    f = fopen(note_path, "w");
    if (!f) {
        fprintf(stderr, "\033[90mCould not update note.\033[0m\n");
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"id\": \"%s\",\n", note_id);
    fprintf(f, "  \"title\": \"%s\",\n", new_title ? new_title : old_title);
    fprintf(f, "  \"content\": \"%s\",\n", new_content ? new_content : old_content);
    fprintf(f, "  \"created_at\": \"%s\"\n", created);
    fprintf(f, "}\n");
    
    fclose(f);
    printf("\033[90mUpdated note: %s\033[0m\n", note_id);
    return 0;
}

void search_notes(const char *query) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char notes_dir[512];
    snprintf(notes_dir, sizeof(notes_dir), "%s/gitnote/notes", git_dir);
    
    DIR *dir = opendir(notes_dir);
    if (!dir) {
        printf("\033[90mNo notes found.\033[0m\n");
        return;
    }
    
    printf("\033[90mSearching for '%s'...\033[0m\n", query);
    int found = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".json")) continue;
        
        char note_path[512];
        snprintf(note_path, sizeof(note_path), "%s/%s", notes_dir, entry->d_name);
        
        FILE *f = fopen(note_path, "r");
        if (!f) continue;
        
        char line[512];
        char id[64] = "";
        char title[256] = "";
        char content[512] = "";
        
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "\"id\"")) {
                sscanf(line, "  \"id\": \"%[^\"]\"", id);
            }
            if (strstr(line, "\"title\"")) {
                sscanf(line, "  \"title\": \"%[^\"]\"", title);
            }
            if (strstr(line, "\"content\"")) {
                sscanf(line, "  \"content\": \"%[^\"]\"", content);
            }
        }
        fclose(f);
        
        if (strcasestr(title, query) || strcasestr(content, query)) {
            printf("  \033[1;92m[%s]\033[0m %s\n", id, title);
            if (strcasestr(content, query)) {
                printf("    \033[90m...%s...\033[0m\n", content);
            }
            found++;
        }
    }
    closedir(dir);
    
    if (found == 0) {
        printf("  \033[90mNo results\033[0m\n");
    } else {
        printf("\033[90mFound %d notes\033[0m\n", found);
    }
}

int attach_note_to_target(const char *note_id, const char *target_type, const char *target_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), "%s/gitnote/notes/%s.json", git_dir, note_id);
    FILE *test = fopen(note_path, "r");
    if (!test) {
        fprintf(stderr, "\033[90mNote not found: %s\033[0m\n", note_id);
        return -1;
    }
    fclose(test);
    
    // Use nanoseconds to avoid collisions
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    char attach_path[512];
    snprintf(attach_path, sizeof(attach_path), 
             "%s/gitnote/metadata/attach_%ld_%09ld.json", git_dir, ts.tv_sec, ts.tv_nsec);
    
    FILE *f = fopen(attach_path, "w");
    if (!f) {
        fprintf(stderr, "\033[90mCould not create attachment.\033[0m\n");
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"note_id\": \"%s\",\n", note_id);
    fprintf(f, "  \"target_type\": \"%s\",\n", target_type);
    fprintf(f, "  \"target_path\": \"%s\"\n", target_path);
    fprintf(f, "}\n");
    
    fclose(f);
    printf("\033[90mAttached note %s to %s: %s\033[0m\n", note_id, target_type, target_path);
    return 0;
}

void show_target_notes(const char *target_type, const char *target_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/gitnote/metadata", git_dir);
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        printf("\033[90mNo attachments found.\033[0m\n");
        return;
    }
    
    printf("\033[90mNotes for %s '%s':\033[0m\n", target_type, target_path);
    int found = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, "attach_") && strstr(entry->d_name, ".json")) {
            char attach_path[512];
            snprintf(attach_path, sizeof(attach_path), "%s/%s", metadata_dir, entry->d_name);
            
            FILE *f = fopen(attach_path, "r");
            if (f) {
                char line[512];
                char note_id[128] = "";
                char ttype[64] = "";
                char tpath[256] = "";
                
                while (fgets(line, sizeof(line), f)) {
                    if (strstr(line, "\"note_id\"")) {
                        sscanf(line, "  \"note_id\": \"%[^\"]\"", note_id);
                    }
                    if (strstr(line, "\"target_type\"")) {
                        sscanf(line, "  \"target_type\": \"%[^\"]\"", ttype);
                    }
                    if (strstr(line, "\"target_path\"")) {
                        sscanf(line, "  \"target_path\": \"%[^\"]\"", tpath);
                    }
                }
                fclose(f);
                
                if (strcmp(ttype, target_type) == 0 && strcmp(tpath, target_path) == 0) {
                    char note_path[512];
                    snprintf(note_path, sizeof(note_path), "%s/gitnote/notes/%s.json", git_dir, note_id);
                    
                    FILE *nf = fopen(note_path, "r");
                    if (nf) {
                        char ntitle[256] = "";
                        char ncontent[512] = "";
                        char created[64] = "";
                        
                        while (fgets(line, sizeof(line), nf)) {
                            if (strstr(line, "\"title\"")) {
                                sscanf(line, "  \"title\": \"%[^\"]\"", ntitle);
                            }
                            if (strstr(line, "\"content\"")) {
                                sscanf(line, "  \"content\": \"%[^\"]\"", ncontent);
                            }
                            if (strstr(line, "\"created_at\"")) {
                                sscanf(line, "  \"created_at\": \"%[^\"]\"", created);
                            }
                        }
                        fclose(nf);
                        
                        printf("\n  \033[1;92m[%s] %s\033[0m\n", note_id, ntitle);
                        printf("  \033[90mCreated: %s\033[0m\n", created);
                        printf("  \033[97m%s\033[0m\n", ncontent);
                        found = 1;
                    }
                }
            }
        }
    }
    
    closedir(dir);
    
    if (!found) {
        printf("  \033[90m(no notes attached)\033[0m\n");
    }
}

void show_directory_notes_recursive(const char *dir_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    printf("\n\033[90m=== Notes in directory '%s' (recursive) ===\033[0m\n", dir_path);
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/gitnote/metadata", git_dir);
    
    DIR *mdir = opendir(metadata_dir);
    if (mdir) {
        struct dirent *mentry;
        int dir_has_notes = 0;
        while ((mentry = readdir(mdir)) != NULL) {
            if (strstr(mentry->d_name, "attach_") && strstr(mentry->d_name, ".json")) {
                char attach_path[512];
                snprintf(attach_path, sizeof(attach_path), "%s/%s", metadata_dir, mentry->d_name);
                
                FILE *f = fopen(attach_path, "r");
                if (f) {
                    char line[512];
                    char ttype[64] = "";
                    char tpath[256] = "";
                    
                    while (fgets(line, sizeof(line), f)) {
                        if (strstr(line, "\"target_type\"")) {
                            sscanf(line, "  \"target_type\": \"%[^\"]\"", ttype);
                        }
                        if (strstr(line, "\"target_path\"")) {
                            sscanf(line, "  \"target_path\": \"%[^\"]\"", tpath);
                        }
                    }
                    fclose(f);
                    
                    if (strcmp(ttype, "dir") == 0 && strcmp(tpath, dir_path) == 0) {
                        dir_has_notes = 1;
                        break;
                    }
                }
            }
        }
        closedir(mdir);
        
        if (dir_has_notes) {
            show_target_notes("dir", dir_path);
        }
    }
    
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || 
            strcmp(entry->d_name, ".git") == 0 || strcmp(entry->d_name, ".gitnote") == 0)
            continue;
        
        char full_path[512];
        int dir_len = strlen(dir_path);
        if (dir_path[dir_len - 1] == '/') {
            snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                show_directory_notes_recursive(full_path);
            } else if (S_ISREG(st.st_mode)) {
                DIR *check_dir = opendir(metadata_dir);
                if (!check_dir) continue;
                
                int found_attachment = 0;
                struct dirent *check_entry;
                
                while ((check_entry = readdir(check_dir)) != NULL) {
                    if (strstr(check_entry->d_name, "attach_") && strstr(check_entry->d_name, ".json")) {
                        char attach_file[512];
                        snprintf(attach_file, sizeof(attach_file), "%s/%s", metadata_dir, check_entry->d_name);
                        
                        FILE *af = fopen(attach_file, "r");
                        if (af) {
                            char line[512];
                            char atype[64] = "";
                            char apath[256] = "";
                            
                            while (fgets(line, sizeof(line), af)) {
                                if (strstr(line, "\"target_type\"")) {
                                    sscanf(line, "  \"target_type\": \"%[^\"]\"", atype);
                                }
                                if (strstr(line, "\"target_path\"")) {
                                    sscanf(line, "  \"target_path\": \"%[^\"]\"", apath);
                                }
                            }
                            fclose(af);
                            
                            if (strcmp(atype, "file") == 0 && strcmp(apath, full_path) == 0) {
                                found_attachment = 1;
                                break;
                            }
                        }
                    }
                }
                closedir(check_dir);
                
                if (found_attachment) {
                    show_target_notes("file", full_path);
                }
            }
        }
    }
    
    closedir(dir);
}