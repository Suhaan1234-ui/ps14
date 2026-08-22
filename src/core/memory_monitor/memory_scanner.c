#include "ps14/memory.h"
#include "ps14/logger.h"
i32 ps14_memory_init(void) { PS14_LOG_INFO("Memory monitor initialized"); return PS14_SUCCESS; }
void ps14_memory_shutdown(void) { PS14_LOG_INFO("Memory monitor shutdown"); }
