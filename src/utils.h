#ifndef _UTILS
#define _UTILS

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <crypt.h>
#include <jwt.h>

typedef struct {
  void *value;
  char *error;
} result;

typedef struct {
  char *first;
  char *second;
} pair;

/**
 * @brief Frees the given pointer, and sets it to null to prevent usage after
 * being freed.
 */
void safe_free(void *ptr);

result *new_result(void *value, char *error);
void free_result(result *r);
void *result_value(result *r);
bool result_is_error(result *r);

char *hash_password(char *password);
bool compare_hashes(const char *hashed_password, const char *password);

void init_jwt();
result *encode_jwt(pair *values, time_t exp);
result *decode_jwt(const char *token, char **keys, size_t keys_len);

char *itoa(int n);

#endif // !_UTILS
