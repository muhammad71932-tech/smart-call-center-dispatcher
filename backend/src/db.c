#include "db.h"
#include <stdio.h>
#include <stdlib.h>

static sqlite3 *g_db = NULL;

int db_init(const char *path) {
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    const char *schema =
        "PRAGMA journal_mode=WAL;"

        "CREATE TABLE IF NOT EXISTS agents ("
        "  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name          TEXT    NOT NULL,"
        "  phone         TEXT    DEFAULT '',"
        "  email         TEXT    DEFAULT '',"
        "  status        TEXT    DEFAULT 'OFFLINE',"
        "  skills        TEXT    DEFAULT '',"
        "  calls_handled INTEGER DEFAULT 0,"
        "  created_at    DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"

        "CREATE TABLE IF NOT EXISTS calls ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  caller_id    TEXT NOT NULL,"
        "  caller_name  TEXT DEFAULT 'Unknown',"
        "  priority     TEXT DEFAULT 'NORMAL',"
        "  category     TEXT DEFAULT 'general',"
        "  status       TEXT DEFAULT 'WAITING',"
        "  agent_id     INTEGER REFERENCES agents(id),"
        "  notes        TEXT DEFAULT '',"
        "  wait_time    INTEGER DEFAULT 0,"
        "  handle_time  INTEGER DEFAULT 0,"
        "  created_at   DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  resolved_at  DATETIME"
        ");"

        "CREATE TABLE IF NOT EXISTS daily_stats ("
        "  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  date            TEXT UNIQUE,"
        "  total_calls     INTEGER DEFAULT 0,"
        "  resolved_calls  INTEGER DEFAULT 0,"
        "  abandoned_calls INTEGER DEFAULT 0,"
        "  avg_wait_time   REAL    DEFAULT 0,"
        "  avg_handle_time REAL    DEFAULT 0"
        ");";

    char *err = NULL;
    if (sqlite3_exec(g_db, schema, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "Schema error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }

    return 0;
}

void db_close(void) {
    if (g_db) sqlite3_close(g_db);
}

sqlite3 *db_get(void) {
    return g_db;
}
