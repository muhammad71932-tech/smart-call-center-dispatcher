#ifndef CALL_H
#define CALL_H

void call_get_all(char *out, int out_sz);
void call_get_queue(char *out, int out_sz);
void call_create(const char *body, char *out, int out_sz);
void call_update(int id, const char *body, char *out, int out_sz);
void call_resolve(int id, char *out, int out_sz);
void call_abandon(int id, char *out, int out_sz);

#endif
