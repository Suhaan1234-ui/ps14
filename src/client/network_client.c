#include "ps14/ps14.h"
#include "ps14/network.h"
#include "ps14/logger.h"

#include <windows.h>

static SOCKET g_server_socket = INVALID_SOCKET;
static bool g_connected = false;

bool ps14_network_client_connect(const char* server_ip, u16 port) {
    ps14_log_info("Connecting to server: %s:%d", server_ip, port);
    g_server_socket = ps14_network_connect(server_ip, port);
    if (g_server_socket != INVALID_SOCKET) {
        g_connected = true;
        ps14_log_info("Connected to server");
        return true;
    }
    ps14_log_error("Failed to connect to server");
    return false;
}

void ps14_network_client_disconnect() {
    if (g_server_socket != INVALID_SOCKET) {
        closesocket(g_server_socket);
        g_server_socket = INVALID_SOCKET;
    }
    g_connected = false;
    ps14_log_info("Disconnected from server");
}

bool ps14_network_client_is_connected() {
    return g_connected;
}

bool ps14_network_client_send_auth_token(const char* token, u32 token_len) {
    if (!g_connected || !token || token_len == 0) return false;
    return ps14_network_send(g_server_socket, token, token_len);
}

bool ps14_network_client_send_state_update(void* state_data, u32 data_size) {
    if (!g_connected || !state_data || data_size == 0) return false;
    return ps14_network_send(g_server_socket, state_data, data_size);
}

bool ps14_network_client_receive(void* buffer, u32 buffer_size, u32* bytes_received) {
    if (!g_connected || !buffer || buffer_size == 0) return false;
    return ps14_network_receive(g_server_socket, buffer, buffer_size, bytes_received);
}
