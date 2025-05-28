#ifndef _DB
#define _DB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#include "./models.h"
#include "./utils.h"

#define DB_OK 0
#define DB_ERROR 1
#define DB_INIT_ERROR 2
#define DB_INVALID_ATTRIBUTE 3
#define DB_NOT_FOUND 4
#define DB_STATEMENT_ERROR 5

result *db_init_tables(void);

result *db_create_user(user *u);
result *db_get_user(const char *username);
result *db_update_user_password(int id, char *new_password);
result *db_delete_user(int id);

result *db_create_todo(todo *t);
result *db_get_todos_for_user(int user_id, size_t *out_todos_count);
result *db_toggle_todo_done(int idi, int user_id);
result *db_delete_todo(int id, int user_id);
result *db_delete_todos_for_user(int user_id);
result *db_delete_finished_todos_for_user(int user_id);

#endif // !_DB
