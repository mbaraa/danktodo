#ifndef _MIDDLEWARES
#define _MIDDLEWARES

#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <ulfius.h>

#include "db.h"
#include "utils.h"

typedef int (*ulfius_handler)(const struct _u_request *req,
                              struct _u_response *res, void *user_data);

int handle_redirect(const struct _u_request *req, struct _u_response *res,
                    const char *url);
/*
 * @brief this middleware is used for unauthenticated pages
 * valid token => alt handler
 * invalid token => main handler
 *
 * @example
 * going to /register when logged in, redirects to the home page
 * */
int unauthenticated_only(const struct _u_request *req, struct _u_response *res,
                         void *user_data);

/*
 * @brief this middleware is used for authenticated pages
 * valid token => main handler
 * invalid token => alt handler
 *
 * @example
 * going to /settings when not logged in, redirects to the login page
 * */
int authenticated_only(const struct _u_request *req, struct _u_response *res,
                       void *user_data);

#endif // !_MIDDLEWARES
