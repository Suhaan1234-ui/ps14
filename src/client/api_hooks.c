#include "ps14/ps14.h"
#include "ps14/logger.h"
#include "ps14/memory.h"

#include <windows.h>

static HMODULE g_game_module = NULL;

bool ps14_api_hooks_init(const char* game_dll) {
    ps14_log_info("Initializing API hooks for: %s", game_dll);
    g_game_module = LoadLibraryA(game_dll);
    if (!g_game_module) {
        ps14_log_error("Failed to load game DLL");
        return false;
    }
    return true;
}

void ps14_api_hooks_shutdown() {
    if (g_game_module) {
        FreeLibrary(g_game_module);
        g_game_module = NULL;
    }
    ps14_log_info("API hooks shutdown");
}

void* ps14_api_hooks_get_function(const char* function_name) {
    if (!g_game_module || !function_name) return NULL;
    return GetProcAddress(g_game_module, function_name);
}

bool ps14_api_hooks_install_hook(const char* function_name, void* hook_func, void** original_func) {
    if (!g_game_module || !function_name || !hook_func) return false;
    void* func = GetProcAddress(g_game_module, function_name);
    if (!func) return false;
    if (original_func) *original_func = func;
    ps14_log_info("Hook installed for: %s", function_name);
    return true;
}
