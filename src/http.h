#pragma once
#include <uv.h>

typedef struct HashMap HashMap;

enum req_verb { GET, POST };

typedef struct Request {
  enum req_verb verb;
  char *path;
  HashMap *data;
  HashMap *query;
} Request;

typedef struct Response {
  uint16_t code;
  char *content;
} Response;

void init_routes();
void route(enum req_verb verb, char *path,
           void (*func)(Request req, Response *res));

int parse(char *buf, char *delimiter, char **result);
void parse_path(char *str, Request *req);
char *slice_str(char *str, int x, int y);
void parse_request_line(char *line, Request *req);
void parse_header(char **str, Request *req, int len);
char *get_date();
char *read_file(char *filename);
void send_response(Response *res, uv_stream_t *client);
void parse_request(uv_stream_t *client, const uv_buf_t *buf);
