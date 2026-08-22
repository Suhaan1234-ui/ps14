#include "ps14/network.h"
#include <string.h>
#include <stdlib.h>

static bool g_net_init = false;
#ifdef PS14_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#endif

i32 ps14_network_init(void) {
    if (g_net_init) return PS14_SUCCESS;
    #ifdef PS14_PLATFORM_WINDOWS
    WSADATA w; if (WSAStartup(MAKEWORD(2,2), &w) != 0) return PS14_ERROR_UNKNOWN;
    #endif
    g_net_init = true; return PS14_SUCCESS;
}

void ps14_network_shutdown(void) {
    if (!g_net_init) return;
    #ifdef PS14_PLATFORM_WINDOWS
    WSACleanup();
    #endif
    g_net_init = false;
}

#ifdef PS14_PLATFORM_WINDOWS
typedef SOCKET psock; #define INVALID PS14_INVALID_SOCKET
#else
typedef int psock; #define INVALID (-1)
#endif

psock ps14_socket_create(i32 d, i32 t, i32 p) {
    psock s = socket(d, t, p);
    return s == INVALID ? INVALID : s;
}

i32 ps14_socket_close(psock s) {
    #ifdef PS14_PLATFORM_WINDOWS
    return closesocket(s) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
    #else
    return close(s) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
    #endif
}

i32 ps14_socket_bind(psock s, const char* a, u16 p) {
    struct sockaddr_in ad = {0}; ad.sin_family = AF_INET; ad.sin_port = htons(p);
    if (!a || strcmp(a,"0.0.0.0")==0) ad.sin_addr.s_addr = htonl(INADDR_ANY);
    #ifdef PS14_PLATFORM_WINDOWS
    else ad.sin_addr.s_addr = inet_addr(a);
    #else
    else inet_pton(AF_INET, a, &ad.sin_addr);
    #endif
    return bind(s, (struct sockaddr*)&ad, sizeof(ad)) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
}

i32 ps14_socket_listen(psock s, i32 b) {
    return listen(s, b) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
}

i32 ps14_socket_connect(psock s, const char* a, u16 p, u32 to) {
    struct sockaddr_in ad = {0}; ad.sin_family = AF_INET; ad.sin_port = htons(p);
    #ifdef PS14_PLATFORM_WINDOWS
    ad.sin_addr.s_addr = inet_addr(a);
    DWORD tv = to; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
    #else
    inet_pton(AF_INET, a, &ad.sin_addr);
    struct timeval tv2 = {to/1000, (to%1000)*1000};
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv2, sizeof(tv2));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv2, sizeof(tv2));
    #endif
    return connect(s, (struct sockaddr*)&ad, sizeof(ad)) == 0 ? PS14_SUCCESS : PS14_ERROR_NETWORK_ERROR;
}

i32 ps14_socket_send(psock s, const void* d, usize sz, usize* bs) {
    i32 r = (i32)send(s, (const char*)d, (int)sz, 0);
    if (r < 0) return PS14_ERROR_NETWORK_ERROR;
    if (bs) *bs = (usize)r; return PS14_SUCCESS;
}

i32 ps14_socket_receive(psock s, void* b, usize sz, usize* br) {
    i32 r = (i32)recv(s, (char*)b, (int)sz, 0);
    if (r < 0) return PS14_ERROR_NETWORK_ERROR;
    if (br) *br = (usize)r; return r == 0 ? PS14_ERROR_NETWORK_ERROR : PS14_SUCCESS;
}

i32 ps14_socket_set_non_blocking(psock s, bool nb) {
    #ifdef PS14_PLATFORM_WINDOWS
    u_long m = nb ? 1 : 0; return ioctlsocket(s, FIONBIO, &m) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
    #else
    int f = fcntl(s, F_GETFL, 0); if (f == -1) return PS14_ERROR_UNKNOWN;
    f = nb ? (f | O_NONBLOCK) : (f & ~O_NONBLOCK);
    return fcntl(s, F_SETFL, f) == 0 ? PS14_SUCCESS : PS14_ERROR_UNKNOWN;
    #endif
}

i32 ps14_socket_get_error(void) {
    #ifdef PS14_PLATFORM_WINDOWS
    return WSAGetLastError();
    #else
    return errno;
    #endif
}

u16 ps14_network_htons(u16 v) { return htons(v); }
u16 ps14_network_ntohs(u16 v) { return ntohs(v); }
u32 ps14_network_htonl(u32 v) { return htonl(v); }
u32 ps14_network_ntohl(u32 v) { return ntohl(v); }
