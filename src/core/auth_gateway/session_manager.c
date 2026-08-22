#include "ps14/auth.h"
#include "ps14/logger.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Global session list
static Ps14Session* g_sessions = NULL;
static Ps14Mutex g_sessions_mutex;

// Generate unique session ID
static void generate_session_id(char* buffer, usize size) {
    // Simple random generation for now
    // In production, use cryptographically secure RNG
    srand((u32)time(NULL));
    for (usize i = 0; i < size - 1; i++) {
        char c = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"[rand() % 62];
        buffer[i] = c;
    }
    buffer[size - 1] = '\0';
}

// Generate hardware ID (for device fingerprinting)
static void generate_hw_id(char* buffer, usize size) {
    // This would use actual hardware information in production
    // For now, just generate a random ID
    generate_session_id(buffer, size);
}

// Initialize session manager
i32 ps14_session_manager_init(void) {
    ps14_mutex_init(&g_sessions_mutex);
    PS14_LOG_INFO("Session manager initialized");
    return PS14_SUCCESS;
}

// Shutdown session manager
void ps14_session_manager_shutdown(void) {
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session* curr = g_sessions;
    while (curr) {
        Ps14Session* next = curr->next;
        free(curr);
        curr = next;
    }
    g_sessions = NULL;
    ps14_mutex_unlock(&g_sessions_mutex);
    ps14_mutex_destroy(&g_sessions_mutex);
    PS14_LOG_INFO("Session manager shutdown");
}

// Create a new session
Ps14Session* ps14_session_create(const char* user_id, const char* username, u64 timeout_seconds) {
    if (!user_id) {
        return NULL;
    }
    
    Ps14Session* session = (Ps14Session*)malloc(sizeof(Ps14Session));
    if (!session) {
        PS14_LOG_ERROR("Failed to allocate session");
        return NULL;
    }
    
    // Generate session ID
    generate_session_id(session->session_id, sizeof(session->session_id));
    
    strncpy(session->user_id, user_id, sizeof(session->user_id) - 1);
    session->user_id[sizeof(session->user_id) - 1] = '\0';
    
    if (username) {
        strncpy(session->username, username, sizeof(session->username) - 1);
        session->username[sizeof(session->username) - 1] = '\0';
    } else {
        session->username[0] = '\0';
    }
    
    // Set timestamps
    time_t now = time(NULL);
    session->created_at = (u64)now;
    session->expires_at = (u64)now + timeout_seconds;
    
    // Set IP and HW ID (empty for now)
    session->ip_address[0] = '\0';
    generate_hw_id(session->hw_id, sizeof(session->hw_id));
    
    session->state = PS14_SESSION_STATE_ACTIVE;
    session->rights = 0xFFFFFFFF; // All rights by default
    
    // Add to list
    ps14_mutex_lock(&g_sessions_mutex);
    session->next = g_sessions;
    g_sessions = session;
    ps14_mutex_unlock(&g_sessions_mutex);
    
    PS14_LOG_INFO("Session created: %s for user: %s", session->session_id, user_id);
    return session;
}

// Destroy a session
void ps14_session_destroy(Ps14Session* session) {
    if (!session) {
        return;
    }
    
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session** curr = &g_sessions;
    while (*curr) {
        if (*curr == session) {
            *curr = session->next;
            break;
        }
        curr = &(*curr)->next;
    }
    ps14_mutex_unlock(&g_sessions_mutex);
    
    PS14_LOG_INFO("Session destroyed: %s", session->session_id);
    free(session);
}

// Find a session by ID
Ps14Session* ps14_session_find(const char* session_id) {
    if (!session_id) {
        return NULL;
    }
    
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session* curr = g_sessions;
    while (curr) {
        if (strcmp(curr->session_id, session_id) == 0) {
            ps14_mutex_unlock(&g_sessions_mutex);
            return curr;
        }
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_sessions_mutex);
    return NULL;
}

// Find a session by user ID
Ps14Session* ps14_session_find_by_user(const char* user_id) {
    if (!user_id) {
        return NULL;
    }
    
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session* curr = g_sessions;
    while (curr) {
        if (strcmp(curr->user_id, user_id) == 0) {
            ps14_mutex_unlock(&g_sessions_mutex);
            return curr;
        }
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_sessions_mutex);
    return NULL;
}

// Check if a session is valid (not expired, not banned)
bool ps14_session_is_valid(Ps14Session* session) {
    if (!session) {
        return false;
    }
    
    if (session->state == PS14_SESSION_STATE_BANNED || session->state == PS14_SESSION_STATE_INVALID) {
        return false;
    }
    
    if (session->state == PS14_SESSION_STATE_EXPIRED) {
        return false;
    }
    
    // Check expiration
    time_t now = time(NULL);
    if ((u64)now > session->expires_at) {
        session->state = PS14_SESSION_STATE_EXPIRED;
        PS14_LOG_INFO("Session expired: %s", session->session_id);
        return false;
    }
    
    return true;
}

// Get current session count
u32 ps14_session_count(void) {
    u32 count = 0;
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session* curr = g_sessions;
    while (curr) {
        count++;
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_sessions_mutex);
    return count;
}

// Ban a session
void ps14_session_ban(Ps14Session* session, const char* reason) {
    if (!session) {
        return;
    }
    
    session->state = PS14_SESSION_STATE_BANNED;
    PS14_LOG_WARNING("Session banned: %s (reason: %s)", session->session_id, reason ? reason : "unknown");
}

// Ban a session by ID
void ps14_session_ban_by_id(const char* session_id, const char* reason) {
    Ps14Session* session = ps14_session_find(session_id);
    if (session) {
        ps14_session_ban(session, reason);
    }
}

// Invalidate a session
void ps14_session_invalidate(Ps14Session* session) {
    if (!session) {
        return;
    }
    session->state = PS14_SESSION_STATE_INVALID;
    PS14_LOG_INFO("Session invalidated: %s", session->session_id);
}

// Cleanup expired sessions
u32 ps14_session_cleanup_expired(void) {
    u32 count = 0;
    time_t now = time(NULL);
    
    ps14_mutex_lock(&g_sessions_mutex);
    Ps14Session** curr = &g_sessions;
    while (*curr) {
        if ((u64)now > (*curr)->expires_at) {
            Ps14Session* to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            count++;
        } else {
            curr = &(*curr)->next;
        }
    }
    ps14_mutex_unlock(&g_sessions_mutex);
    
    PS14_LOG_INFO("Cleaned up %u expired sessions", count);
    return count;
}
