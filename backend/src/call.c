#include "call.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

static const char *col(sqlite3_stmt *s, int i) {
    const char *v = (const char *)sqlite3_column_text(s, i);
    return v ? v : "";
}

static char *extract_json_str(const char *body, const char *key, char *val, int vsz) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\"", key);
    char *p = strstr(body, search);
    if (!p) { val[0] = '\0'; return NULL; }
    p += strlen(search);
    while (*p == ' ' || *p == ':') p++;
    if (*p == '"') {
        p++;
        int i = 0;
        while (*p && *p != '"' && i < vsz - 1) {
            if (*p == '\\') p++;
            val[i++] = *p++;
        }
        val[i] = '\0';
        return val;
    }
    val[0] = '\0';
    return NULL;
}

void call_get_all(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT c.id, c.caller_id, c.caller_name, c.priority, c.category, c.status,"
        " c.agent_id, COALESCE(a.name,'Unassigned'), c.notes, c.wait_time, c.handle_time, c.created_at, c.resolved_at"
        " FROM calls c LEFT JOIN agents a ON c.agent_id=a.id ORDER BY c.id DESC LIMIT 200;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    strcpy(out, "[");
    int first = 1;
    char buf[512];
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(buf, sizeof(buf),
            "%s{\"id\":%d,\"caller_id\":\"%s\",\"caller_name\":\"%s\",\"priority\":\"%s\","
            "\"category\":\"%s\",\"status\":\"%s\",\"agent_id\":%d,\"agent_name\":\"%s\","
            "\"notes\":\"%s\",\"wait_time\":%d,\"handle_time\":%d,\"created_at\":\"%s\",\"resolved_at\":\"%s\"}",
            first ? "" : ",",
            sqlite3_column_int(stmt,0), col(stmt,1), col(stmt,2), col(stmt,3),
            col(stmt,4), col(stmt,5), sqlite3_column_int(stmt,6), col(stmt,7),
            col(stmt,8), sqlite3_column_int(stmt,9), sqlite3_column_int(stmt,10),
            col(stmt,11), col(stmt,12));
        if (strlen(out) + strlen(buf) < (size_t)out_sz - 4)
            strcat(out, buf);
        first = 0;
    }
    strcat(out, "]");
    sqlite3_finalize(stmt);
}

void call_get_queue(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT id, caller_id, caller_name, priority, category, created_at"
        " FROM calls WHERE status='WAITING'"
        " ORDER BY CASE priority WHEN 'EMERGENCY' THEN 0 WHEN 'VIP' THEN 1 ELSE 2 END, created_at ASC;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    strcpy(out, "[");
    int first = 1;
    char buf[256];
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(buf, sizeof(buf),
            "%s{\"id\":%d,\"caller_id\":\"%s\",\"caller_name\":\"%s\",\"priority\":\"%s\",\"category\":\"%s\",\"created_at\":\"%s\"}",
            first ? "" : ",",
            sqlite3_column_int(stmt,0), col(stmt,1), col(stmt,2),
            col(stmt,3), col(stmt,4), col(stmt,5));
        if (strlen(out) + strlen(buf) < (size_t)out_sz - 4)
            strcat(out, buf);
        first = 0;
    }
    strcat(out, "]");
    sqlite3_finalize(stmt);
}

void call_create(const char *body, char *out, int out_sz) {
    char caller_id[64]={}, caller_name[128]={}, priority[32]={}, category[64]={}, notes[256]={};
    extract_json_str(body, "caller_id",   caller_id,   sizeof(caller_id));
    extract_json_str(body, "caller_name", caller_name, sizeof(caller_name));
    extract_json_str(body, "priority",    priority,    sizeof(priority));
    extract_json_str(body, "category",    category,    sizeof(category));
    extract_json_str(body, "notes",       notes,       sizeof(notes));

    if (!caller_id[0]) { snprintf(out, out_sz, "{\"error\":\"caller_id required\"}"); return; }
    if (!priority[0])  strcpy(priority, "NORMAL");
    if (!category[0])  strcpy(category, "general");

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO calls(caller_id,caller_name,priority,category,notes,status) VALUES(?,?,?,?,?,'WAITING');";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, caller_id,   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, caller_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, priority,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, category,    -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, notes,       -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int new_id = (int)sqlite3_last_insert_rowid(db_get());
        snprintf(out, out_sz, "{\"success\":true,\"id\":%d}", new_id);
    } else {
        snprintf(out, out_sz, "{\"error\":\"Insert failed\"}");
    }
    sqlite3_finalize(stmt);
}

void call_update(int id, const char *body, char *out, int out_sz) {
    char status[32]={}, notes[256]={};
    extract_json_str(body, "status", status, sizeof(status));
    extract_json_str(body, "notes",  notes,  sizeof(notes));

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),
        "UPDATE calls SET status=COALESCE(NULLIF(?,''),status), notes=COALESCE(NULLIF(?,''),notes) WHERE id=?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, notes,  -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, id);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        snprintf(out, out_sz, "{\"success\":true}");
    else
        snprintf(out, out_sz, "{\"error\":\"Update failed\"}");
    sqlite3_finalize(stmt);
}

void call_resolve(int id, char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "UPDATE calls SET status='COMPLETED', resolved_at=CURRENT_TIMESTAMP,"
        " handle_time=CAST((julianday(CURRENT_TIMESTAMP)-julianday(created_at))*86400 AS INTEGER)"
        " WHERE id=?;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        /* update agent call count */
        sqlite3_stmt *s2;
        sqlite3_prepare_v2(db_get(),
            "UPDATE agents SET calls_handled=calls_handled+1 WHERE id=(SELECT agent_id FROM calls WHERE id=?);",
            -1, &s2, NULL);
        sqlite3_bind_int(s2, 1, id);
        sqlite3_step(s2);
        sqlite3_finalize(s2);
        /* free agent */
        sqlite3_stmt *s3;
        sqlite3_prepare_v2(db_get(),
            "UPDATE agents SET status='AVAILABLE' WHERE id=(SELECT agent_id FROM calls WHERE id=?) AND status='BUSY';",
            -1, &s3, NULL);
        sqlite3_bind_int(s3, 1, id);
        sqlite3_step(s3);
        sqlite3_finalize(s3);
        snprintf(out, out_sz, "{\"success\":true}");
    } else {
        snprintf(out, out_sz, "{\"error\":\"Resolve failed\"}");
    }
    sqlite3_finalize(stmt);
}

void call_abandon(int id, char *out, int out_sz) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(),
        "UPDATE calls SET status='ABANDONED', resolved_at=CURRENT_TIMESTAMP WHERE id=?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        snprintf(out, out_sz, "{\"success\":true}");
    else
        snprintf(out, out_sz, "{\"error\":\"Abandon failed\"}");
    sqlite3_finalize(stmt);
}
