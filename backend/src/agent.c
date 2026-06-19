#include "agent.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

static void json_escape(const char *in, char *out, int sz) {
    int j = 0;
    for (int i = 0; in[i] && j < sz - 2; i++) {
        if (in[i] == '"')  { out[j++] = '\\'; out[j++] = '"'; }
        else if (in[i] == '\\') { out[j++] = '\\'; out[j++] = '\\'; }
        else out[j++] = in[i];
    }
    out[j] = '\0';
}

static const char *col(sqlite3_stmt *s, int i) {
    const char *v = (const char *)sqlite3_column_text(s, i);
    return v ? v : "";
}

void agent_get_all(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id,name,phone,email,status,skills,calls_handled,created_at FROM agents ORDER BY id;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    char buf[600];
    strcpy(out, "[");
    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        char ename[128], eskills[256];
        json_escape(col(stmt,1), ename, sizeof(ename));
        json_escape(col(stmt,5), eskills, sizeof(eskills));
        snprintf(buf, sizeof(buf), "%s{\"id\":%d,\"name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"status\":\"%s\",\"skills\":\"%s\",\"calls_handled\":%d,\"created_at\":\"%s\"}",
            first ? "" : ",",
            sqlite3_column_int(stmt, 0),
            ename, col(stmt,2), col(stmt,3), col(stmt,4), eskills,
            sqlite3_column_int(stmt, 6), col(stmt,7));
        if (strlen(out) + strlen(buf) < (size_t)out_sz - 4)
            strcat(out, buf);
        first = 0;
    }
    strcat(out, "]");
    sqlite3_finalize(stmt);
}

void agent_get_by_id(int id, char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id,name,phone,email,status,skills,calls_handled,created_at FROM agents WHERE id=?;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        char ename[128], eskills[256];
        json_escape(col(stmt,1), ename, sizeof(ename));
        json_escape(col(stmt,5), eskills, sizeof(eskills));
        snprintf(out, out_sz,
            "{\"id\":%d,\"name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"status\":\"%s\",\"skills\":\"%s\",\"calls_handled\":%d,\"created_at\":\"%s\"}",
            sqlite3_column_int(stmt,0), ename, col(stmt,2), col(stmt,3), col(stmt,4), eskills,
            sqlite3_column_int(stmt,6), col(stmt,7));
    } else {
        snprintf(out, out_sz, "{\"error\":\"Agent not found\"}");
    }
    sqlite3_finalize(stmt);
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

void agent_create(const char *body, char *out, int out_sz) {
    char name[128]={}, phone[64]={}, email[128]={}, skills[256]={};
    extract_json_str(body, "name",   name,   sizeof(name));
    extract_json_str(body, "phone",  phone,  sizeof(phone));
    extract_json_str(body, "email",  email,  sizeof(email));
    extract_json_str(body, "skills", skills, sizeof(skills));

    if (!name[0]) { snprintf(out, out_sz, "{\"error\":\"name required\"}"); return; }

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO agents(name,phone,email,skills,status) VALUES(?,?,?,?,'AVAILABLE');";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name,   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, phone,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, skills, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        int new_id = (int)sqlite3_last_insert_rowid(db_get());
        snprintf(out, out_sz, "{\"success\":true,\"id\":%d}", new_id);
    } else {
        snprintf(out, out_sz, "{\"error\":\"Insert failed\"}");
    }
    sqlite3_finalize(stmt);
}

void agent_update(int id, const char *body, char *out, int out_sz) {
    char name[128]={}, phone[64]={}, email[128]={}, skills[256]={}, status[32]={};
    extract_json_str(body, "name",   name,   sizeof(name));
    extract_json_str(body, "phone",  phone,  sizeof(phone));
    extract_json_str(body, "email",  email,  sizeof(email));
    extract_json_str(body, "skills", skills, sizeof(skills));
    extract_json_str(body, "status", status, sizeof(status));

    sqlite3_stmt *stmt;
    const char *sql = "UPDATE agents SET name=COALESCE(NULLIF(?,''),name), phone=COALESCE(NULLIF(?,''),phone), email=COALESCE(NULLIF(?,''),email), skills=COALESCE(NULLIF(?,''),skills), status=COALESCE(NULLIF(?,''),status) WHERE id=?;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name,   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, phone,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, email,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, skills, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, id);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        snprintf(out, out_sz, "{\"success\":true}");
    else
        snprintf(out, out_sz, "{\"error\":\"Update failed\"}");
    sqlite3_finalize(stmt);
}

void agent_delete(int id, char *out, int out_sz) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(), "DELETE FROM agents WHERE id=?;", -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        snprintf(out, out_sz, "{\"success\":true}");
    else
        snprintf(out, out_sz, "{\"error\":\"Delete failed\"}");
    sqlite3_finalize(stmt);
}

void agent_update_status(int id, const char *status, char *out, int out_sz) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_get(), "UPDATE agents SET status=? WHERE id=?;", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    if (sqlite3_step(stmt) == SQLITE_DONE)
        snprintf(out, out_sz, "{\"success\":true}");
    else
        snprintf(out, out_sz, "{\"error\":\"Status update failed\"}");
    sqlite3_finalize(stmt);
}
