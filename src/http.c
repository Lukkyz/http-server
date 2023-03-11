#include "./http.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <uv.h>

#include "./hashmap.h"
#include "./server.h"

HashMap *routes;

void init_routes() { routes = malloc(sizeof(HashMap)); }

void route(enum req_verb verb, char *path,
           void (*func)(Request req, Response *res)) {
  insert_func(routes, path, func);
}

int parse(char *buf, char *delimiter, char **result) {
  int i = 0;
  char *token = strtok(buf, delimiter);
  while (token) {
    result[i] = token;
    token = strtok(NULL, delimiter);
    i += 1;
  }
  return i;
}

char *get_date() {
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char *s = malloc(128);
  size_t ret = strftime(s, 128, "%c", tm);
  return s;
}

char *read_file(char *filename) {
  FILE *fp = fopen(filename, "r");
  char *source = NULL;
  if (fp != NULL) {
    /* Go to the end of the file. */
    if (fseek(fp, 0L, SEEK_END) == 0) {
      /* Get the size of the file. */
      long bufsize = ftell(fp);
      if (bufsize == -1) { /* Error */
      }

      /* Allocate our buffer to that size. */
      source = malloc(sizeof(char) * (bufsize + 1));

      /* Go back to the start of the file. */
      if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
      }

      /* Read the entire file into memory. */
      size_t newLen = fread(source, sizeof(char), bufsize, fp);
      if (ferror(fp) != 0) {
        fputs("Error reading file", stderr);
      } else {
        source[newLen++] = '\0'; /* Just to be safe. */
      }
    }
    fclose(fp);
  }
  return source;
}

char *slice_str(char *str, int x, int y) {
  if (y == -1) {
    y = strlen(str) - 1;
  }
  char *new_str = malloc(y - x);
  for (int j = 0; x < y; x++) {
    new_str[j] = str[x];
    j += 1;
  }
  return new_str;
}

void parse_path(char *str, Request *req) {
  char *path[1000];

  int len_path = parse(str, "?", path);

  req->path = malloc(strlen(path[0]));
  strcpy(req->path, path[0]);

  if (len_path == 0) return;

  req->query = malloc(sizeof(HashMap));
  char *query[1000];
  int len_key_val = parse(path[1], "&", query);

  for (int i = 0; i < len_key_val; i++) {
    char *key_val[100];
    parse(query[i], "=", key_val);
    insert(req->query, key_val[0], key_val[1]);
  }
}

void parse_request_line(char *line, Request *req) {
  char *word[20];
  parse(line, " ", word);
  if (strcmp(word[0], "GET") == 0) {
    req->verb = GET;
  } else if (strcmp(word[0], "POST") == 0) {
    req->verb = POST;
  }
  parse_path(word[1], req);
}

void parse_header(char **str, Request *req, int len) {
  req->data = malloc(sizeof(HashMap));
  // We add one because the first line is the request line
  // and len - 1 because last line is empty
  for (int i = 1; i < len - 1; i++) {
    char *key_val[100];
    parse(str[i], " ", key_val);
    // Remove last charac ":"
    char *new_key = slice_str(key_val[0], 0, -1);
    insert(req->data, new_key, key_val[1]);
  }
}

char *read_binary_file(char *filename, int *length) {
  FILE *fp = fopen(filename, "rb");
  char *buffer;
  long len;
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  rewind(fp);
  buffer = (char *)malloc(len * sizeof(char));
  int a = fread(buffer, len, 1, fp);
  fclose(fp);
  *length = len;
  return buffer;
}

void send_response(Response *res, uv_stream_t *client) {
  uv_write_t *info_req = malloc(sizeof(uv_write_t));
  uv_buf_t *res_buff = malloc(sizeof(uv_buf_t));
  res_buff->base = malloc(20000);

  char *content;
  char *type;
  int len;

  char *date = get_date();
  if (res->content_type == NULL) {
    type = "text/html charset=iso-8859-1;";
    content = read_file(res->content);
    sprintf(res_buff->base,
            "HTTP/1.1 %d\r\nServer: http-server/0.1\r\nDate: "
            "%s\r\nContent-Length: %lu\r\nContent-Type: %s"
            "\r\n\r\n%s",
            res->code, date, strlen(content), type, content);
    res_buff->len = strlen(res_buff->base);
  } else {
    type = res->content_type;
    content = read_binary_file(res->content, &len);
    sprintf(res_buff->base,
            "HTTP/1.1 %d\r\nServer: http-server/0.1\r\nDate: "
            "%s\r\nContent-Length: %lu\r\nContent-Type: %s"
            "\r\n\r\n",
            res->code, date, strlen(content), type);
    memcpy(res_buff->base, content, len);
    res_buff->len = strlen(res_buff->base) + len;
  }

  uv_write(info_req, client, res_buff, 1, echo_write);
  uv_close((uv_handle_t *)client, NULL);
}

bool start_with(char *str, char *to_check) {
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] != to_check[i]) {
      return false;
    }
  }
  return true;
}

void parse_request(uv_stream_t *client, const uv_buf_t *buf) {
  char *str[1000];
  int len = parse(buf->base, "\n", str);
  Request *req = malloc(sizeof(Request));
  parse_request_line(str[0], req);
  bool is_static = start_with("/static", req->path);
  parse_header(str, req, len);

  Response *new_res = malloc(sizeof(Response));

  if (is_static) {
    new_res->content = slice_str(req->path, 1, strlen(req->path));
    new_res->code = 200;

    send_response(new_res, client);
    return;
  }
  // Find cb
  KeyVal *finded = find(routes, req->path);

  if (finded == NULL) {
    new_res->content = "pages/404.html";
    new_res->code = 404;
  } else {
    new_res->code = 200;
    (*finded->func)(*req, new_res);
  }
  send_response(new_res, client);
}

