#include "ps14/ps14.h"
#include "ps14/logger.h"

typedef struct {
    char session_id[64];
    u32 user_id;
    u64 create_time;
    u64 expiry_time;
} SessionEntry;

static SessionEntry g_sessions[1024];
static u32 g_session_count = 0;

bool ps14_session_store_init() {
    return true;
}

void ps14_session_store_shutdown() {
}

bool ps14_session_store_create(u32 user_id, const char* session_id, u64 expiry_seconds) {
    if (g_session_count >= 1024) return false;
    strncpy(g_sessions[g_session_count].session_id, session_id, 63);
    g_sessions[g_session_count].user_id = user_id;
    g_sessions[g_session_count].create_time = time(NULL);
    g_sessions[g_session_count].expiry_time = time(NULL) + expiry_seconds;
    g_session_count++;
    return true;
}

bool ps14_session_store_destroy(const char* session_id) {
    for (u32 i = 0; i < g_session_count; i++) {
        if (strcmp(g_sessions[i].session_id, session_id) == 0) {
            for (u32 j = i; j < g_session_count - 1; j++) {
                g_sessions[j] = g_sessions[j + 1];
            }
            g_session_count--;
            return true;
        }
    }
    return false;
}

SessionEntry* ps14_session_store_get(const char* session_id) {
    for (u32 i = 0; i < g_session_count; i++) {
        if (strcmp(g_sessions[i].session_id, session_id) == 0) {
            return &g_sessions[i];
        }
    }
    return NULL;
}
