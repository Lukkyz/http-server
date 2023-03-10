#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdint.h>

enum req_verb { GET, POST };

typedef struct HashMap HashMap;
typedef struct Request Request;
typedef struct KeyVal KeyVal;
typedef struct Response Response;

struct Request {
  enum req_verb verb;
  char *path;
  HashMap *data;
  HashMap *query;
};

struct KeyVal {
  char *key;
  union {
    void (*func)(Request req, Response *res);
    char *val;
  };
};

struct HashMap {
  KeyVal keyvals[100];
};

struct Response {
  uint16_t code;
  char *html;
};

void insert(HashMap *hm, char *key, char *str);
void insert_func(HashMap *hm, char *key,
                 void (*ptr)(Request req, Response *res));
int hash(char *key, int size);
KeyVal *find(HashMap *hm, char *key);
#endif

