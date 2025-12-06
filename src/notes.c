#include "notes.h"
#include "git_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static char* generate_note_id(void) {
    static char id[32];
    snprintf(id, sizeof(id), "note_%ld", time(NULL));
    return id;
}

int add_note(const char *title, const char *content) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "Not in a git repository.\n");
        return -1;
    }
    
    char *note_id = generate_note_id();
    char note_path[512];
    snprintf(note_path, sizeof(note_path), 
             "%s/clisuite/notes/%s.json", git_dir, note_id);
    
    FILE *f = fopen(note_path, "w");
    if (!f) {
        fprintf(stderr, "Could not create note file.\n");
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
    printf("Note created: %s\n", note_id);
    return 0;
}

void list_notes(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "Not in a git repository.\n");
        return;
    }
    
    char notes_dir[512];
    snprintf(notes_dir, sizeof(notes_dir), "%s/clisuite/notes", git_dir);
    
    DIR *dir = opendir(notes_dir);
    if (!dir) {
        printf("No notes found.\n");
        return;
    }
    
    printf("Notes:\n");
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
                printf("  [%s] %s\n", id, title);
                fclose(f);
            }
        }
    }
    closedir(dir);
}

void show_note(const char *note_id) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), 
             "%s/clisuite/notes/%s.json", git_dir, note_id);
    
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
             "%s/clisuite/notes/%s.json", git_dir, note_id);
    
    if (remove(note_path) == 0) {
        printf("Deleted note: %s\n", note_id);
        return 0;
    } else {
        printf("Could not delete note: %s\n", note_id);
        return -1;
    }
}

int attach_note_to_target(const char *note_id, const char *target_type, const char *target_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char note_path[512];
    snprintf(note_path, sizeof(note_path), "%s/clisuite/notes/%s.json", git_dir, note_id);
    FILE *test = fopen(note_path, "r");
    if (!test) {
        fprintf(stderr, "Note not found: %s\n", note_id);
        return -1;
    }
    fclose(test);
    
    char attach_path[512];
    snprintf(attach_path, sizeof(attach_path), 
             "%s/clisuite/metadata/attach_%ld.json", git_dir, time(NULL));
    
    FILE *f = fopen(attach_path, "w");
    if (!f) {
        fprintf(stderr, "Could not create attachment.\n");
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"note_id\": \"%s\",\n", note_id);
    fprintf(f, "  \"target_type\": \"%s\",\n", target_type);
    fprintf(f, "  \"target_path\": \"%s\"\n", target_path);
    fprintf(f, "}\n");
    
    fclose(f);
    printf("Attached note %s to %s: %s\n", note_id, target_type, target_path);
    return 0;
}

void show_target_notes(const char *target_type, const char *target_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/clisuite/metadata", git_dir);
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        printf("No attachments found.\n");
        return;
    }
    
    printf("Notes for %s '%s':\n", target_type, target_path);
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
                    snprintf(note_path, sizeof(note_path), "%s/clisuite/notes/%s.json", git_dir, note_id);
                    
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
                        
                        printf("\n  [%s] %s\n", note_id, ntitle);
                        printf("  Created: %s\n", created);
                        printf("  %s\n", ncontent);
                        found = 1;
                    }
                }
            }
        }
    }
    
    closedir(dir);
    
    if (!found) {
        printf("  (no notes attached)\n");
    }
}

void show_directory_notes_recursive(const char *dir_path) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    printf("\n=== Notes in directory '%s' (recursive) ===\n", dir_path);
    
    show_target_notes("dir", dir_path);
    
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                show_directory_notes_recursive(full_path);
            } else if (S_ISREG(st.st_mode)) {
                char metadata_dir[512];
                snprintf(metadata_dir, sizeof(metadata_dir), "%s/clisuite/metadata", git_dir);
                
                DIR *mdir = opendir(metadata_dir);
                if (!mdir) continue;
                
                int has_notes = 0;
                struct dirent *mentry;
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
                            
                            if (strcmp(ttype, "file") == 0 && strcmp(tpath, full_path) == 0) {
                                has_notes = 1;
                                break;
                            }
                        }
                    }
                }
                closedir(mdir);
                
                if (has_notes) {
                    show_target_notes("file", full_path);
                }
            }
        }
    }
    
    closedir(dir);
}