#include <stdlib.h>
#include <string.h>

#include "./hashmap.h"
#include "./http.h"
#include "./server.h"

void index_cb(Request req, Response *res) { res->content = "pages/index.html"; }

void about_cb(Request req, Response *res) { res->content = "pages/about.html"; }

int main() {
  void (*index)() = &index_cb;
  void (*about)() = &about_cb;
  init_routes();
  route(GET, "/", index);
  route(GET, "/about", about);
  run();
}

