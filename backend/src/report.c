#include "report.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

static const char *col(sqlite3_stmt *s, int i) {
    const char *v = (const char *)sqlite3_column_text(s, i);
    return v ? v : "";
}

void report_dashboard(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    int total=0, waiting=0, active=0, completed=0, abandoned=0;
    int avail=0, busy=0, on_break=0, offline=0;

    sqlite3_prepare_v2(db_get(),
        "SELECT status, COUNT(*) FROM calls GROUP BY status;", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *s = col(stmt,0);
        int c = sqlite3_column_int(stmt,1);
        total += c;
        if (!strcmp(s,"WAITING"))   waiting   = c;
        if (!strcmp(s,"ACTIVE"))    active    = c;
        if (!strcmp(s,"COMPLETED")) completed = c;
        if (!strcmp(s,"ABANDONED")) abandoned = c;
    }
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db_get(),
        "SELECT status, COUNT(*) FROM agents GROUP BY status;", -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *s = col(stmt,0);
        int c = sqlite3_column_int(stmt,1);
        if (!strcmp(s,"AVAILABLE")) avail    = c;
        if (!strcmp(s,"BUSY"))      busy     = c;
        if (!strcmp(s,"BREAK"))     on_break = c;
        if (!strcmp(s,"OFFLINE"))   offline  = c;
    }
    sqlite3_finalize(stmt);

    double avg_wait=0, avg_handle=0;
    sqlite3_prepare_v2(db_get(),
        "SELECT AVG(wait_time), AVG(handle_time) FROM calls WHERE status='COMPLETED';",
        -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avg_wait   = sqlite3_column_double(stmt,0);
        avg_handle = sqlite3_column_double(stmt,1);
    }
    sqlite3_finalize(stmt);

    snprintf(out, out_sz,
        "{\"total_calls\":%d,\"waiting\":%d,\"active\":%d,\"completed\":%d,\"abandoned\":%d,"
        "\"agents_available\":%d,\"agents_busy\":%d,\"agents_break\":%d,\"agents_offline\":%d,"
        "\"avg_wait_time\":%.1f,\"avg_handle_time\":%.1f}",
        total, waiting, active, completed, abandoned,
        avail, busy, on_break, offline, avg_wait, avg_handle);
}

void report_daily(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT date(created_at) as day, COUNT(*) as total,"
        " SUM(CASE WHEN status='COMPLETED' THEN 1 ELSE 0 END) as resolved,"
        " SUM(CASE WHEN status='ABANDONED' THEN 1 ELSE 0 END) as abandoned,"
        " AVG(wait_time) as avg_wait, AVG(handle_time) as avg_handle"
        " FROM calls GROUP BY day ORDER BY day DESC LIMIT 30;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    strcpy(out, "[");
    int first=1;
    char buf[256];
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(buf, sizeof(buf),
            "%s{\"date\":\"%s\",\"total\":%d,\"resolved\":%d,\"abandoned\":%d,\"avg_wait\":%.1f,\"avg_handle\":%.1f}",
            first?"":",",
            col(stmt,0), sqlite3_column_int(stmt,1),
            sqlite3_column_int(stmt,2), sqlite3_column_int(stmt,3),
            sqlite3_column_double(stmt,4), sqlite3_column_double(stmt,5));
        if (strlen(out)+strlen(buf) < (size_t)out_sz-4)
            strcat(out, buf);
        first=0;
    }
    strcat(out, "]");
    sqlite3_finalize(stmt);
}

void report_agents(char *out, int out_sz) {
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT a.id, a.name, a.status, a.skills, a.calls_handled,"
        " COUNT(c.id) as total_calls,"
        " AVG(c.handle_time) as avg_handle,"
        " SUM(CASE WHEN c.status='COMPLETED' THEN 1 ELSE 0 END) as resolved"
        " FROM agents a LEFT JOIN calls c ON a.id=c.agent_id"
        " GROUP BY a.id ORDER BY a.calls_handled DESC;";
    sqlite3_prepare_v2(db_get(), sql, -1, &stmt, NULL);

    strcpy(out, "[");
    int first=1;
    char buf[512];
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        snprintf(buf, sizeof(buf),
            "%s{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\",\"skills\":\"%s\","
            "\"calls_handled\":%d,\"total_calls\":%d,\"avg_handle\":%.1f,\"resolved\":%d}",
            first?"":",",
            sqlite3_column_int(stmt,0), col(stmt,1), col(stmt,2), col(stmt,3),
            sqlite3_column_int(stmt,4), sqlite3_column_int(stmt,5),
            sqlite3_column_double(stmt,6), sqlite3_column_int(stmt,7));
        if (strlen(out)+strlen(buf) < (size_t)out_sz-4)
            strcat(out, buf);
        first=0;
    }
    strcat(out, "]");
    sqlite3_finalize(stmt);
}
