#include "routes.h"
#include "agent.h"
#include "call.h"
#include "dispatcher.h"
#include "report.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define OUT_SZ (1024 * 64)

static char s_frontend_path[512] = "../frontend";

void routes_set_frontend(const char *path) {
    snprintf(s_frontend_path, sizeof(s_frontend_path), "%s", path);
}

static void send_json(struct mg_connection *c, int code, const char *body) {
    mg_http_reply(c, code,
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n",
        "%s", body);
}

static int id_from_uri(struct mg_str uri, const char *prefix) {
    size_t plen = strlen(prefix);
    if (uri.len <= plen) return -1;
    return atoi(uri.buf + plen);
}

static void body_str(struct mg_http_message *hm, char *out, int sz) {
    int n = (int)hm->body.len < sz-1 ? (int)hm->body.len : sz-1;
    memcpy(out, hm->body.buf, n);
    out[n] = '\0';
}

void routes_handle(struct mg_connection *c, struct mg_http_message *hm) {
    char *out = malloc(OUT_SZ);
    if (!out) { send_json(c, 500, "{\"error\":\"OOM\"}"); return; }

    struct mg_str uri    = hm->uri;
    struct mg_str method = hm->method;

#define IS(m,u) (mg_strcmp(method, mg_str(m))==0 && mg_strcmp(uri, mg_str(u))==0)
#define STARTS(u) (strncmp(uri.buf, u, strlen(u))==0)

    /* CORS preflight */
    if (mg_strcmp(method, mg_str("OPTIONS")) == 0) {
        send_json(c, 200, "{}");

    /* --- AGENTS --- */
    } else if (IS("GET", "/api/agents")) {
        agent_get_all(out, OUT_SZ);
        send_json(c, 200, out);

    } else if (IS("POST", "/api/agents")) {
        char body[2048]; body_str(hm, body, sizeof(body));
        agent_create(body, out, OUT_SZ);
        send_json(c, 201, out);

    } else if (STARTS("/api/agents/status/")) {
        int id = id_from_uri(uri, "/api/agents/status/");
        char body[256]; body_str(hm, body, sizeof(body));
        char status[32]={};
        char *p = strstr(body,"\"status\"");
        if (p) { p=strchr(p,':'); if(p){p++;while(*p==' ')p++;if(*p=='"'){p++;int i=0;while(*p&&*p!='"'&&i<31)status[i++]=*p++;status[i]='\0';}}}
        agent_update_status(id, status, out, OUT_SZ);
        send_json(c, 200, out);

    } else if (STARTS("/api/agents/")) {
        int id = id_from_uri(uri, "/api/agents/");
        if (mg_strcmp(method, mg_str("GET")) == 0) {
            agent_get_by_id(id, out, OUT_SZ);
            send_json(c, 200, out);
        } else if (mg_strcmp(method, mg_str("PUT")) == 0) {
            char body[2048]; body_str(hm, body, sizeof(body));
            agent_update(id, body, out, OUT_SZ);
            send_json(c, 200, out);
        } else if (mg_strcmp(method, mg_str("DELETE")) == 0) {
            agent_delete(id, out, OUT_SZ);
            send_json(c, 200, out);
        }

    /* --- CALLS --- */
    } else if (IS("GET", "/api/calls")) {
        call_get_all(out, OUT_SZ);
        send_json(c, 200, out);

    } else if (IS("POST", "/api/calls")) {
        char body[2048]; body_str(hm, body, sizeof(body));
        call_create(body, out, OUT_SZ);
        send_json(c, 201, out);

    } else if (STARTS("/api/calls/resolve/")) {
        int id = id_from_uri(uri, "/api/calls/resolve/");
        call_resolve(id, out, OUT_SZ);
        send_json(c, 200, out);

    } else if (STARTS("/api/calls/abandon/")) {
        int id = id_from_uri(uri, "/api/calls/abandon/");
        call_abandon(id, out, OUT_SZ);
        send_json(c, 200, out);

    } else if (STARTS("/api/calls/")) {
        int id = id_from_uri(uri, "/api/calls/");
        if (mg_strcmp(method, mg_str("PUT")) == 0) {
            char body[2048]; body_str(hm, body, sizeof(body));
            call_update(id, body, out, OUT_SZ);
            send_json(c, 200, out);
        }

    /* --- QUEUE --- */
    } else if (IS("GET", "/api/queue")) {
        call_get_queue(out, OUT_SZ);
        send_json(c, 200, out);

    /* --- DISPATCH --- */
    } else if (IS("POST", "/api/dispatch")) {
        char body[512]; body_str(hm, body, sizeof(body));
        dispatch_call(body, out, OUT_SZ);
        send_json(c, 200, out);

    } else if (IS("POST", "/api/dispatch/auto")) {
        dispatch_auto(out, OUT_SZ);
        send_json(c, 200, out);

    /* --- REPORTS --- */
    } else if (IS("GET", "/api/reports/dashboard")) {
        report_dashboard(out, OUT_SZ);
        send_json(c, 200, out);

    } else if (IS("GET", "/api/reports/daily")) {
        report_daily(out, OUT_SZ);
        send_json(c, 200, out);

    } else if (IS("GET", "/api/reports/agents")) {
        report_agents(out, OUT_SZ);
        send_json(c, 200, out);

    /* --- STATIC FILES (frontend) --- */
    } else {
        struct mg_http_serve_opts opts = {.root_dir = s_frontend_path};
        mg_http_serve_dir(c, hm, &opts);
    }

    free(out);
}
