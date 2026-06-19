#include "dispatcher.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

static char *extract_json_str(const char *body, const char *key, char *val, int vsz) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    char *p = strstr(body, search);
    if (!p) { val[0]='\0'; return NULL; }
    p += strlen(search);
    while (*p==' '||*p==':') p++;
    if (*p=='"') {
        p++;
        int i=0;
        while (*p && *p!='"' && i<vsz-1) {
            if (*p=='\\') p++;
            val[i++]=*p++;
        }
        val[i]='\0';
        return val;
    }
    val[0]='\0';
    return NULL;
}

static int extract_json_int(const char *body, const char *key) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    char *p = strstr(body, search);
    if (!p) return -1;
    p += strlen(search);
    while (*p==' '||*p==':') p++;
    if (*p>='0' && *p<='9') return atoi(p);
    return -1;
}

/* Find best available agent for a call category using skills matching */
static int find_best_agent(const char *category) {
    sqlite3_stmt *stmt;
    /* prefer agents whose skills contain the category, least calls_handled first */
    const char *sql =
        "SELECT id FROM agents WHERE status='AVAILABLE'"
        " ORDER BY CASE WHEN skills LIKE ? THEN 0 ELSE 1 END, calls_handled ASC LIMIT 1;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    char like_pat[128];
    snprintf(like_pat, sizeof(like_pat), "%%%s%%", category);
    sqlite3_bind_text(stmt, 1, like_pat, -1, SQLITE_TRANSIENT);
    int agent_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        agent_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return agent_id;
}

void dispatch_call(const char *body, char *out, int out_sz) {
    int call_id  = extract_json_int(body, "call_id");
    int agent_id_req = extract_json_int(body, "agent_id");
    char category[64]={};
    extract_json_str(body, "category", category, sizeof(category));

    if (call_id < 0) { snprintf(out, out_sz, "{\"error\":\"call_id required\"}"); return; }

    int agent_id = (agent_id_req > 0) ? agent_id_req : find_best_agent(category);
    if (agent_id < 0) { snprintf(out, out_sz, "{\"error\":\"No available agent found\"}"); return; }

    sqlite3_stmt *stmt;
    /* assign agent, update status, set wait_time */
    const char *sql =
        "UPDATE calls SET agent_id=?, status='ACTIVE',"
        " wait_time=CAST((julianday(CURRENT_TIMESTAMP)-julianday(created_at))*86400 AS INTEGER)"
        " WHERE id=? AND status='WAITING';";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, agent_id);
    sqlite3_bind_int(stmt, 2, call_id);
    if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_get()) > 0) {
        /* mark agent busy */
        sqlite3_stmt *s2;
        sqlite3_prepare_v2(db_get(), "UPDATE agents SET status='BUSY' WHERE id=?;", -1, &s2, NULL);
        sqlite3_bind_int(s2, 1, agent_id);
        sqlite3_step(s2);
        sqlite3_finalize(s2);
        snprintf(out, out_sz, "{\"success\":true,\"call_id\":%d,\"agent_id\":%d}", call_id, agent_id);
    } else {
        snprintf(out, out_sz, "{\"error\":\"Dispatch failed — call may not be in WAITING state\"}");
    }
    sqlite3_finalize(stmt);
}

/* Auto-dispatch: assign all WAITING calls to available agents */
void dispatch_auto(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT id, category FROM calls WHERE status='WAITING'"
        " ORDER BY CASE priority WHEN 'EMERGENCY' THEN 0 WHEN 'VIP' THEN 1 ELSE 2 END, created_at ASC;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    int dispatched = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int cid = sqlite3_column_int(stmt, 0);
        const char *cat = (const char *)sqlite3_column_text(stmt, 1);
        int aid = find_best_agent(cat ? cat : "");
        if (aid < 0) break; /* no more agents */

        sqlite3_stmt *upd;
        sqlite3_prepare_v2(db_get(),
            "UPDATE calls SET agent_id=?,status='ACTIVE',"
            "wait_time=CAST((julianday(CURRENT_TIMESTAMP)-julianday(created_at))*86400 AS INTEGER)"
            " WHERE id=?;",
            -1, &upd, NULL);
        sqlite3_bind_int(upd, 1, aid);
        sqlite3_bind_int(upd, 2, cid);
        sqlite3_step(upd);
        sqlite3_finalize(upd);

        sqlite3_stmt *s2;
        sqlite3_prepare_v2(db_get(), "UPDATE agents SET status='BUSY' WHERE id=?;", -1, &s2, NULL);
        sqlite3_bind_int(s2, 1, aid);
        sqlite3_step(s2);
        sqlite3_finalize(s2);
        dispatched++;
    }
    sqlite3_finalize(stmt);
    snprintf(out, out_sz, "{\"success\":true,\"dispatched\":%d}", dispatched);
}
