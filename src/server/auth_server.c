#include "ps14/ps14.h"
#include "ps14/logger.h"
#include "ps14/network.h"
#include "ps14/hash.h"

#include <windows.h>

typedef struct {
    char username[64];
    char password_hash[64];
    char session_token[64];
    u64 expiry_time;
} AuthUser;

typedef struct {
    char token[64];
    u32 user_id;
    u64 expiry;
} SessionToken;

static AuthUser g_users[1024];
static u32 g_user_count = 0;
static SessionToken g_tokens[1024];
static u32 g_token_count = 0;

bool ps14_auth_server_init() {
    ps14_log_info("Auth server initialized");
    return true;
}

void ps14_auth_server_shutdown() {
    ps14_log_info("Auth server shutdown");
}

bool ps14_auth_server_register_user(const char* username, const char* password) {
    if (!username || !password) return false;
    if (g_user_count >= 1024) return false;
    strncpy(g_users[g_user_count].username, username, 63);
    ps14_hash_sha256(password, strlen(password), g_users[g_user_count].password_hash);
    g_users[g_user_count].expiry_time = 0;
    g_users[g_user_count].session_token[0] = 0;
    g_user_count++;
    ps14_log_info("User registered: %s", username);
    return true;
}

bool ps14_auth_server_login(const char* username, const char* password, char* token_out) {
    char hash[64];
    ps14_hash_sha256(password, strlen(password), hash);
    for (u32 i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].username, username) == 0 && strcmp(g_users[i].password_hash, hash) == 0) {
            ps14_hash_sha256(username, strlen(username), g_users[i].session_token);
            if (token_out) strncpy(token_out, g_users[i].session_token, 64);
            g_users[i].expiry_time = time(NULL) + 3600;
            ps14_log_info("User logged in: %s", username);
            return true;
        }
    }
    ps14_log_warning("Login failed for: %s", username);
    return false;
}

bool ps14_auth_server_validate_token(const char* token) {
    for (u32 i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].session_token, token) == 0) {
            if (g_users[i].expiry_time > time(NULL)) {
                return true;
            }
        }
    }
    return false;
}
