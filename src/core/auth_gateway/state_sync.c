#include "ps14/auth.h"
#include "ps14/network.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

// State synchronization configuration
typedef struct {
    u32 last_received_sequence;
    u32 last_sent_sequence;
    bool sync_enabled;
    Ps14Mutex mutex;
    Ps14SocketHandle socket;
    char server_address[260];
    u16 server_port;
} StateSyncState;

static StateSyncState g_state_sync;

// State update packet structure
#pragma pack(push, 1)
typedef struct {
    u32 sequence;
    u32 timestamp;
    u16 data_size;
    u16 flags;
    u8 data[]; // Variable-length state data
} Ps14StatePacket;
#pragma pack(pop)

// Conflict resolution strategy
typedef enum {
    PS14_CONFLICT_SERVER_WINS = 0,
    PS14_CONFLICT_CLIENT_WINS,
    PS14_CONFLICT_MERGE,
    PS14_CONFLICT_LAST_WRITER_WINS
} Ps14ConflictStrategy;

// Initialize state synchronization
PS14_API i32 ps14_state_sync_init(void) {
    memset(&g_state_sync, 0, sizeof(StateSyncState));
    
    ps14_mutex_init(&g_state_sync.mutex);
    g_state_sync.socket = PS14_INVALID_SOCKET;
    g_state_sync.sync_enabled = true;
    g_state_sync.last_received_sequence = 0;
    g_state_sync.last_sent_sequence = 0;
    
    // Get configuration
    Ps14Config* config = ps14_config_get();
    if (config) {
        strncpy(g_state_sync.server_address, config->auth.server_address, 259);
        g_state_sync.server_address[259] = '\0';
        g_state_sync.server_port = config->auth.server_port;
    } else {
        strncpy(g_state_sync.server_address, "localhost", 259);
        g_state_sync.server_address[259] = '\0';
        g_state_sync.server_port = 8080;
    }
    
    PS14_LOG_INFO("State synchronization initialized (server: %s:%u)", 
                  g_state_sync.server_address, g_state_sync.server_port);
    return PS14_SUCCESS;
}

// Shutdown state synchronization
PS14_API void ps14_state_sync_shutdown(void) {
    ps14_mutex_lock(&g_state_sync.mutex);
    
    if (g_state_sync.socket != PS14_INVALID_SOCKET) {
        ps14_socket_close(g_state_sync.socket);
        g_state_sync.socket = PS14_INVALID_SOCKET;
    }
    
    g_state_sync.sync_enabled = false;
    ps14_mutex_unlock(&g_state_sync.mutex);
    
    ps14_mutex_destroy(&g_state_sync.mutex);
    PS14_LOG_INFO("State synchronization shutdown");
}

