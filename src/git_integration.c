#include "../include/git_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool is_git_repo(void) {
    struct stat st;
    return (stat(".git", &st) == 0);
}

char* get_git_dir(void) {
    static char git_dir[512];
    
    struct stat st;
    if (stat(".git", &st) != 0) {
        return NULL;
    }
    
    if (S_ISDIR(st.st_mode)) {
        strcpy(git_dir, ".git");
    } else {
        FILE *f = fopen(".git", "r");
        if (!f) return NULL;
        
        char line[512];
        if (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "gitdir: ", 8) == 0) {
                strcpy(git_dir, line + 8);
                git_dir[strcspn(git_dir, "\n")] = 0;
            }
        }
        fclose(f);
    }
    
    return git_dir;
}

int init_git_structure(void) {
    if (!is_git_repo()) {
        fprintf(stderr, "Not a git repository. Run 'git init' first.\n");
        return -1;
    }
    
    char *git_dir = get_git_dir();
    if (!git_dir) {
        fprintf(stderr, "Could not determine git directory.\n");
        return -1;
    }
    
    char path[512];
    
    snprintf(path, sizeof(path), "%s/clisuite", git_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/clisuite/notes", git_dir);
    mkdir(path, 0755);
    
    snprintf(path, sizeof(path), "%s/clisuite/metadata", git_dir);
    mkdir(path, 0755);
    
    printf("Initialized clisuite in %s/clisuite/\n", git_dir);
    return 0;
}

char* get_current_commit(void) {
    static char commit[41];
    FILE *fp = popen("git rev-parse HEAD 2>/dev/null", "r");
    if (!fp) return NULL;
    
    if (fgets(commit, sizeof(commit), fp)) {
        commit[strcspn(commit, "\n")] = 0;
        pclose(fp);
        return commit;
    }
    
    pclose(fp);
    return NULL;
}

int attach_to_commit(const char *commit_hash, const char *note_id, const char *todo_id) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char metadata_path[512];
    snprintf(metadata_path, sizeof(metadata_path), 
             "%s/clisuite/metadata/%s.json", git_dir, commit_hash);
    
    FILE *f = fopen(metadata_path, "w");
    if (!f) {
        fprintf(stderr, "Could not create metadata file.\n");
        return -1;
    }
    
    fprintf(f, "{\n");
    fprintf(f, "  \"commit\": \"%s\",\n", commit_hash);
    if (note_id) {
        fprintf(f, "  \"note_id\": \"%s\",\n", note_id);
    }
    if (todo_id) {
        fprintf(f, "  \"todo_id\": \"%s\"\n", todo_id);
    }
    fprintf(f, "}\n");
    
    fclose(f);
    printf("Attached metadata to commit %s\n", commit_hash);
    return 0;
}

void show_commit_metadata(const char *commit_hash) {
    char *git_dir = get_git_dir();
    if (!git_dir) return;
    
    char metadata_path[512];
    snprintf(metadata_path, sizeof(metadata_path), 
             "%s/clisuite/metadata/%s.json", git_dir, commit_hash);
    
    FILE *f = fopen(metadata_path, "r");
    if (!f) {
        printf("No metadata for commit %s\n", commit_hash);
        return;
    }
    
    printf("Metadata for commit %s:\n", commit_hash);
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("  %s", line);
    }
    
    fclose(f);
}

int install_hooks(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char hook_path[512];
    
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/post-commit", git_dir);
    FILE *f = fopen(hook_path, "w");
    if (f) {
        fprintf(f, "#!/bin/sh\n");
        fprintf(f, "# clisuite post-commit hook\n");
        fprintf(f, "clisuite hook post-commit\n");
        fclose(f);
        chmod(hook_path, 0755);
        printf("Installed post-commit hook\n");
    }
    
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/post-merge", git_dir);
    f = fopen(hook_path, "w");
    if (f) {
        fprintf(f, "#!/bin/sh\n");
        fprintf(f, "# clisuite post-merge hook\n");
        fprintf(f, "clisuite hook post-merge\n");
        fclose(f);
        chmod(hook_path, 0755);
        printf("Installed post-merge hook\n");
    }
    
    return 0;
}