#include "ps14/ps14.h"
#include "ps14/logger.h"

typedef struct {
    char ip_address[48];
    u64 ban_time;
    u64 duration;
    char reason[256];
} BanEntry;

static BanEntry g_bans[1024];
static u32 g_ban_count = 0;

bool ps14_ban_manager_init() {
    return true;
}

void ps14_ban_manager_shutdown() {
}

bool ps14_ban_manager_ban_ip(const char* ip, u64 duration_seconds, const char* reason) {
    if (g_ban_count >= 1024) return false;
    strncpy(g_bans[g_ban_count].ip_address, ip, 47);
    g_bans[g_ban_count].ban_time = time(NULL);
    g_bans[g_ban_count].duration = duration_seconds;
    strncpy(g_bans[g_ban_count].reason, reason, 255);
    g_ban_count++;
    ps14_log_info("Banned IP: %s for %llu seconds: %s", ip, duration_seconds, reason);
    return true;
}

bool ps14_ban_manager_unban_ip(const char* ip) {
    for (u32 i = 0; i < g_ban_count; i++) {
        if (strcmp(g_bans[i].ip_address, ip) == 0) {
            for (u32 j = i; j < g_ban_count - 1; j++) {
                g_bans[j] = g_bans[j + 1];
            }
            g_ban_count--;
            ps14_log_info("Unbanned IP: %s", ip);
            return true;
        }
    }
    return false;
}

bool ps14_ban_manager_is_banned(const char* ip) {
    for (u32 i = 0; i < g_ban_count; i++) {
        if (strcmp(g_bans[i].ip_address, ip) == 0) {
            if (time(NULL) - g_bans[i].ban_time < g_bans[i].duration) {
                return true;
            }
        }
    }
    return false;
}
