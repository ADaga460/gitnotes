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