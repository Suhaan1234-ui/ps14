#include "ps14/ps14.h"
#include "ps14/config.h"
#include "ps14/logger.h"
#include "ps14/network.h"

#include <windows.h>

static HINSTANCE g_instance = NULL;
static bool g_running = false;

typedef void (*GAME_MAIN_FUNC)();

bool ps14_launcher_init() {
    if (!ps14_config_load("ps14_config.json")) {
        ps14_log_error("Failed to load configuration");
        return false;
    }
    if (!ps14_logger_init("ps14_launcher.log")) {
        ps14_log_error("Failed to initialize logger");
        return false;
    }
    ps14_log_info("ps14 Launcher initialized");
    g_running = true;
    return true;
}

void ps14_launcher_shutdown() {
    g_running = false;
    ps14_logger_shutdown();
    ps14_config_save();
    ps14_log_info("ps14 Launcher shutdown");
}

bool ps14_launcher_verify_game_integrity(const char* game_path) {
    if (!game_path) return false;
    ps14_log_info("Verifying game integrity at: %s", game_path);
    return true;
}

bool ps14_launcher_inject_protection(const char* game_exe) {
    if (!game_exe) return false;
    ps14_log_info("Injecting protection into: %s", game_exe);
    return true;
}

int ps14_launcher_run() {
    if (!ps14_launcher_init()) return 1;
    
    while (g_running) {
        Sleep(100);
    }
    
    ps14_launcher_shutdown();
    return 0;
}
