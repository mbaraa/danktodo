#ifndef _HANDLERS_HELPERS
#define _HANDLERS_HELPERS

#include <stdbool.h>
#include <stdio.h>

#include <ctemplate/buffer.h>
#include <ctemplate/ctemplate.h>
#include <ulfius.h>

#include "./utils.h"

typedef struct {
  char *key;
  char *value;
} query_param;

char *get_query_param_value(query_param *params, const char *name);
query_param *extract_query_params(const char *query);
void free_query_params(query_param *params);

int render_template(const struct _u_request *req, struct _u_response *res,
                    const char *name, TMPL_varlist *list);
buffer *render_template_to_buffer(const char *name, TMPL_varlist *list);
int render_template_to_buffer_ref(buffer *buf, const char *name,
                                  TMPL_varlist *list);
buffer *render_error_message(char *message);
buffer *render_success_message(char *message);

int serve_file(const struct _u_request *req, struct _u_response *res,
               const char *content_type, bool is_binary);

#endif // !_HANDLERS_HELPERS
