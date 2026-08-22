#include "ps14/ps14.h"
#include "ps14/logger.h"
#include "ps14/network.h"

#include <winsock2.h>

static SOCKET g_listen_socket = INVALID_SOCKET;

bool ps14_network_handler_init(u16 port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
    g_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listen_socket == INVALID_SOCKET) return false;
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(g_listen_socket, (sockaddr*)&addr, sizeof(addr)) != 0) return false;
    if (listen(g_listen_socket, SOMAXCONN) != 0) return false;
    ps14_log_info("Network handler listening on port %d", port);
    return true;
}

void ps14_network_handler_shutdown() {
    if (g_listen_socket != INVALID_SOCKET) {
