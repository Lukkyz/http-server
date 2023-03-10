#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "./http.h"

uv_loop_t *loop;

void echo_write(uv_write_t *req, int status) {
  char *base = (char *)req->data;
  free(base);
  free(req);
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

int run() {
  loop = uv_default_loop();

  // routes = malloc(sizeof(HashMap));
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

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
