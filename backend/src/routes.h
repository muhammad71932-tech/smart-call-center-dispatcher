#ifndef ROUTES_H
#define ROUTES_H

#include "../mongoose.h"

void routes_set_frontend(const char *path);
void routes_handle(struct mg_connection *c, struct mg_http_message *hm);

#endif
