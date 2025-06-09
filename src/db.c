#include "db.h"
#include "models.h"
#include "utils.h"

sqlite3 *DB_INSTANCE = 0;

sqlite3 *db_get_instance(void);
int db_run_sql(char *sql);
char *db_map_error(int code);

result *db_create_user(user *u) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "INSERT INTO users(username, password, created_at) "
              "VALUES(?, ?, CURRENT_TIMESTAMP);\0";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  sqlite3_bind_text(stmt, 1, u->username, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, u->password, -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  u->id = sqlite3_last_insert_rowid(db_get_instance());

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_ERROR));
  }

  return new_result(u, NULL);
}

result *db_get_user(const char *username) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "SELECT id, username, password, created_at "
              "FROM users "
              "WHERE username = ? "
              "LIMIT 1;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_NOTFOUND) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_NOT_FOUND));
  }
  if (rc != SQLITE_OK && rc != SQLITE_ROW) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  int id = sqlite3_column_int(stmt, 0);
  const char *fetched_username = (const char *)sqlite3_column_text(stmt, 1);
  const char *password = (const char *)sqlite3_column_text(stmt, 2);
  time_t created_at = sqlite3_column_int64(stmt, 3);

  user *u = (user *)malloc(sizeof(user));
  u->id = id;
  u->username = strdup(fetched_username);
  u->password = strdup(password);
  u->created_at = created_at;
  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result((void *)u, NULL);
}

result *db_update_user_password(int id, char *new_password) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "UPDATE users SET password=? WHERE id = ?;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_text(stmt, 1, new_password, -1, SQLITE_TRANSIENT);
  rc = sqlite3_bind_int(stmt, 2, id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result(NULL, NULL);
}

result *db_delete_user(int id) {
  if (id <= 0) {
    return new_result(NULL, db_map_error(DB_NOT_FOUND));
  }

  int rc;
  sqlite3_stmt *stmt;
  char *sql = "DELETE FROM users WHERE id=?;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_int(stmt, 1, id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return db_delete_todos_for_user(id);
}

result *db_create_todo(todo *t) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "INSERT INTO todos(title, done, user_id, created_at) "
              "VALUES(?, ?, ?, CURRENT_TIMESTAMP);\0";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  sqlite3_bind_text(stmt, 1, t->title, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, false);
  sqlite3_bind_int(stmt, 3, t->user_id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  t->id = sqlite3_last_insert_rowid(db_get_instance());

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_ERROR));
  }

  return new_result(t, NULL);
}

