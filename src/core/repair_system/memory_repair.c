#include "ps14/repair.h"
#include "ps14/logger.h"
#include "ps14/memory.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

// Global memory backup list
static Ps14MemoryBackup* g_memory_backups = NULL;
static Ps14Mutex g_backups_mutex;
static bool g_memory_repair_initialized = false;

// Repair statistics
static u32 g_repairs_attempted = 0;
static u32 g_repairs_successful = 0;

// Initialize memory repair system
PS14_API i32 ps14_repair_init_memory_backup(void) {
    if (g_memory_repair_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_backups_mutex);
    g_memory_backups = NULL;
    g_repairs_attempted = 0;
    g_repairs_successful = 0;
    g_memory_repair_initialized = true;
    
    PS14_LOG_INFO("Memory repair system initialized");
    return PS14_SUCCESS;
}

// Shutdown memory repair system
PS14_API void ps14_repair_shutdown_memory_backup(void) {
    if (!g_memory_repair_initialized) {
        return;
    }
    
    // Free all backups
    ps14_mutex_lock(&g_backups_mutex);
    Ps14MemoryBackup* backup = g_memory_backups;
    while (backup) {
        Ps14MemoryBackup* next = backup->next;
        free(backup->data);
        free(backup);
        backup = next;
    }
    g_memory_backups = NULL;
    ps14_mutex_unlock(&g_backups_mutex);
    
    ps14_mutex_destroy(&g_backups_mutex);
    g_memory_repair_initialized = false;
    
    PS14_LOG_INFO("Memory repair system shutdown");
}

// Create a backup of a memory region
PS14_API Ps14MemoryBackup* ps14_repair_backup_memory(const char* name, void* address, usize size) {
    if (!name || !address || size == 0) {
        PS14_LOG_ERROR("Invalid arguments for memory backup");
        return NULL;
    }
    
    if (!g_memory_repair_initialized) {
        ps14_repair_init_memory_backup();
    }
    
    // Allocate backup structure
    Ps14MemoryBackup* backup = (Ps14MemoryBackup*)malloc(sizeof(Ps14MemoryBackup));
    if (!backup) {
        PS14_LOG_ERROR("Failed to allocate memory for backup");
        return NULL;
    }
    
    // Allocate data buffer
    backup->data = (u8*)malloc(size);
    if (!backup->data) {
        free(backup);
        PS14_LOG_ERROR("Failed to allocate memory for backup data");
        return NULL;
    }
    
    // Copy memory data
    memcpy(backup->data, address, size);
    
    // Calculate hash
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_CRC32;
    ps14_hash_compute(algo, backup->data, size, &backup->hash);
    
    backup->address = address;
    backup->size = size;
    backup->timestamp = ps14_thread_get_tick_count();
    backup->next = NULL;
    
    // Add to list
    ps14_mutex_lock(&g_backups_mutex);
    backup->next = g_memory_backups;
    g_memory_backups = backup;
    ps14_mutex_unlock(&g_backups_mutex);
    
    PS14_LOG_DEBUG("Created memory backup for %s at %p (size: %zu)", name, address, size);
    return backup;
}

// Find backup by address
static Ps14MemoryBackup* find_backup_by_address(void* address) {
    ps14_mutex_lock(&g_backups_mutex);
    Ps14MemoryBackup* backup = g_memory_backups;
    while (backup) {
        if (backup->address == address) {
            ps14_mutex_unlock(&g_backups_mutex);
            return backup;
        }
        backup = backup->next;
    }
    ps14_mutex_unlock(&g_backups_mutex);
    return NULL;
}

// Find backup by memory region
static Ps14MemoryBackup* find_backup_by_region(Ps14MemoryRegion* region) {
    if (!region) {
        return NULL;
    }
    return find_backup_by_address(region->base_address);
}

// Restore memory from backup
PS14_API Ps14RepairResult ps14_repair_restore_memory(Ps14MemoryRegion* region) {
    if (!region) {
        PS14_LOG_ERROR("Invalid region for memory restore");
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    g_repairs_attempted++;
    
    Ps14MemoryBackup* backup = find_backup_by_region(region);
    if (!backup) {
        PS14_LOG_WARNING("No backup found for memory region: %s", region->name);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // Restore memory
    memcpy(region->base_address, backup->data, backup->size);
    
    // Update region hash
    ps14_hash_copy(&region->hash, &backup->hash);
    ps14_hash_copy(&region->last_good_hash, &backup->hash);
    
    g_repairs_successful++;
    PS14_LOG_INFO("Restored memory region: %s (size: %zu bytes)", region->name, backup->size);
    return PS14_REPAIR_RESULT_SUCCESS;
}

// Restore memory by region name
PS14_API Ps14RepairResult ps14_repair_restore_memory_by_name(const char* name) {
    if (!name) {
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    Ps14MemoryRegion* region = ps14_memory_find_region(name);
    if (!region) {
        PS14_LOG_WARNING("Memory region not found: %s", name);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    return ps14_repair_restore_memory(region);
}

// Free a memory backup
PS14_API void ps14_repair_free_memory_backup(Ps14MemoryBackup* backup) {
    if (!backup) {
        return;
    }
    
    ps14_mutex_lock(&g_backups_mutex);
    
    // Remove from list
    Ps14MemoryBackup** curr = &g_memory_backups;
    while (*curr) {
        if (*curr == backup) {
            *curr = backup->next;
            break;
        }
        curr = &(*curr)->next;
    }
    
    ps14_mutex_unlock(&g_backups_mutex);
    
    free(backup->data);
    free(backup);
    PS14_LOG_DEBUG("Freed memory backup at %p", backup->address);
}

// Attempt to repair any detected tampering in a memory region
PS14_API Ps14RepairResult ps14_repair_attempt_auto_fix(Ps14MemoryRegion* region) {
    if (!region) {
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // First, check if the region is tampered
    if (!ps14_memory_scan_region(region)) {
        // No tampering detected
        return PS14_REPAIR_RESULT_SKIPPED;
    }
    
    // Tampering detected - try to restore
    return ps14_repair_restore_memory(region);
}

// Check system integrity and repair all memory issues
PS14_API u32 ps14_repair_check_and_fix_all(void) {
    u32 repaired_count = 0;
    
    Ps14MemoryScanResult results[64];
    u32 tampered_count = ps14_memory_scan_all(results, 64);
    
    for (u32 i = 0; i < tampered_count; i++) {
        if (ps14_repair_attempt_auto_fix(results[i].region) == PS14_REPAIR_RESULT_SUCCESS) {
            repaired_count++;
        }
    }
    
    if (repaired_count > 0) {
        PS14_LOG_INFO("Auto-repaired %u memory regions", repaired_count);
    }
    
    return repaired_count;
}

// Get repair statistics
PS14_API void ps14_repair_get_stats(u32* repairs_attempted, u32* repairs_successful) {
    if (repairs_attempted) {
        *repairs_attempted = g_repairs_attempted;
    }
    if (repairs_successful) {
        *repairs_successful = g_repairs_successful;
    }
}
