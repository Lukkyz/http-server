#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "./hashmap.h"

uv_loop_t *loop;

HashMap *routes;

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

void echo_write(uv_write_t *req, int status) {
  char *base = (char *)req->data;
  free(base);
  free(req);
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

void send_response(Response *res, uv_stream_t *client) {
  uv_write_t *info_req = malloc(sizeof(uv_write_t));
  uv_buf_t *res_buff = malloc(sizeof(uv_buf_t));
  res_buff->base = malloc(1024);

  char *date = get_date();
  char *content = read_file("pages/index.html");
  sprintf(res_buff->base,
          "HTTP/1.1 %d\r\nServer: http-server/0.1\r\nDate: "
          "%s\r\nContent-Length: %lu\r\nContent-Type: text/html; "
          "charset=iso-8859-1\r\n\r\n%s\r\n",
          res->code, date, strlen(content), content);
  res_buff->len = strlen(res_buff->base);
  uv_write(info_req, client, res_buff, 1, echo_write);
  uv_close((uv_handle_t *)client, NULL);
}

void parse_request(uv_stream_t *client, const uv_buf_t *buf) {
  char *str[1000];
  int len = parse(buf->base, "\n", str);
  Request *req = malloc(sizeof(Request));
  parse_request_line(str[0], req);
  parse_header(str, req, len);
  // Find cb
  KeyVal *finded = find(routes, req->path);
  Response *new_res = malloc(sizeof(Response));
  (*finded->func)(*req, new_res);
  send_response(new_res, client);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
  printf("%s\n", buf->base);
  parse_request(client, buf);
  if (nread == -1) {
    fprintf(stderr, "Read error\n");
    uv_close((uv_handle_t *)client, NULL);
    return;
  }
  // uv_write_t *info_req = malloc(sizeof(uv_write_t));
  // uv_buf_t buff;
  // buff.base = "HTTP/1.1 200 OK\r\n";
  // buff.len = strlen(buff.base);
  // uv_write(info_req, client, &buff, 1, echo_write);
  // uv_close((uv_handle_t *)client, NULL);
}

void on_new_connection(uv_stream_t *server, int status) {
  if (status == -1) return;

  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);

  if (uv_accept(server, (uv_stream_t *)client) == 0) {
    uv_read_start((uv_stream_t *)client, alloc_buffer, echo_read);
  } else {
    uv_close((uv_handle_t *)client, NULL);
  }
}

void test(Request req, Response *res) { res->code = 200; }

int main() {
  loop = uv_default_loop();

  routes = malloc(sizeof(HashMap));
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  void (*func)() = &test;
  route(GET, "/salut", func);
  route(GET, "/bonsoir", func);
  struct sockaddr_in recv_addr;
  uv_ip4_addr("0.0.0.0", 7000, &recv_addr);
  uv_tcp_bind(&server, (const struct sockaddr *)&recv_addr, 0);

  int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
  if (r) {
    fprintf(stderr, "Listen error");
    return 1;
  }
  return uv_run(loop, UV_RUN_DEFAULT);
}
