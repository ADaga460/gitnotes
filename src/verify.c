#include "verify.h"
#include "git_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static int check_note_exists(const char *note_id, const char *git_dir) {
    char note_path[512];
    snprintf(note_path, sizeof(note_path), "%s/clisuite/notes/%s.json", git_dir, note_id);
    
    struct stat st;
    return (stat(note_path, &st) == 0);
}

int verify_clisuite(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "\033[90mNot in a git repository.\033[0m\n");
        return -1;
    }
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/clisuite/metadata", git_dir);
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        printf("\033[90mNo metadata to verify.\033[0m\n");
        return 0;
    }
    
    int total = 0;
    int orphaned = 0;
    
    printf("\033[90mVerifying clisuite data...\033[0m\n");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".json")) continue;
        
        char attach_path[512];
        snprintf(attach_path, sizeof(attach_path), "%s/%s", metadata_dir, entry->d_name);
        
        FILE *f = fopen(attach_path, "r");
        if (!f) continue;
        
        char line[512];
        char note_id[128] = "";
        
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "\"note_id\"")) {
                sscanf(line, "  \"note_id\": \"%[^\"]\"", note_id);
                break;
            }
        }
        fclose(f);
        
        if (strlen(note_id) > 0) {
            total++;
            if (!check_note_exists(note_id, git_dir)) {
                printf("  \033[1;91m✗\033[0m Orphaned attachment: %s → %s (note missing)\n", 
                       entry->d_name, note_id);
                orphaned++;
            }
        }
    }
    closedir(dir);
    
    if (orphaned == 0) {
        printf("  \033[1;92m✓\033[0m All %d attachments valid\n", total);
    } else {
        printf("  \033[1;91m%d orphaned attachments found\033[0m\n", orphaned);
        printf("  Run \033[97mclisuite repair\033[0m to clean up\n");
    }
    
    return orphaned;
}

int repair_clisuite(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char metadata_dir[512];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/clisuite/metadata", git_dir);
    
    DIR *dir = opendir(metadata_dir);
    if (!dir) {
        printf("\033[90mNo metadata to repair.\033[0m\n");
        return 0;
    }
    
    int removed = 0;
    
    printf("\033[90mRepairing clisuite data...\033[0m\n");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strstr(entry->d_name, ".json")) continue;
        
        char attach_path[512];
        snprintf(attach_path, sizeof(attach_path), "%s/%s", metadata_dir, entry->d_name);
        
        FILE *f = fopen(attach_path, "r");
        if (!f) continue;
        
        char line[512];
        char note_id[128] = "";
        
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "\"note_id\"")) {
                sscanf(line, "  \"note_id\": \"%[^\"]\"", note_id);
                break;
            }
        }
        fclose(f);
        
        if (strlen(note_id) > 0 && !check_note_exists(note_id, git_dir)) {
            if (remove(attach_path) == 0) {
                printf("  \033[90mRemoved orphaned attachment: %s\033[0m\n", entry->d_name);
                removed++;
            }
        }
    }
    closedir(dir);
    
    if (removed == 0) {
        printf("  \033[1;92m✓\033[0m Nothing to repair\n");
    } else {
        printf("  \033[1;92m✓\033[0m Removed %d orphaned attachments\n", removed);
    }
    
    return 0;
}