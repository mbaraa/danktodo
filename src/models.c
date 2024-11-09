#include "models.h"

user *new_user(char *username, char *password) {
  user *u = (user *)malloc(sizeof(user));
  u->username = username;
  u->password = hash_password(password);
  u->created_at = time(NULL);

  return u;
}

void free_user(user *u) {
  safe_free(u->username);
  safe_free(u->password);
  safe_free(u);
}

todo *new_todo(int user_id, char *title) {
  todo *t = (todo *)malloc(sizeof(todo));
  t->title = title;
  t->user_id = user_id;
  t->done = false;
  t->created_at = time(NULL);

  return t;
}

void free_todo(todo *t) {
  safe_free(t->title);
  safe_free(t);
}
