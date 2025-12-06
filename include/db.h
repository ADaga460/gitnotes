#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3* init_db(void);

void close_db(sqlite3 *db);

int exec_query(sqlite3 *db, const char *query);

#endif
