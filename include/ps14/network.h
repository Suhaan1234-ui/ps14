#ifndef PS14_NETWORK_H
#define PS14_NETWORK_H
#pragma once
#include "ps14.h"
#ifdef __cplusplus
extern "C" {
#endif

i32 ps14_network_init(void);
void ps14_network_shutdown(void);

typedef void* Ps14SocketHandle;

PS14_API i32 ps14_socket_create(i32 domain, i32 type, i32 protocol);
PS14_API i32 ps14_socket_close(Ps14SocketHandle sock);
PS14_API i32 ps14_socket_bind(Ps14SocketHandle sock, const char* address, u16 port);
PS14_API i32 ps14_socket_listen(Ps14SocketHandle sock, i32 backlog);
PS14_API i32 ps14_socket_connect(Ps14SocketHandle sock, const char* address, u16 port, u32 timeout_ms);
PS14_API i32 ps14_socket_send(Ps14SocketHandle sock, const void* data, usize size, usize* bytes_sent);
PS14_API i32 ps14_socket_receive(Ps14SocketHandle sock, void* buffer, usize size, usize* bytes_received);
PS14_API i32 ps14_socket_set_non_blocking(Ps14SocketHandle sock, bool non_blocking);
PS14_API i32 ps14_socket_get_error(void);
PS14_API u16 ps14_network_htons(u16 value);
PS14_API u16 ps14_network_ntohs(u16 value);
PS14_API u32 ps14_network_htonl(u32 value);
PS14_API u32 ps14_network_ntohl(u32 value);

#ifdef __cplusplus
}
#endif
#endif
