#include "./hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hash(char *key, int size) {
  int total = 0;
  for (int i = 0; i < strlen(key); i++) {
    total += key[i];
  }
  return total % size;
}

void insert_func(HashMap *hm, char *key,
                 void (*ptr)(Request req, Response *res)) {
  int index = hash(key, 100);
  KeyVal *kv = malloc(sizeof(KeyVal));
  kv->key = key;
  kv->func = ptr;
  hm->keyvals[index] = kv;
}

void insert(HashMap *hm, char *key, char *str) {
  int index = hash(key, 100);
  KeyVal *kv = malloc(sizeof(KeyVal));
  kv->key = key;
  kv->val = str;
  hm->keyvals[index] = kv;
}

KeyVal *find(HashMap *hm, char *key) {
  int index = hash(key, 100);
  KeyVal *kv = hm->keyvals[index];
  if (kv != NULL) {
    return hm->keyvals[index];
  } else {
    return NULL;
  }
}
