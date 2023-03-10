#pragma once
#include <stdint.h>

typedef struct Request Request;
typedef struct Response Response;

typedef struct KeyVal {
  char *key;
  union {
    void (*func)(Request req, Response *res);
    char *val;
  };
} KeyVal;

typedef struct HashMap {
  KeyVal *keyvals[100];
} HashMap;

void insert(HashMap *hm, char *key, char *str);
void insert_func(HashMap *hm, char *key,
                 void (*ptr)(Request req, Response *res));
int hash(char *key, int size);
KeyVal *find(HashMap *hm, char *key);

