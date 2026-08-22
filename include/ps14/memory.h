#ifndef PS14_MEMORY_H
#define PS14_MEMORY_H

#pragma once

#include "ps14.h"
#include "ps14/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory region
typedef struct Ps14MemoryRegion {
    char name[PS14_MAX_NAME];
    void* base_address;
    usize size;
    Ps14Hash hash;
    Ps14Hash last_good_hash;
    bool is_protected;
    struct Ps14MemoryRegion* next;
} Ps14MemoryRegion;

// Memory scan result
typedef struct Ps14MemoryScanResult {
    Ps14MemoryRegion* region;
    u64 timestamp;
    bool tampered;
    u32 tamper_type;
    char description[PS14_MAX_MESSAGE];
} Ps14MemoryScanResult;

// Tamper types
typedef enum {
    PS14_TAMPER_NONE = 0,
    PS14_TAMPER_DATA_MODIFIED,
    PS14_TAMPER_CODE_PATCHED,
    PS14_TAMPER_PROTECTION_REMOVED,
    PS14_TAMPER_MAX
} Ps14TamperType;

// Callback type
typedef void (*Ps14MemoryCallback)(Ps14MemoryRegion* region, bool tampered, void* user_data);

// Main functions
PS14_API i32 ps14_memory_init(void);
PS14_API void ps14_memory_shutdown(void);
PS14_API i32 ps14_memory_start_audit(u32 interval_ms);
PS14_API void ps14_memory_stop_audit(void);

PS14_API Ps14MemoryRegion* ps14_memory_add_region(const char* name, void* addr, usize size);
PS14_API void ps14_memory_remove_region(Ps14MemoryRegion* region);
PS14_API Ps14MemoryRegion* ps14_memory_find_region(const char* name);
PS14_API bool ps14_memory_scan_region(Ps14MemoryRegion* region);
PS14_API u32 ps14_memory_scan_all(Ps14MemoryScanResult* results, usize max_results);

PS14_API void* ps14_memory_register_callback(Ps14MemoryCallback cb, void* user_data);
PS14_API void ps14_memory_unregister_callback(void* handle);

PS14_API i32 ps14_memory_protect_region(Ps14MemoryRegion* region);
PS14_API i32 ps14_memory_repair_region(Ps14MemoryRegion* region);

#ifdef __cplusplus
}
#endif
#endif
