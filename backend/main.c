#include "mongoose.h"
#include "src/db.h"
#include "src/routes.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#define PORT     "9090"
#define DB_PATH  "data/callcenter.db"

static int s_running = 1;

static void sig_handler(int sig) { (void)sig; s_running = 0; }

static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        routes_handle(c, (struct mg_http_message *)ev_data);
    }
}

/* Resolve absolute path to frontend/ next to the backend/ directory */
static void resolve_frontend_path(char *out, int sz) {
    char exe[512] = {0};
    /* Try /proc/self/exe (Linux) */
    if (readlink("/proc/self/exe", exe, sizeof(exe)-1) > 0) {
        char *dir = dirname(exe);          /* → .../backend           */
        char *parent = dirname(dir);       /* → .../call_center       */
        snprintf(out, sz, "%s/frontend", parent);
        return;
    }
    /* Fallback: use cwd */
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd))) {
        char *parent = dirname(cwd);
        snprintf(out, sz, "%s/frontend", parent);
        return;
    }
    snprintf(out, sz, "../frontend");
}

int main(void) {
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    if (db_init(DB_PATH) < 0) {
        fprintf(stderr, "Failed to initialize database.\n");
        return 1;
    }

    char frontend_path[512];
    resolve_frontend_path(frontend_path, sizeof(frontend_path));
    routes_set_frontend(frontend_path);
    printf("  Serving frontend from: %s\n", frontend_path);

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    char url[32];
    snprintf(url, sizeof(url), "http://0.0.0.0:%s", PORT);

    if (!mg_http_listen(&mgr, url, ev_handler, NULL)) {
        fprintf(stderr, "Failed to listen on port %s\n", PORT);
        return 1;
    }

    printf("\n");
    printf("  Smart Call Center Dispatcher\n");
    printf("  ================================\n");
    printf("  Server running at http://localhost:%s\n", PORT);
    printf("  Open your browser and navigate to the URL above.\n");
    printf("  Press Ctrl+C to stop.\n\n");

    while (s_running) mg_mgr_poll(&mgr, 100);

    mg_mgr_free(&mgr);
    db_close();
    printf("\nServer stopped.\n");
    return 0;
}
