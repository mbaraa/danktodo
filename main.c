#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <ctemplate/buffer.h>
#include <ctemplate/ctemplate.h>
#include <ulfius.h>

#include "src/db.h"
#include "src/handlers_helpers.h"
#include "src/middlewares.h"
#include "src/models.h"
#include "src/utils.h"

#define PORT 8080

char *VERSION = "git-latest";

static volatile int app_running = true;

void sigint_handler(int _) {
  puts("\033[31mTODOOO OUT\033[0m");
  app_running = false;
}
void substr(char *s, int a, int b, char *t) { strncpy(t, s + a, b); }
void init() {
  puts("Todooo init");
  result *init_result = db_init_tables();
  if (result_is_error(init_result)) {
    printf("DB INIT FAILED: %s\n", init_result->error);
    free_result(init_result);
    exit(1);
  }
  free_result(init_result);

  init_jwt();

  char *version = getenv("VERSION");
  if (version != 0 && strlen(version) > 7) {
    VERSION = (char *)malloc(7);
    strncpy(VERSION, version, 7);
  }
  puts("Todooo init done");
}

int handle_dingus(const struct _u_request *req, struct _u_response *res,
                  void *user_data) {
  const char *url = "https://www.youtube.com/watch?v=dQw4w9WgXcQ";
  return handle_redirect(req, res, url);
}

int handle_logout(const struct _u_request *req, struct _u_response *res,
                  void *user_data) {
  const char *cookie = "token=dingus; Path=/; HttpOnly; Max-Age=-420";
  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    ulfius_set_response_properties(
        res, U_OPT_STATUS, 200, U_OPT_HEADER_PARAMETER, "set-cookie", cookie,
        U_OPT_HEADER_PARAMETER, "HX-Redirect", "/login", U_OPT_COOKIE_PARAMETER,
        "token", "dingus", U_OPT_NONE);
    return U_CALLBACK_CONTINUE;
  }

  return handle_redirect(req, res, "/login");
}

int handle_delete_user(const struct _u_request *req, struct _u_response *res,
                       void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  /* char *username = (char *)u_map_get(req->map_header, "username"); */

  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);
  result *r = db_delete_user(user_id);
  if (result_is_error(r)) {
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_CONTINUE;
  }
  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    ulfius_set_response_properties(res, U_OPT_STATUS, 200,
                                   U_OPT_HEADER_PARAMETER, "HX-Redirect",
                                   "/login", U_OPT_NONE);
    return U_CALLBACK_CONTINUE;
  }

  return handle_redirect(req, res, "/login");
}

int handle_add_todo(const struct _u_request *req, struct _u_response *res,
                    void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);
  if (user_id == 0) {
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }

  char *todo_title = (char *)u_map_get(req->map_post_body, "todo-title");
  if (todo_title == NULL) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }

  todo *t = new_todo(user_id, todo_title);
  result *new_todo_res = db_create_todo(t);
  if (result_is_error(new_todo_res)) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_CONTINUE;
  }

  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    buffer *new_todos_item =
        buffer_create(2560); // size of a single views/todooos_list_item.html
    int stat = render_template_to_buffer_ref(
        new_todos_item, "views/todooos_list_item.html",
        TMPL_make_var_list(
            6, "todo_id", itoa(((todo *)new_todo_res->value)->id), "todo_title",
            t->title, "todo_done", t->done ? "true" : "false"));

    if (stat != U_CALLBACK_CONTINUE) {
      buffer_destroy(new_todos_item);
      ulfius_set_string_body_response(res, 500, "Internal server error");
      return U_CALLBACK_ERROR;
    }

    ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_STRING_BODY,
                                   buffer_data(new_todos_item), U_OPT_NONE);
    return U_CALLBACK_CONTINUE;
  }

  return handle_redirect(req, res, "/");
}

