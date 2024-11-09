#ifndef _MODELS
#define _MODELS

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

typedef struct {
  int id;
  char *username;
  char *password;
  time_t created_at;
} user;

typedef struct {
  int id;
  char *title;
  bool done;
  int user_id;
  time_t created_at;
} todo;

user *new_user(char *username, char *password);
void free_user(user *u);
todo *new_todo(int user_id, char *title);
void free_todo(todo *t);

#endif // !_MODELS
