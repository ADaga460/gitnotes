#ifndef GIT_INTEGRATION_H
#define GIT_INTEGRATION_H

#include <stdbool.h>

bool is_git_repo(void);

char* get_git_dir(void);

int init_git_structure(void);

char* get_current_commit(void);

int attach_to_commit(const char *commit_hash, const char *note_id, const char *todo_id);

void show_commit_metadata(const char *commit_hash);

int install_hooks(void);

#endif