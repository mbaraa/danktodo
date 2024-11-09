#include "handlers_helpers.h"

buffer *render_success_message(char *message) {
  return render_template_to_buffer("views/success.html",
                                   TMPL_make_var_list(2, "message", message));
}

buffer *render_error_message(char *message) {
  return render_template_to_buffer("views/error.html",
                                   TMPL_make_var_list(2, "message", message));
}

int render_template(const struct _u_request *req, struct _u_response *res,
                    const char *name, TMPL_varlist *list) {
  buffer *buf = buffer_create(128);
  if (buf == NULL) {
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  if (TMPL_write_to_buffer(name, NULL, NULL, list, buf, stderr)) {
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  char *resp_body = buffer_data(buf);
  ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_STRING_BODY,
                                 resp_body, U_OPT_HEADER_PARAMETER,
                                 "Content-Type", "text/html; charset=utf-8",
                                 U_OPT_NONE);

  TMPL_free_varlist(list);
  buffer_destroy(buf);
  safe_free(resp_body);

  return U_CALLBACK_CONTINUE;
}

buffer *render_template_to_buffer(const char *name, TMPL_varlist *list) {
  buffer *buf = buffer_create(16);
  if (buf == NULL) {
    return NULL;
  }
  if (TMPL_write_to_buffer(name, NULL, NULL, list, buf, stderr)) {
    buffer_destroy(buf);
    TMPL_free_varlist(list);
    return NULL;
  }
  TMPL_free_varlist(list);
  /* safe_free((void *)name); */

  return buf;
}

int render_template_to_buffer_ref(buffer *buf, const char *name,
                                  TMPL_varlist *list) {
  if (TMPL_write_to_buffer(name, NULL, NULL, list, buf, stderr)) {
    buffer_destroy(buf);
    TMPL_free_varlist(list);
    return U_CALLBACK_ERROR;
  }
  TMPL_free_varlist(list);

  return U_CALLBACK_CONTINUE;
}

int serve_file(const struct _u_request *req, struct _u_response *res,
               const char *content_type, bool is_binary) {
  if (strstr(req->url_path, "..")) {
    ulfius_set_string_body_response(res, 404, "You're not funny yk!");
    return U_CALLBACK_ERROR;
  }

  if (strlen(req->url_path) < strlen("/assets/")) {
    perror("bad request");
    ulfius_set_string_body_response(res, 400, "Bad request");
    return U_CALLBACK_ERROR;
  }

  size_t len = strlen(req->url_path);
  char *file_path = (char *)malloc(len);
  for (size_t i = 0; i < len - 1; i++) {
    file_path[i] = req->url_path[i + 1];
  }
  file_path[len - 1] = '\0';

  FILE *file;
  file = fopen(file_path, is_binary ? "rb+" : "r+");
  if (!file) {
    perror("Error opening file");
    ulfius_set_string_body_response(res, 404, "Not found");
    return U_CALLBACK_ERROR;
  }

  struct stat file_stat;
  if (fstat(fileno(file), &file_stat) != 0) {
    perror("Error getting file size");
    ulfius_set_string_body_response(res, 500, "Internal server error");
    return U_CALLBACK_ERROR;
  }

  unsigned char resp_data[file_stat.st_size + !is_binary];
  fread(resp_data, file_stat.st_size, 1, file);

  if (is_binary) {
    ulfius_set_response_properties(
        res, U_OPT_STATUS, 200, U_OPT_BINARY_BODY, resp_data, file_stat.st_size,
        U_OPT_HEADER_PARAMETER, "Content-Type", content_type, U_OPT_NONE);
  } else {
    resp_data[file_stat.st_size] = '\0';
    ulfius_set_response_properties(res, U_OPT_STATUS, 200, U_OPT_STRING_BODY,
                                   resp_data, U_OPT_HEADER_PARAMETER,
                                   "Content-Type", content_type, U_OPT_NONE);
  }

  fclose(file);
  safe_free(file_path);

  return U_CALLBACK_CONTINUE;
}

#define MAX_PARAMS 10

char *get_query_param_value(query_param *params, const char *name) {
  char *ret = NULL;
  if (params == NULL || name == NULL) {
    return ret;
  }

  size_t params_length = sizeof(*params) / sizeof(query_param *);
  for (size_t i = 0; i < params_length; i++) {
    if (!strcmp(params[i].key, name)) {
      ret = params[i].value;
      break;
    }
  }

  return ret;
}

query_param *extract_query_params(const char *query) {
  query_param *params = malloc(MAX_PARAMS * sizeof(query_param));
  if (!params) {
    perror("Failed to allocate memory");
    return NULL;
  }

  char *query_copy = strdup(query);
  if (!query_copy) {
    perror("Failed to allocate memory");
    safe_free(params);
    return NULL;
  }

  char *pair = strtok(query_copy, "&");
  int count = 0;

  while (pair && count < MAX_PARAMS) {
    char *equals = strchr(pair, '=');
    if (equals) {
      *equals = '\0';
      params[count].key = strdup(pair);
      params[count].value = strdup(equals + 1);
      count++;
    }
    pair = strtok(NULL, "&");
  }

  safe_free(query_copy);
  return params;
}

void free_query_params(query_param *params) {
  size_t params_length = sizeof(*params) / sizeof(query_param *);
  for (int i = 0; i < params_length; i++) {
    safe_free(params[i].key);
    safe_free(params[i].value);
  }
  safe_free(params);
}
