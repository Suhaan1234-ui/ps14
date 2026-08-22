#include "ps14/auth.h"
#include "ps14/network.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <string.h>
#include <stdlib.h>

// Connection state
static Ps14SocketHandle g_socket = PS14_INVALID_SOCKET;
static char g_server_address[PS14_MAX_PATH] = "localhost";
static u16 g_server_port = 8080;
static bool g_connected = false;
static Ps14Mutex g_connection_mutex;

// Pending callbacks (for async operations)
typedef struct PendingAuth {
    Ps14AuthCallback callback;
    void* user_data;
    struct PendingAuth* next;
} PendingAuth;

static PendingAuth* g_pending_auths = NULL;
static Ps14Mutex g_pending_mutex;

// Initialize authentication client
i32 ps14_auth_init(const char* server_address, u16 port) {
    ps14_mutex_init(&g_connection_mutex);
    ps14_mutex_init(&g_pending_mutex);
    
    if (server_address) {
        strncpy(g_server_address, server_address, PS14_MAX_PATH - 1);
        g_server_address[PS14_MAX_PATH - 1] = '\0';
    }
    g_server_port = port;
    
    // Initialize network
    ps14_network_init();
    
    PS14_LOG_INFO("Authentication client initialized (server: %s:%u)", g_server_address, g_server_port);
    return PS14_SUCCESS;
}

// Shutdown authentication client
void ps14_auth_shutdown(void) {
    ps14_auth_disconnect();
    ps14_mutex_destroy(&g_connection_mutex);
    ps14_mutex_destroy(&g_pending_mutex);
    ps14_network_shutdown();
    PS14_LOG_INFO("Authentication client shutdown");
}

// Connect to server
i32 ps14_auth_connect(void) {
    ps14_mutex_lock(&g_connection_mutex);
    
    if (g_connected) {
        ps14_mutex_unlock(&g_connection_mutex);
        return PS14_SUCCESS;
    }
    
    // Close existing connection
    if (g_socket != PS14_INVALID_SOCKET) {
        ps14_socket_close(g_socket);
        g_socket = PS14_INVALID_SOCKET;
    }
    
    // Create socket
    g_socket = ps14_socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket == PS14_INVALID_SOCKET) {
        ps14_mutex_unlock(&g_connection_mutex);
        PS14_LOG_ERROR("Failed to create socket for auth connection");
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    // Connect
    if (ps14_socket_connect(g_socket, g_server_address, g_server_port, 5000) != PS14_SUCCESS) {
        ps14_socket_close(g_socket);
        g_socket = PS14_INVALID_SOCKET;
        ps14_mutex_unlock(&g_connection_mutex);
        PS14_LOG_ERROR("Failed to connect to auth server: %s:%u", g_server_address, g_server_port);
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    g_connected = true;
    ps14_mutex_unlock(&g_connection_mutex);
    PS14_LOG_INFO("Connected to auth server: %s:%u", g_server_address, g_server_port);
    return PS14_SUCCESS;
}

// Disconnect from server
void ps14_auth_disconnect(void) {
    ps14_mutex_lock(&g_connection_mutex);
    
    if (g_socket != PS14_INVALID_SOCKET) {
        ps14_socket_close(g_socket);
        g_socket = PS14_INVALID_SOCKET;
    }
    g_connected = false;
    
    ps14_mutex_unlock(&g_connection_mutex);
    PS14_LOG_INFO("Disconnected from auth server");
}

// Check if connected
bool ps14_auth_is_connected(void) {
    ps14_mutex_lock(&g_connection_mutex);
    bool connected = g_connected;
    ps14_mutex_unlock(&g_connection_mutex);
    return connected;
}

// Login (async)
i32 ps14_auth_login(const char* username, const char* password, Ps14AuthCallback callback, void* user_data) {
    if (!username || !password) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!callback) {
        // Synchronous login not implemented yet
        return PS14_ERROR_NOT_INITIALIZED;
    }
    
    // Connect if not already connected
    if (ps14_auth_connect() != PS14_SUCCESS) {
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    // For async, we'd normally send the request and store the callback
    // For now, we'll simulate a successful login
    
    Ps14AuthResult result;
    result.code = PS14_SUCCESS;
    snprintf(result.message, PS14_MAX_MESSAGE, "Login successful for: %s", username);
    result.session = ps14_session_create("temp_user_id", username, 3600);
    
    callback(&result, user_data);
    
    PS14_LOG_INFO("Login request for: %s", username);
    return PS14_SUCCESS;
}

// Logout
i32 ps14_auth_logout(const char* session_id) {
    if (!session_id) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (ps14_auth_connect() != PS14_SUCCESS) {
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    // Find and destroy session
    Ps14Session* session = ps14_session_find(session_id);
    if (session) {
        ps14_session_destroy(session);
    }
    
    PS14_LOG_INFO("Logout request for session: %s", session_id);
    return PS14_SUCCESS;
}

// Validate session
i32 ps14_auth_validate_session(const char* session_id) {
    if (!session_id) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    Ps14Session* session = ps14_session_find(session_id);
    if (!session) {
        PS14_LOG_WARNING("Session not found: %s", session_id);
        return PS14_ERROR_NOT_FOUND;
    }
    
    if (!ps14_session_is_valid(session)) {
        PS14_LOG_WARNING("Session expired: %s", session_id);
        return PS14_ERROR_AUTHENTICATION_FAILED;
    }
    
    PS14_LOG_DEBUG("Session validated: %s", session_id);
    return PS14_SUCCESS;
}

// Refresh session
i32 ps14_auth_refresh_session(const char* session_id) {
    if (!session_id) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    Ps14Session* session = ps14_session_find(session_id);
    if (!session) {
        return PS14_ERROR_NOT_FOUND;
    }
    
    // Extend expiration
    Ps14Config* config = ps14_config_get();
    u64 timeout = config ? config->auth.session_timeout_seconds : 3600;
    session->expires_at = session->created_at + timeout;
    
    PS14_LOG_DEBUG("Session refreshed: %s", session_id);
    return PS14_SUCCESS;
}