int handle_delete_todo(const struct _u_request *req, struct _u_response *res,
                       void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);
  if (user_id == 0) {
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }

  char *todo_id_str = (char *)u_map_get(req->map_url, "id");
  if (todo_id_str == NULL) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }
  int todo_id = todo_id_str == NULL ? 0 : atoi(todo_id_str);

  result *del_todo_res = db_delete_todo(todo_id, user_id);
  if (result_is_error(del_todo_res)) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_CONTINUE;
  }

  ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_toggle_todo(const struct _u_request *req, struct _u_response *res,
                       void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);
  if (user_id == 0) {
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }

  char *todo_id_str = (char *)u_map_get(req->map_url, "id");
  if (todo_id_str == NULL) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }
  int todo_id = todo_id_str == NULL ? 0 : atoi(todo_id_str);

  result *toggle_todo_res = db_toggle_todo_done(todo_id, user_id);
  if (result_is_error(toggle_todo_res)) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_CONTINUE;
  }

  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  todo *t = (todo *)toggle_todo_res->value;
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    buffer *updated_todos_item =
        buffer_create(2560); // size of a single views/todooos_list_item.html
    int stat = render_template_to_buffer_ref(
        updated_todos_item, "views/todooos_list_item.html",
        TMPL_make_var_list(6, "todo_id", itoa(t->id), "todo_title", t->title,
                           "todo_done", t->done ? "true" : "false"));

    if (stat != U_CALLBACK_CONTINUE) {
      buffer_destroy(updated_todos_item);
      ulfius_set_string_body_response(res, 500, "Internal server error");
      return U_CALLBACK_ERROR;
    }

    ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_STRING_BODY,
                                   buffer_data(updated_todos_item), U_OPT_NONE);
    return U_CALLBACK_CONTINUE;
  }

  ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_delete_all_todos(const struct _u_request *req,
                            struct _u_response *res, void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);
  if (user_id == 0) {
    ulfius_set_string_body_response(res, 400, "Bad Request");
    return U_CALLBACK_CONTINUE;
  }

  result *del_todo_res = db_delete_todos_for_user(user_id);
  if (result_is_error(del_todo_res)) {
    // TODO: display error message instead
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_CONTINUE;
  }

  ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_delete_all_finished_todos(const struct _u_request *req,
                                     struct _u_response *res, void *user_data) {
  ulfius_set_response_properties(res, U_OPT_STATUS, 501, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_export_todos_as_json(const struct _u_request *req,
                                struct _u_response *res, void *user_data) {
  ulfius_set_response_properties(res, U_OPT_STATUS, 501, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_export_todos_as_csv(const struct _u_request *req,
                               struct _u_response *res, void *user_data) {
  ulfius_set_response_properties(res, U_OPT_STATUS, 501, U_OPT_NONE);
  return U_CALLBACK_CONTINUE;
}

int handle_login_page(const struct _u_request *req, struct _u_response *res,
                      void *user_data) {
  buffer *login_form = render_template_to_buffer(
      "views/login_form.html",
      TMPL_make_var_list(6, "error", "", "success", "", "username", ""));
  if (login_form == NULL) {
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  buffer *reg = render_template_to_buffer(
      "views/login.html",
      TMPL_make_var_list(2, "login_form", buffer_data(login_form)));
  if (reg == NULL) {
    buffer_destroy(login_form);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  int stat =
      render_template(req, res, "views/layout.html",
                      TMPL_make_var_list(6, "title", "Login", "main_content",
                                         buffer_data(reg), "version", VERSION));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(reg);
    buffer_destroy(login_form);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  buffer_destroy(reg);
  buffer_destroy(login_form);

  return U_CALLBACK_CONTINUE;
}

int handle_login_user(const struct _u_request *req, struct _u_response *res,
                      void *user_data) {
  char *username = (char *)u_map_get(req->map_post_body, "username");
  char *password = (char *)u_map_get(req->map_post_body, "password");
  char *remember_me = (char *)u_map_get(req->map_post_body, "remember-me");
  remember_me = remember_me == NULL ? "off" : remember_me;
  if (username == NULL || password == NULL) {
    buffer *error_message = render_error_message("invalid data");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }
  result *user_result = db_get_user(username);
  if (result_is_error(user_result)) {
    buffer *error_message =
        render_error_message("oopsie doopsie, something went wrong");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  user *u = (user *)user_result->value;
  if (u == NULL) {
    buffer *error_message = render_error_message("wrong username or password!");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  if (!compare_hashes(u->password, password)) {
    buffer *error_message = render_error_message("wrong username or password!");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  long long token_age = strcmp(remember_me, "on") == 0 ? 2592000 : 3600;
  pair jwt_payload[2] = {{"iss", "Todooo"}, {"sub", u->username}};
  result *token_result = encode_jwt(jwt_payload, time(NULL) + token_age);
  if (result_is_error(token_result)) {
    buffer *error_message =
        render_error_message("oopsie doopsie, something went wrong");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  size_t todos_count = 0;
  result *todos_result = db_get_todos_for_user(u->id, &todos_count);
  if (result_is_error(todos_result)) {
    buffer *error_message =
        render_error_message("oopsie doopsie, something went wrong");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }
  todo *todos = (todo *)todos_result->value;
  buffer *todos_items = buffer_create(
      2560 * todos_count); // size of a single views/todooos_list_item.html
  for (size_t i = 0; i < todos_count; i++) {
    int stat = render_template_to_buffer_ref(
        todos_items, "views/todooos_list_item.html",
        TMPL_make_var_list(6, "todo_id", itoa(todos[i].id), "todo_title",
                           todos[i].title, "todo_done",
                           todos[i].done ? "true" : "false"));

    if (stat != U_CALLBACK_CONTINUE) {
      free_result(token_result);
      free_result(user_result);
      buffer_destroy(todos_items);
      ulfius_set_string_body_response(res, 500, "Internal server error");
      return U_CALLBACK_ERROR;
    }
  }

  buffer *todos_list = buffer_create(buffer_size(todos_items) + 1536);
  int stat = render_template_to_buffer_ref(
      todos_list, "views/todooos_list.html",
      TMPL_make_var_list(2, "todoooos", buffer_data(todos_items)));
  if (stat != U_CALLBACK_CONTINUE) {
    free_result(token_result);
    free_result(user_result);
    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  char *token = strdup((char *)token_result->value);
  char cookie[strlen(token) + 69];
  sprintf(cookie, "token=%s; Path=/; HttpOnly; Max-Age=%lld", token, token_age);

  ulfius_set_response_properties(
      res, U_OPT_HEADER_PARAMETER, "HX-Retarget", "#container",
      U_OPT_HEADER_PARAMETER, "set-cookie", cookie, U_OPT_HEADER_PARAMETER,
      "HX-Push-Url", "/", U_OPT_COOKIE_PARAMETER, "token", token, U_OPT_NONE);

  stat = render_template(req, res, "views/todooos.html",
                         TMPL_make_var_list(4, "todooos_list",
                                            buffer_data(todos_list), "username",
                                            u->username));
  if (stat != U_CALLBACK_CONTINUE) {
    free_result(token_result);
    free_result(user_result);
    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  safe_free(token);
  free_result(token_result);
  free_result(user_result);
  buffer_destroy(todos_items);
  buffer_destroy(todos_list);

  return U_CALLBACK_CONTINUE;
}

int handle_index_page(const struct _u_request *req, struct _u_response *res,
                      void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  char *username = (char *)u_map_get(req->map_header, "username");

  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);

  size_t todos_count = 0;
  result *todos_result = db_get_todos_for_user(user_id, &todos_count);
  if (result_is_error(todos_result)) {
    buffer *error_message =
        render_error_message("oopsie doopsie, something went wrong");
    return render_template(req, res, "views/login_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }
  todo *todos = (todo *)todos_result->value;
  buffer *todos_items = buffer_create(
      2560 * todos_count); // size of a single views/todooos_list_item.html
  for (size_t i = 0; i < todos_count; i++) {
    int stat = render_template_to_buffer_ref(
        todos_items, "views/todooos_list_item.html",
        TMPL_make_var_list(6, "todo_id", itoa(todos[i].id), "todo_title",
                           todos[i].title, "todo_done",
                           todos[i].done ? "true" : "false"));

    if (stat != U_CALLBACK_CONTINUE) {
      buffer_destroy(todos_items);
      ulfius_set_string_body_response(res, 500, "Internal server error");
      return U_CALLBACK_ERROR;
    }
  }

  buffer *todos_list = buffer_create(buffer_size(todos_items) + 1536);
  int stat = render_template_to_buffer_ref(
      todos_list, "views/todooos_list.html",
      TMPL_make_var_list(2, "todoooos", buffer_data(todos_items)));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  ulfius_set_response_properties(res, U_OPT_HEADER_PARAMETER, "HX-Push-Url",
                                 "/", U_OPT_NONE);

  buffer *todooos = buffer_create(buffer_size(todos_list) + 4608);
  stat = render_template_to_buffer_ref(
      todooos, "views/todooos.html",
      TMPL_make_var_list(4, "todooos_list", buffer_data(todos_list), "username",
                         username));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    ulfius_set_response_properties(res, U_OPT_STRING_BODY, buffer_data(todooos),
                                   U_OPT_HEADER_PARAMETER, "HX-Retarget",
                                   "#container", U_OPT_HEADER_PARAMETER,
                                   "HX-Push-Url", "/", U_OPT_NONE);

    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    buffer_destroy(todooos);

    return U_CALLBACK_CONTINUE;
  }

  stat = render_template(req, res, "views/layout.html",
                         TMPL_make_var_list(4, "main_content",
                                            buffer_data(todooos), "version",
                                            VERSION));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(todos_items);
    buffer_destroy(todos_list);
    buffer_destroy(todooos);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return stat;
  }

  buffer_destroy(todos_items);
  buffer_destroy(todos_list);
  buffer_destroy(todooos);

  return U_CALLBACK_CONTINUE;
}

int handle_user_settings_page(const struct _u_request *req,
                              struct _u_response *res, void *user_data) {
  char *user_id_str = (char *)u_map_get(req->map_header, "user_id");
  char *username = (char *)u_map_get(req->map_header, "username");

  int user_id = user_id_str == NULL ? 0 : atoi(user_id_str);

  buffer *user_settings = buffer_create(4915);

  int stat = render_template_to_buffer_ref(
      user_settings, "views/user_settings.html",
      TMPL_make_var_list(2, "username", username));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(user_settings);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return stat;
  }

  char *is_htmx = (char *)u_map_get(req->map_header, "HX-Request");
  if (is_htmx != NULL && strcmp(is_htmx, "true") == 0) {
    ulfius_set_response_properties(
        res, U_OPT_STRING_BODY, buffer_data(user_settings),
        U_OPT_HEADER_PARAMETER, "HX-Retarget", "#container",
        U_OPT_HEADER_PARAMETER, "HX-Push-Url", "/user/settings", U_OPT_NONE);

    return U_CALLBACK_CONTINUE;
  }

  stat = render_template(req, res, "views/layout.html",
                         TMPL_make_var_list(4, "main_content",
                                            buffer_data(user_settings),
                                            "version", VERSION));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(user_settings);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return stat;
  }

  buffer_destroy(user_settings);

  return U_CALLBACK_CONTINUE;
}

int handle_register_page(const struct _u_request *req, struct _u_response *res,
                         void *user_data) {
  buffer *register_form = render_template_to_buffer(
      "views/register_form.html",
      TMPL_make_var_list(6, "error", "", "success", "", "username", ""));
  if (register_form == NULL) {
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  buffer *reg = render_template_to_buffer(
      "views/register.html",
      TMPL_make_var_list(2, "register_form", buffer_data(register_form)));
  if (reg == NULL) {
    buffer_destroy(register_form);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  int stat =
      render_template(req, res, "views/layout.html",
                      TMPL_make_var_list(6, "title", "Register", "main_content",
                                         buffer_data(reg), "version", VERSION));
  if (stat != U_CALLBACK_CONTINUE) {
    buffer_destroy(reg);
    buffer_destroy(register_form);
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  buffer_destroy(reg);
  buffer_destroy(register_form);

  return U_CALLBACK_CONTINUE;
}

int handle_register_user(const struct _u_request *req, struct _u_response *res,
                         void *user_data) {
  char *username = (char *)u_map_get(req->map_post_body, "username");
  char *password = (char *)u_map_get(req->map_post_body, "password");
  char *password_confirm =
      (char *)u_map_get(req->map_post_body, "password-confirm");
  if (username == NULL || password == NULL || password_confirm == NULL ||
      strcmp(password, password_confirm) != 0) {
    buffer *error_message = render_error_message("invalid data");
    return render_template(req, res, "views/register_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  user *u = new_user(username, password);
  result *result = db_create_user(u);
  if (result_is_error(result)) {
    buffer *error_message =
        render_error_message("oopsie doopsie, something went wrong");
    return render_template(req, res, "views/register_form.html",
                           TMPL_make_var_list(6, "error",
                                              buffer_data(error_message),
                                              "success", "", "username", ""));
  }

  buffer *success_message =
      render_success_message("You can now login with your new account !");
  buffer *login_form = render_template_to_buffer(
      "views/login_form.html",
      TMPL_make_var_list(6, "error", "", "success",
                         buffer_data(success_message), "username", username));

  ulfius_set_response_properties(res, U_OPT_HEADER_PARAMETER, "HX-Retarget",
                                 "#container", U_OPT_HEADER_PARAMETER,
                                 "HX-Push-Url", "/login", U_OPT_NONE);
  return render_template(
      req, res, "views/login.html",
      TMPL_make_var_list(2, "login_form", buffer_data(login_form)));
}

int handle_serve_static_text(const struct _u_request *req,
                             struct _u_response *res, void *user_data) {
  return serve_file(req, res, "text/plain", false);
}

int handle_serve_static_js(const struct _u_request *req,
                           struct _u_response *res, void *user_data) {
  return serve_file(req, res, "application/javascript", false);
}

int handle_serve_static_css(const struct _u_request *req,
                            struct _u_response *res, void *user_data) {
  return serve_file(req, res, "text/css", false);
}

int handle_serve_static_imgs(const struct _u_request *req,
                             struct _u_response *res, void *user_data) {
  if (strstr(req->url_path, "gif")) {
    return serve_file(req, res, "image/gif", true);
  } else if (strstr(req->url_path, "png")) {
    return serve_file(req, res, "image/png", true);
  } else if (strstr(req->url_path, "webp")) {
    return serve_file(req, res, "image/webp", true);
  } else {
    return serve_file(req, res, "image/png", true);
  }
}

int main(void) {
  init();

  struct _u_instance instance;

  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return 1;
  }

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/admin", 0,
                             &handle_dingus, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/wp-admin", 0,
                             &handle_dingus, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/dingus", 0,
                             &handle_dingus, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/", 1, &handle_index_page,
                             NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/login", 0,
                             &unauthenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/login", 1,
                             &handle_login_page, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/login", 0,
                             &handle_login_user, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/register", 0,
                             &unauthenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/register", 1,
                             &handle_register_page, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/register", 0,
                             &handle_register_user, NULL);

  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/logout", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/logout", 1,
                             &handle_logout, NULL);

  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/todos", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", NULL, "/api/todos", 1,
                             &handle_add_todo, NULL);

  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/todos/:id", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/todos/:id", 1,
                             &handle_delete_todo, NULL);

  ulfius_add_endpoint_by_val(&instance, "PUT", NULL, "/api/todos/:id/toggle", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "PUT", NULL, "/api/todos/:id/toggle", 1,
                             &handle_toggle_todo, NULL);

  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/todos/all", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/todos/all", 1,
                             &handle_delete_all_todos, NULL);

  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL,
                             "/api/todos/all-finished", 0, &authenticated_only,
                             NULL);
  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL,
                             "/api/todos/all-finished", 1,
                             &handle_delete_all_finished_todos, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/api/todos/export/json",
                             0, &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/api/todos/export/json",
                             1, &handle_export_todos_as_json, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/api/todos/export/csv", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/api/todos/export/csv", 1,
                             &handle_export_todos_as_csv, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/user/settings", 0,
                             &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/user/settings", 1,
                             &handle_user_settings_page, NULL);

  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/user/delete-me",
                             0, &authenticated_only, NULL);
  ulfius_add_endpoint_by_val(&instance, "DELETE", NULL, "/api/user/delete-me",
                             1, &handle_delete_user, NULL);

  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/assets/*", 0,
                             &handle_serve_static_text, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/assets/js/*", 0,
                             &handle_serve_static_js, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/assets/css/*", 0,
                             &handle_serve_static_css, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/assets/imgs/*", 0,
                             &handle_serve_static_imgs, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", NULL, "/assets/icons/*", 0,
                             &handle_serve_static_imgs, NULL);

  signal(SIGINT, sigint_handler);
  if (ulfius_start_framework(&instance) == U_OK) {
    printf("Start framework on port %d\n", instance.port);
    while (app_running) {
    }
  } else {
    fprintf(stderr, "Error starting framework\n");
  }
  printf("End framework\n");

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);

  return 0;
}
