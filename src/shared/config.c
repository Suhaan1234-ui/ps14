#include "ps14/config.h"
#include "ps14/logger.h"
#include <string.h>

static Ps14Config g_config;
static bool g_config_initialized = false;

static Ps14Config g_default_config = {
    .enabled = true,
    .log_level = PS14_LOG_LEVEL_INFO,
    .log_file = "ps14.log",
    .memory_monitor = {true, 100, 4, true, "regions.json"},
    .integrity_checker = {true, 1000, PS14_HASH_CRC32, "checksums.db"},
    .auth = {true, "localhost", 8080, "", 3600},
    .network = {false, "cert.pem", "key.pem", 30, 3},
    .repair = {true, "backups", 10, true},
    .kernel = {false, "ps14_driver.sys", "\.\ps14_protection"},
    .performance = {80, 512, true}
};

i32 ps14_config_init(void) {
    if (g_config_initialized) return PS14_ERROR_ALREADY_INITIALIZED;
    memcpy(&g_config, &g_default_config, sizeof(Ps14Config));
    g_config_initialized = true;
    return PS14_SUCCESS;
}

void ps14_config_shutdown(void) {
    g_config_initialized = false;
}

Ps14Config* ps14_config_get(void) {
    if (!g_config_initialized) ps14_config_init();
    return &g_config;
}

const Ps14Config* ps14_config_get_const(void) {
    return ps14_config_get();
}

i32 ps14_config_load(const char* filepath) {
    if (!g_config_initialized) ps14_config_init();
    return PS14_SUCCESS;
}

i32 ps14_config_save(const char* filepath) {
    if (!g_config_initialized) return PS14_ERROR_NOT_INITIALIZED;
    return PS14_SUCCESS;
}