// Connect to state server
static i32 state_sync_connect(void) {
    ps14_mutex_lock(&g_state_sync.mutex);
    
    if (g_state_sync.socket != PS14_INVALID_SOCKET) {
        ps14_mutex_unlock(&g_state_sync.mutex);
        return PS14_SUCCESS;
    }
    
    // Create socket
    g_state_sync.socket = ps14_socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_state_sync.socket == PS14_INVALID_SOCKET) {
        ps14_mutex_unlock(&g_state_sync.mutex);
        PS14_LOG_ERROR("Failed to create state sync socket");
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    // Connect
    if (ps14_socket_connect(g_state_sync.socket, g_state_sync.server_address, 
                           g_state_sync.server_port, 5000) != PS14_SUCCESS) {
        ps14_socket_close(g_state_sync.socket);
        g_state_sync.socket = PS14_INVALID_SOCKET;
        ps14_mutex_unlock(&g_state_sync.mutex);
        PS14_LOG_ERROR("Failed to connect to state server: %s:%u", 
                      g_state_sync.server_address, g_state_sync.server_port);
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    ps14_mutex_unlock(&g_state_sync.mutex);
    PS14_LOG_INFO("Connected to state server: %s:%u", 
                  g_state_sync.server_address, g_state_sync.server_port);
    return PS14_SUCCESS;
}

// Ensure connection to server
static i32 state_sync_ensure_connected(void) {
    if (!g_state_sync.sync_enabled) {
        return PS14_ERROR_NOT_INITIALIZED;
    }
    
    if (g_state_sync.socket == PS14_INVALID_SOCKET) {
        return state_sync_connect();
    }
    
    return PS14_SUCCESS;
}

// Send state update to server
PS14_API i32 ps14_state_send_update(u32 sequence, const void* state_data, usize data_size) {
    if (!state_data || data_size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (data_size > 65535) { // Max size for our packet format
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    i32 result = state_sync_ensure_connected();
    if (result != PS14_SUCCESS) {
        return result;
    }
    
    // Build packet
    usize packet_size = sizeof(Ps14StatePacket) + data_size;
    Ps14StatePacket* packet = (Ps14StatePacket*)malloc(packet_size);
    if (!packet) {
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    packet->sequence = sequence;
    packet->timestamp = (u32)(ps14_thread_get_tick_count() & 0xFFFFFFFF);
    packet->data_size = (u16)data_size;
    packet->flags = 0x0001; // STATE_UPDATE flag
    memcpy(packet->data, state_data, data_size);
    
    // Send packet
    i32 bytes_sent = ps14_socket_send(g_state_sync.socket, packet, packet_size, 0);
    free(packet);
    
    if (bytes_sent != (i32)packet_size) {
        PS14_LOG_WARNING("Failed to send full state update packet");
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    ps14_mutex_lock(&g_state_sync.mutex);
    g_state_sync.last_sent_sequence = sequence;
    ps14_mutex_unlock(&g_state_sync.mutex);
    
    PS14_LOG_DEBUG("Sent state update (seq: %u, size: %zu)", sequence, data_size);
    return PS14_SUCCESS;
}

// Receive state update from server
PS14_API i32 ps14_state_receive_update(u32* sequence, void* state_data, usize* data_size) {
    if (!sequence || !state_data || !data_size || *data_size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    i32 result = state_sync_ensure_connected();
    if (result != PS14_SUCCESS) {
        return result;
    }
    
    // Read packet header
    Ps14StatePacket header;
    i32 bytes_received = ps14_socket_receive(g_state_sync.socket, &header, sizeof(header), 0);
    
    if (bytes_received != (i32)sizeof(header)) {
        PS14_LOG_WARNING("Failed to receive state packet header");
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    // Validate packet
    if (header.data_size == 0 || header.data_size > *data_size) {
        PS14_LOG_WARNING("Invalid state packet size: %u (max: %zu)", 
                        header.data_size, *data_size);
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    // Read packet data
    bytes_received = ps14_socket_receive(g_state_sync.socket, state_data, header.data_size, 0);
    if (bytes_received != (i32)header.data_size) {
        PS14_LOG_WARNING("Failed to receive state packet data");
        return PS14_ERROR_NETWORK_ERROR;
    }
    
    *sequence = header.sequence;
    *data_size = header.data_size;
    
    ps14_mutex_lock(&g_state_sync.mutex);
    g_state_sync.last_received_sequence = header.sequence;
    ps14_mutex_unlock(&g_state_sync.mutex);
    
    PS14_LOG_DEBUG("Received state update (seq: %u, size: %u)", header.sequence, header.data_size);
    return PS14_SUCCESS;
}

// Resolve conflict based on strategy
PS14_API i32 ps14_state_resolve_conflict(void) {
    // For now, implement a simple strategy: server wins
    // This is the most common approach in competitive games
    
    Ps14Config* config = ps14_config_get();
    Ps14ConflictStrategy strategy = PS14_CONFLICT_SERVER_WINS;
    
    // In a real implementation, we would:
    // 1. Fetch the latest state from server
    // 2. Compare with local state
    // 3. Apply resolution strategy
    
    PS14_LOG_INFO("Conflict detected - resolving with strategy: %d", strategy);
    
    // For now, just log and return success
    // The actual resolution would happen in the game-specific code
    
    return PS14_SUCCESS;
}

// Get current synchronization status
PS14_API bool ps14_state_sync_is_connected(void) {
    ps14_mutex_lock(&g_state_sync.mutex);
    bool connected = (g_state_sync.socket != PS14_INVALID_SOCKET) && g_state_sync.sync_enabled;
    ps14_mutex_unlock(&g_state_sync.mutex);
    return connected;
}

// Get last received sequence number
PS14_API u32 ps14_state_sync_get_last_sequence(void) {
    ps14_mutex_l
