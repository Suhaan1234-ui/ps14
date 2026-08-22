#include "ps14/logger.h"
#include "ps14/thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { Ps14LogLevel l; u32 t; FILE* f; } LS;
static LS g_l = {0};
static const char* names[] = {"FATAL","ERROR","WARNING","INFO","DEBUG","TRACE"};

static void ts(char* b, int s) {
    time_t n = time(NULL);
    struct tm* t = localtime(&n);
    strftime(b, s, "%Y-%m-%d %H:%M:%S", t);
}

i32 ps14_logger_init(Ps14LogLevel ll, u32 t, const char* lf) {
    if (g_l.l != 0) return PS14_ERROR_ALREADY_INITIALIZED;
    ps14_mutex_init((Ps14Mutex*)&g_l);
    g_l.l = ll; g_l.t = t;
    if ((t & 2) && lf) g_l.f = fopen(lf, "a");
    return PS14_SUCCESS;
}

void ps14_logger_shutdown(void) {
    if (g_l.f) fclose(g_l.f);
    g_l.f = NULL; g_l.l = 0; g_l.t = 0;
}

void ps14_logger_set_level(Ps14LogLevel ll) { g_l.l = ll; }
Ps14LogLevel ps14_logger_get_level(void) { return g_l.l; }
void ps14_logger_set_targets(u32 t) { g_l.t = t; }
void ps14_logger_add_target(Ps14LogTarget t) { g_l.t |= t; }
void ps14_logger_remove_target(Ps14LogTarget t) { g_l.t &= ~t; }
void* ps14_logger_register_callback(Ps14LogCallback c, void* d) { return NULL; }
void ps14_logger_unregister_callback(void* h) {}

i32 ps14_logger_init_network(const char* a, u16 p) { return PS14_ERROR_NOT_INITIALIZED; }
void ps14_logger_shutdown_network(void) {}
i32 ps14_logger_send_network(Ps14LogLevel l, const char* m) { return PS14_ERROR_NOT_INITIALIZED; }
i32 ps14_logger_rotate_files(u64 s, u32 f) { return PS14_ERROR_NOT_INITIALIZED; }

void ps14_log(Ps14LogLevel l, const char* f, u32 li, const char* fn, const char* fmt, ...) {
    if (l > g_l.l) return;
    char buf[2048], ts[64];
    ts(ts, sizeof(ts));
    va_list a; va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
    char msg[2048];
    snprintf(msg, sizeof(msg), "[%s] [%s] [%s:%u] [%s] %s", ts, names[l], f?f:"unk", li, fn?fn:"unk", buf);
    if (g_l.t & 1) printf("%s\n", msg);
    if ((g_l.t & 2) && g_l.f) { fprintf(g_l.f, "%s\n", msg); fflush(g_l.f); }
}
