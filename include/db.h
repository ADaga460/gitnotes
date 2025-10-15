#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Open or create the database file, return handle
sqlite3* init_db(void);

// Close database safely
void close_db(sqlite3 *db);

// Execute arbitrary SQL with no return data (CREATE, INSERT, UPDATE)
int exec_query(sqlite3 *db, const char *query);

#endif
