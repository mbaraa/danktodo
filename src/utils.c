#include "./utils.h"

void safe_free(void *ptr) {
  free(ptr);
  ptr = NULL;
}

result *new_result(void *value, char *error) {
  result *r = (result *)malloc(sizeof(result));
  if (r == NULL) {
    return NULL;
  }

  r->value = value;
  r->error = error;
  return r;
}

void *result_value(result *r) {
  if (r == NULL) {
    return NULL;
  }
  return r->value;
}

bool result_is_error(result *r) {
  if (r == NULL) {
    return false;
  }
  if (r->error == NULL) {
    return false;
  }
  if (strlen(r->error) == 0) {
    return false;
  }

  return true;
}

void free_result(result *r) {
  safe_free(r->value);
  safe_free(r->error);
  safe_free(r);
}

char *hash_password(char *password) {
  char *hashed = (char *)malloc(128);
  if (hashed == NULL) {
    return NULL;
  }

  const char *salt = "mingusdadingus";
  char *result = crypt(password, salt);
  if (result == NULL) {
    return NULL;
  }

  if (result != NULL) {
    strcpy(hashed, result);
  } else {
    safe_free(hashed);
    return NULL;
  }

  return hashed;
}

bool compare_hashes(const char *hashed_password, const char *password) {
  char *salt = strndup(hashed_password,
                       strchr(hashed_password, '$') - hashed_password + 1);
  if (salt == NULL) {
    return -1;
  }

  struct crypt_data data;
  data.initialized = 0;
  char *new_hash = crypt_r(password, salt, &data);
  safe_free(salt);

  if (new_hash == NULL) {
    return -1;
  }

  return strcmp(hashed_password, new_hash) == 0;
}

char *jwt_secret = "secret-cyka";

void init_jwt() {
  char *secret = getenv("JWT_SECRET");
  if (secret == 0) {
    return;
  }
  jwt_secret = strdup(secret);
}

result *encode_jwt(pair *values, time_t exp) {
  jwt_t *jwt = NULL;
  int err = jwt_new(&jwt);
  if (err) {
    return new_result(NULL, "error while creating jwt instance");
  }

  jwt_add_grant_int(jwt, "iat", time(NULL));
  jwt_add_grant_int(jwt, "exp", exp);

  size_t values_len = sizeof(*values) / sizeof(pair *);
  for (size_t i = 0; i < values_len; i++) {
    jwt_add_grant(jwt, values[i].first, values[i].second);
  }

  jwt_set_alg(jwt, JWT_ALG_HS256, (const unsigned char *)jwt_secret,
              strlen(jwt_secret));

  char *token = jwt_encode_str(jwt);
  if (!token) {
    jwt_free(jwt);
    return new_result(NULL, "error while encoding jwt");
  }

  jwt_free(jwt);

  return new_result(token, NULL);
}

result *decode_jwt(const char *token, char **keys, size_t keys_len) {
  jwt_t *decoded_jwt = NULL;
  int err = jwt_decode(&decoded_jwt, token, (const unsigned char *)jwt_secret,
                       strlen(jwt_secret));
  if (err) {
    return new_result(NULL, "error while creating jwt instance");
  }

  time_t exp = jwt_get_grant_int(decoded_jwt, "exp");
  if (exp < time(NULL)) {
    return new_result(NULL, "token has expired");
  }

  pair *jwt_payload = malloc(keys_len * sizeof(pair));
  for (size_t i = 0; i < keys_len; i++) {
    jwt_payload[i].first = (char *)malloc(strlen(keys[i]));
    strcpy(jwt_payload[i].first, keys[i]);

    const char *value = jwt_get_grant(decoded_jwt, (const char *)keys[i]);
    jwt_payload[i].second = (char *)malloc(strlen(value));
    strcpy(jwt_payload[i].second, value);
  }

  jwt_free(decoded_jwt);

  return new_result(jwt_payload, NULL);
}

char *itoa(int n) {
  if (n <= 0) {
    n++;
  }
  size_t str_len = (int)((ceil(log10(n)) + 1) * sizeof(char));
  char *str = (char *)malloc(str_len);
  sprintf(str, "%d", n);

  return str;
}
