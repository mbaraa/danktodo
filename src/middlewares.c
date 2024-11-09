#include "./middlewares.h"
#include <ulfius.h>

result *check_token_and_return_user(const char *token);

int handle_redirect(const struct _u_request *req, struct _u_response *res,
                    const char *url) {
  char *body = (char *)malloc(strlen(url) + 26);
  sprintf(body, "<a href=\"%s\">301</a>.\n", url);
  ulfius_set_response_properties(res, U_OPT_STATUS, 301, U_OPT_STRING_BODY,
                                 body, U_OPT_HEADER_PARAMETER, "Location", url,
                                 U_OPT_HEADER_PARAMETER, "Content-Type",
                                 "text/html; charset=utf-8", U_OPT_NONE);

  return U_CALLBACK_COMPLETE;
}

int unauthenticated_only(const struct _u_request *req, struct _u_response *res,
                         void *user_data) {
  char *token = (char *)u_map_get(req->map_cookie, "token");
  result *u = check_token_and_return_user(token);
  if (result_is_error(u)) {
    return U_CALLBACK_CONTINUE;
  }
  u_map_put(req->map_header, "user_id", itoa(((user *)u->value)->id));
  u_map_put(req->map_header, "username", strdup(((user *)u->value)->username));

  return handle_redirect(req, res, "/");
}

int authenticated_only(const struct _u_request *req, struct _u_response *res,
                       void *user_data) {
  char *token = (char *)u_map_get(req->map_cookie, "token");
  result *u = check_token_and_return_user(token);
  if (result_is_error(u)) {
    return handle_redirect(req, res, "/login");
  }
  u_map_put(req->map_header, "user_id", itoa(((user *)u->value)->id));
  u_map_put(req->map_header, "username", strdup(((user *)u->value)->username));

  return U_CALLBACK_CONTINUE;
}

result *check_token_and_return_user(const char *token) {
  if (token == NULL || strlen(token) == 0) {
    return new_result(NULL, "token is empty");
  }

  char **keys = malloc(2 * sizeof(char **));
  keys[0] = "iss";
  keys[1] = "sub";
  result *dec_tok_res = decode_jwt(token, keys, 2);
  if (result_is_error(dec_tok_res)) {
    return dec_tok_res;
  }

  char *username;
  pair *payload = (pair *)dec_tok_res->value;
  for (size_t i = 0; i < 2; i++) {
    if (strcmp(payload[i].first, "sub") == 0) {
      username = strdup(payload[i].second);
    }
  }
  result *u = db_get_user(username);
  if (result_is_error(u)) {
    return u;
  }

  free_result(dec_tok_res);
  safe_free(keys);

  return u;
}
