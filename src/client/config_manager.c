#include "ps14/ps14.h"
#include "ps14/config.h"
#include "ps14/logger.h"

#include <windows.h>

static Ps14Config g_config;

bool ps14_config_manager_load(const char* path) {
    if (ps14_config_load(path)) {
        g_config = ps14_config_get();
        return true;
    }
    return false;
}

bool ps14_config_manager_save(const char* path) {
    return ps14_config_save_as(path);
}

Ps14Config ps14_config_manager_get() {
    return g_config;
}

bool ps14_config_manager_set(Ps14Config config) {
    g_config = config;
    return ps14_config_set(g_config);
}

bool ps14_config_manager_set_memory_protection(bool enabled) {
    g_config.memory_protection_enabled = enabled;
    return true;
}

bool ps14_config_manager_set_integrity_check(bool enabled) {
    g_config.integrity_check_enabled = enabled;
    return true;
}

bool ps14_config_manager_set_debugger_detection(bool enabled) {
    g_config.debugger_detection_enabled = enabled;
    return true;
}
