#ifndef NOTES_H
#define NOTES_H

int add_note(const char *title, const char *content);

void list_notes(void);

void show_note(const char *note_id);

int delete_note(const char *note_id);

int attach_note_to_target(const char *note_id, const char *target_type, const char *target_path);

void show_target_notes(const char *target_type, const char *target_path);

void show_directory_notes_recursive(const char *dir_path);

int edit_note(const char *note_id, const char *new_title, const char *new_content);

void search_notes(const char *query);

int migrate_attachment(const char *old_path, const char *new_path);

#endif