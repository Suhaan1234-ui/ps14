#include "ps14.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include "ps14/hash.h"
#include "ps14/memory.h"
#include "ps14/thread.h"

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID p) {
    switch (r) {
        case DLL_PROCESS_ATTACH:
            ps14_logger_init(PS14_LOG_LEVEL_INFO, 1, NULL);
            ps14_config_init();
            ps14_hash_init();
            ps14_memory_init();
            PS14_LOG_INFO("ps14_core.dll loaded");
            break;
        case DLL_PROCESS_DETACH:
            ps14_memory_shutdown();
            ps14_hash_shutdown();
            ps14_config_shutdown();
            ps14_logger_shutdown();
            PS14_LOG_INFO("ps14_core.dll unloaded");
            break;
    }
    return TRUE;
}
