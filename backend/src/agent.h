#ifndef AGENT_H
#define AGENT_H

void agent_get_all(char *out, int out_sz);
void agent_get_by_id(int id, char *out, int out_sz);
void agent_create(const char *body, char *out, int out_sz);
void agent_update(int id, const char *body, char *out, int out_sz);
void agent_delete(int id, char *out, int out_sz);
void agent_update_status(int id, const char *status, char *out, int out_sz);

#endif
