#ifndef PS14_AUTH_H
#define PS14_AUTH_H

#pragma once

#include "ps14.h"

#ifdef __cplusplus
extern "C" {
#endif

// Session states
typedef enum {
    PS14_SESSION_STATE_INVALID = 0,
    PS14_SESSION_STATE_PENDING,
    PS14_SESSION_STATE_ACTIVE,
    PS14_SESSION_STATE_EXPIRED,
    PS14_SESSION_STATE_BANNED
} Ps14SessionState;

// Session structure
typedef struct Ps14Session {
    char session_id[64];
    char user_id[64];
    char username[PS14_MAX_NAME];
    u64 created_at;
    u64 expires_at;
    char ip_address[48];
    char hw_id[64];
    Ps14SessionState state;
    u32 rights;
    struct Ps14Session* next;
} Ps14Session;

// Authentication result
typedef struct Ps14AuthResult {
    i32 code;
    char message[PS14_MAX_MESSAGE];
    Ps14Session* session;
} Ps14AuthResult;

// Authentication callback
typedef void (*Ps14AuthCallback)(Ps14AuthResult* result, void* user_data);

// Authentication client
PS14_API i32 ps14_auth_init(const char* server_address, u16 port);
PS14_API void ps14_auth_shutdown(void);

PS14_API i32 ps14_auth_login(const char* username, const char* password, Ps14AuthCallback callback, void* user_data);
PS14_API i32 ps14_auth_logout(const char* session_id);
PS14_API i32 ps14_auth_validate_session(const char* session_id);
PS14_API i32 ps14_auth_refresh_session(const char* session_id);

// Session management
PS14_API Ps14Session* ps14_session_create(const char* user_id, const char* username, u64 timeout_seconds);
PS14_API void ps14_session_destroy(Ps14Session* session);
PS14_API Ps14Session* ps14_session_find(const char* session_id);
PS14_API bool ps14_session_is_valid(Ps14Session* session);

// State synchronization
PS14_API i32 ps14_state_sync_init(void);
PS14_API void ps14_state_sync_shutdown(void);
PS14_API i32 ps14_state_send_update(u32 sequence, const void* state_data, usize data_size);
PS14_API i32 ps14_state_receive_update(u32* sequence, void* state_data, usize* data_size);
PS14_API i32 ps14_state_resolve_conflict(void);

// Token management
PS14_API i32 ps14_token_generate(char* buffer, usize buffer_size);
PS14_API i32 ps14_token_validate(const char* token);
PS14_API i32 ps14_token_parse(const char* token, char* user_id, usize user_id_size, char* username, usize username_size);

#ifdef __cplusplus
}
#endif
#endif
