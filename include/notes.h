#ifndef NOTES_H
#define NOTES_H

int add_note(const char *title, const char *content);

void list_notes(void);

void show_note(const char *note_id);

int delete_note(const char *note_id);

#endif