result *db_get_todos_for_user(int user_id, size_t *out_todos_count) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "SELECT id, title, done, created_at "
              "FROM todos "
              "WHERE user_id = ? "
              "ORDER BY done ASC, created_at DESC;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  sqlite3_bind_int(stmt, 1, user_id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_NOT_FOUND));
  }
  todo *todos = NULL;
  int todos_count = 0;
  int todos_capacity = 10;
  todos = (todo *)malloc(sizeof(todo) * todos_capacity);
  if (todos == NULL) {
    sqlite3_finalize(stmt);
    return new_result(NULL, db_map_error(DB_ERROR));
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (rc == SQLITE_NOTFOUND) {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
      return new_result(NULL, db_map_error(DB_NOT_FOUND));
    }
    if (todos_count >= todos_capacity) {
      todos_capacity *= 2;
      todos = (todo *)realloc(todos, sizeof(todo) * todos_capacity);
      if (todos == NULL) {
        fprintf(stderr, "Memory reallocation failed\n");
        sqlite3_finalize(stmt);
        return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
      }
    }

    int id = sqlite3_column_int(stmt, 0);
    const char *title = (const char *)sqlite3_column_text(stmt, 1);
    int done = sqlite3_column_int(stmt, 2);
    time_t created_at = sqlite3_column_int64(stmt, 3);

    todos[todos_count].id = id;
    todos[todos_count].title = strdup(title);
    todos[todos_count].done = done == 1;
    todos[todos_count].created_at = created_at;
    todos_count++;
  }

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Error fetching data: %s\n",
            sqlite3_errmsg(db_get_instance()));
    sqlite3_finalize(stmt);
    return new_result(NULL, db_map_error(DB_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  *out_todos_count = todos_count;
  return new_result(todos, NULL);
}

result *db_toggle_todo_done(int id, int user_id) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "UPDATE todos "
              "SET done = NOT done "
              "WHERE id = ? AND user_id = ? "
              "RETURNING title, done, created_at;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_int(stmt, 1, id);
  rc = sqlite3_bind_int(stmt, 2, user_id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  todo *t = NULL;
  switch (rc) {
  case SQLITE_DONE:
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    break;
  case SQLITE_ROW:
    char *title = (char *)sqlite3_column_text(stmt, 0);
    int done = sqlite3_column_int(stmt, 1);
    time_t created_at = sqlite3_column_int64(stmt, 2);

    t = new_todo(user_id, title);
    t->id = id;
    t->done = (bool)done;
    t->title = strdup(title);
    t->created_at = created_at;
    break;
  default:
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result(t, NULL);
}

result *db_delete_todo(int id, int user_id) {
  int rc;
  sqlite3_stmt *stmt;
  char *sql = "DELETE FROM todos WHERE id = ? AND user_id = ?;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_int(stmt, 1, id);
  rc = sqlite3_bind_int(stmt, 2, user_id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result(NULL, NULL);
}

result *db_delete_todos_for_user(int user_id) {
  if (user_id <= 0) {
    return new_result(NULL, db_map_error(DB_NOT_FOUND));
  }

  int rc;
  sqlite3_stmt *stmt;
  char *sql = "DELETE FROM todos WHERE user_id=?;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_int(stmt, 1, user_id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result(NULL, NULL);
}

result *db_delete_finished_todos_for_user(int user_id) {
  if (user_id <= 0) {
    return new_result(NULL, db_map_error(DB_NOT_FOUND));
  }

  int rc;
  sqlite3_stmt *stmt;
  char *sql = "DELETE FROM todos WHERE user_id=? AND done=1;";

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_bind_int(stmt, 1, user_id);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return new_result(NULL, db_map_error(DB_STATEMENT_ERROR));
  }

  return new_result(NULL, NULL);
}

result *db_init_tables(void) {
  char *users_table = "CREATE TABLE IF NOT EXISTS users("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "username VARCHAR(255) UNIQUE NOT NULL,"
                      "password TEXT NOT NULL,"
                      "created_at TIMESTAMP);";
  if (db_run_sql(users_table) != DB_OK) {
    return new_result(NULL, db_map_error(DB_INIT_ERROR));
  }

  char *todos_table =
      "CREATE TABLE IF NOT EXISTS todos("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "title TEXT NOT NULL,"
      "done INTEGER(1),"
      "user_id INTEGER,"
      "created_at TIMESTAMP,"
      "FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE"
      ");";
  if (db_run_sql(todos_table) != DB_OK) {
    return new_result(NULL, db_map_error(DB_INIT_ERROR));
  }

  return new_result(NULL, NULL);
}

sqlite3 *db_get_instance(void) {
  if (DB_INSTANCE == 0) {
    char *sqlite3_file_path = getenv("SQLITE3_PATH");
    if (sqlite3_file_path == 0) {
      sqlite3_file_path = "./danktodo.db";
    }

    if (sqlite3_open(sqlite3_file_path, &DB_INSTANCE) != SQLITE_OK) {
      fprintf(stderr, "Cannot open database: %s\n",
              sqlite3_errmsg(DB_INSTANCE));
      sqlite3_close(DB_INSTANCE);
      exit(1);
    }
  }

  return DB_INSTANCE;
}

int db_run_sql(char *sql) {
  int rc;
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(db_get_instance(), sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %d, error_msg: %s\n", rc,
            sqlite3_errmsg(db_get_instance()));
    return DB_ERROR;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return DB_ERROR;
  }

  rc = sqlite3_finalize(stmt);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db_get_instance()));
    return DB_ERROR;
  }

  return DB_OK;
}

char *db_map_error(int code) {
  switch (code) {
  case DB_ERROR:
    return "internal database error";
  case DB_INIT_ERROR:
    return "init table error";
  case DB_INVALID_ATTRIBUTE:
    return "invalid attribute for column passed";
  case DB_NOT_FOUND:
    return "record not found";
  case DB_STATEMENT_ERROR:
    return "statement error";
  case DB_OK:
  default:
    return "something really fucked up happened";
  }
}
