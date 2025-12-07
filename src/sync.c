#include "sync.h"
#include "git_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static void copy_file(const char *src, const char *dest) {
    FILE *source = fopen(src, "rb");
    if (!source) return;
    
    FILE *target = fopen(dest, "wb");
    if (!target) {
        fclose(source);
        return;
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, target);
    }
    
    fclose(source);
    fclose(target);
}

static void copy_directory(const char *src_dir, const char *dest_dir) {
    mkdir(dest_dir, 0755);
    
    DIR *dir = opendir(src_dir);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char src_path[512];
        char dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);
        
        struct stat st;
        if (stat(src_path, &st) == 0 && S_ISREG(st.st_mode)) {
            copy_file(src_path, dest_path);
        }
    }
    
    closedir(dir);
}

int sync_to_tracked(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    mkdir(".gitnote", 0755);
    mkdir(".gitnote/notes", 0755);
    mkdir(".gitnote/metadata", 0755);
    
    char local_notes[512], local_metadata[512];
    snprintf(local_notes, sizeof(local_notes), "%s/gitnote/notes", git_dir);
    snprintf(local_metadata, sizeof(local_metadata), "%s/gitnote/metadata", git_dir);
    
    copy_directory(local_notes, ".gitnote/notes");
    copy_directory(local_metadata, ".gitnote/metadata");
    
    printf("Synced metadata to .gitnote/ (ready to commit)\n");
    printf("Run: git add .gitnote && git commit -m 'Update metadata'\n");
    return 0;
}

int sync_from_tracked(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    struct stat st;
    if (stat(".gitnote", &st) != 0) {
        printf("No tracked metadata found (.gitnote/ doesn't exist)\n");
        return 0;
    }
    
    char local_notes[512], local_metadata[512];
    snprintf(local_notes, sizeof(local_notes), "%s/gitnote/notes", git_dir);
    snprintf(local_metadata, sizeof(local_metadata), "%s/gitnote/metadata", git_dir);
    
    copy_directory(".gitnote/notes", local_notes);
    copy_directory(".gitnote/metadata", local_metadata);
    
    printf("Synced remote metadata from .gitnote/\n");
    
    show_current_commit_notes();
    
    return 0;
}

int install_sync_hooks(void) {
    char *git_dir = get_git_dir();
    if (!git_dir) return -1;
    
    char hook_path[512];
    
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/pre-push", git_dir);
    FILE *f = fopen(hook_path, "w");
    if (f) {
        fprintf(f, "#!/bin/sh\n");
        fprintf(f, "# gitnote pre-push hook\n");
        fprintf(f, "gitnote sync\n");
        fprintf(f, "if [ -d .gitnote ]; then\n");
        fprintf(f, "  git add .gitnote/ 2>/dev/null || true\n");
        fprintf(f, "  git commit --amend --no-edit --no-verify 2>/dev/null || true\n");
        fprintf(f, "fi\n");
        fclose(f);
        chmod(hook_path, 0755);
        printf("Installed pre-push hook\n");
    }
    
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/post-merge", git_dir);
    f = fopen(hook_path, "w");
    if (f) {
        fprintf(f, "#!/bin/sh\n");
        fprintf(f, "# gitnote post-merge hook\n");
        fprintf(f, "gitnote pull\n");
        fclose(f);
        chmod(hook_path, 0755);
        printf("Installed post-merge hook\n");
    }
    
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/post-checkout", git_dir);
    f = fopen(hook_path, "w");
    if (f) {
        fprintf(f, "#!/bin/sh\n");
        fprintf(f, "# gitnote post-checkout hook\n");
        fprintf(f, "gitnote pull\n");
        fclose(f);
        chmod(hook_path, 0755);
        printf("Installed post-checkout hook\n");
    }
    
    return 0;
}

static void remove_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                remove_directory(full_path);
            } else {
                remove(full_path);
            }
        }
    }
    
    closedir(dir);
    rmdir(path);
}

int reset_gitnote(int tracked_only) {
    char *git_dir = get_git_dir();
    
    if (tracked_only) {
        remove_directory(".gitnote");
        printf("Removed .gitnote/ (tracked metadata)\n");
        printf("Run: git add .gitnote && git commit -m 'Remove metadata'\n");
        return 0;
    }
    
    if (git_dir) {
        char local_path[512];
        snprintf(local_path, sizeof(local_path), "%s/gitnote", git_dir);
        remove_directory(local_path);
        printf("Removed .git/gitnote/ (local data)\n");
    }
    
    remove_directory(".gitnote");
    printf("Removed .gitnote/ (tracked metadata)\n");
    printf("All gitnote data has been erased.\n");
    
    return 0;
}