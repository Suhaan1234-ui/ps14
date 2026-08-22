#include "ps14/memory.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

// Global state
static Ps14MemoryRegion* g_regions = NULL;
static Ps14Mutex g_regions_mutex;
static Ps14ThreadPool* g_thread_pool = NULL;
static bool g_audit_running = false;
static u32 g_audit_interval_ms = 100;

// Callback list
typedef struct CallbackNode {
    Ps14MemoryCallback callback;
    void* user_data;
    struct CallbackNode* next;
} CallbackNode;

static CallbackNode* g_callbacks = NULL;
static Ps14Mutex g_callbacks_mutex;

// Initialize memory monitor
i32 ps14_memory_init(void) {
    ps14_mutex_init(&g_regions_mutex);
    ps14_mutex_init(&g_callbacks_mutex);
    PS14_LOG_INFO("Memory monitor initialized");
    return PS14_SUCCESS;
}

// Shutdown memory monitor
void ps14_memory_shutdown(void) {
    ps14_memory_stop_audit();
    
    // Free all regions
    ps14_mutex_lock(&g_regions_mutex);
    Ps14MemoryRegion* curr = g_regions;
    while (curr) {
        Ps14MemoryRegion* next = curr->next;
        free(curr);
        curr = next;
    }
    g_regions = NULL;
    ps14_mutex_unlock(&g_regions_mutex);
    
    // Free all callbacks
    ps14_mutex_lock(&g_callbacks_mutex);
    CallbackNode* cb = g_callbacks;
    while (cb) {
        CallbackNode* next = cb->next;
        free(cb);
        cb = next;
    }
    g_callbacks = NULL;
    ps14_mutex_unlock(&g_callbacks_mutex);
    
    ps14_mutex_destroy(&g_regions_mutex);
    ps14_mutex_destroy(&g_callbacks_mutex);
    
    PS14_LOG_INFO("Memory monitor shutdown");
}

// Audit thread function
static void audit_thread_func(void* arg) {
    while (g_audit_running) {
        Ps14Config* config = ps14_config_get();
        u32 interval = config ? config->memory_monitor.audit_interval_ms : g_audit_interval_ms;
        
        // Scan all regions
        Ps14MemoryScanResult results[64];
        u32 tampered_count = ps14_memory_scan_all(results, 64);
        
        if (tampered_count > 0) {
            PS14_LOG_WARNING("Detected %u tampered memory regions", tampered_count);
            
            // Notify callbacks
            ps14_mutex_lock(&g_callbacks_mutex);
            CallbackNode* cb = g_callbacks;
            while (cb) {
                for (u32 i = 0; i < tampered_count; i++) {
                    if (results[i].tampered) {
                        cb->callback(results[i].region, true, cb->user_data);
                    }
                }
                cb = cb->next;
            }
            ps14_mutex_unlock(&g_callbacks_mutex);
        }
        
        // Sleep for the interval
        ps14_thread_sleep(interval);
    }
}

// Start async auditing
i32 ps14_memory_start_audit(u32 interval_ms) {
    if (g_audit_running) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    g_audit_interval_ms = interval_ms;
    g_audit_running = true;
    
    // Create thread pool if not exists
    if (!g_thread_pool) {
        Ps14Config* config = ps14_config_get();
        u32 thread_count = config ? config->memory_monitor.thread_count : 4;
        if (ps14_thread_pool_init(&g_thread_pool, thread_count, 100) != PS14_SUCCESS) {
            g_audit_running = false;
            return PS14_ERROR_UNKNOWN;
        }
    }
    
    // Submit audit task
    if (ps14_thread_pool_submit(g_thread_pool, audit_thread_func, NULL, "memory_audit") != PS14_SUCCESS) {
        g_audit_running = false;
        return PS14_ERROR_UNKNOWN;
    }
    
    PS14_LOG_INFO("Memory audit started (interval: %u ms)", interval_ms);
    return PS14_SUCCESS;
}

// Stop async auditing
void ps14_memory_stop_audit(void) {
    if (!g_audit_running) {
        return;
    }
    
    g_audit_running = false;
    
    if (g_thread_pool) {
        ps14_thread_pool_shutdown(g_thread_pool, true);
        g_thread_pool = NULL;
    }
    
    PS14_LOG_INFO("Memory audit stopped");
}

// Add a memory region to monitor
Ps14MemoryRegion* ps14_memory_add_region(const char* name, void* addr, usize size) {
    if (!name || !addr || size == 0) {
        PS14_LOG_ERROR("Invalid arguments for memory region");
        return NULL;
    }
    
    Ps14MemoryRegion* region = (Ps14MemoryRegion*)malloc(sizeof(Ps14MemoryRegion));
    if (!region) {
        PS14_LOG_ERROR("Failed to allocate memory region");
        return NULL;
    }
    
    strncpy(region->name, name, PS14_MAX_NAME - 1);
    region->name[PS14_MAX_NAME - 1] = '\0';
    region->base_address = addr;
    region->size = size;
    region->is_protected = false;
    
    // Calculate initial hash
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_CRC32;
    ps14_hash_compute(algo, (const u8*)addr, size, &region->hash);
    ps14_hash_copy(&region->last_good_hash, &region->hash);
    
    // Add to list
    ps14_mutex_lock(&g_regions_mutex);
    region->next = g_regions;
    g_regions = region;
    ps14_mutex_unlock(&g_regions_mutex);
    
    PS14_LOG_DEBUG("Added memory region: %s at %p (size: %zu)", name, addr, size);
    return region;
}

// Remove a memory region
void ps14_memory_remove_region(Ps14MemoryRegion* region) {
    if (!region) {
        return;
    }
    
    ps14_mutex_lock(&g_regions_mutex);
    
    Ps14MemoryRegion** curr = &g_regions;
    while (*curr) {
        if (*curr == region) {
            *curr = region->next;
            break;
        }
        curr = &(*curr)->next;
    }
    
    ps14_mutex_unlock(&g_regions_mutex);
    free(region);
    PS14_LOG_DEBUG("Removed memory region: %s", region->name);
}

// Find a memory region by name
Ps14MemoryRegion* ps14_memory_find_region(const char* name) {
    if (!name) {
        return NULL;
    }
    
    ps14_mutex_lock(&g_regions_mutex);
    Ps14MemoryRegion* curr = g_regions;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            ps14_mutex_unlock(&g_regions_mutex);
            return curr;
        }
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_regions_mutex);
    return NULL;
}

// Scan a single memory region
bool ps14_memory_scan_region(Ps14MemoryRegion* region) {
    if (!region) {
        return false;
    }
    
    // Calculate current hash
    Ps14Hash current_hash;
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_CRC32;
    
    // Use WDK-compatible memory read for kernel, regular read for user
    #ifdef PS14_MODE_KERNEL
    // TODO: Kernel-mode memory read
    #else
    i32 result = ps14_hash_compute(algo, (const u8*)region->base_address, region->size, &current_hash);
    if (result != PS14_SUCCESS) {
        PS14_LOG_WARNING("Failed to hash region: %s", region->name);
        return false;
    }
    #endif
    
    // Compare with last good hash
    if (!ps14_hash_compare(&current_hash, &region->last_good_hash)) {
        PS14_LOG_WARNING("Tampering detected in region: %s", region->name);
        
        // Store current as last known (for debugging)
        ps14_hash_copy(&region->hash, &current_hash);
        
        return true;
    }
    
    return false;
}

// Scan all memory regions
u32 ps14_memory_scan_all(Ps14MemoryScanResult* results, usize max_results) {
    if (!results || max_results == 0) {
        return 0;
    }
    
    u32 tampered_count = 0;
    u64 timestamp = 0; // TODO: Get actual timestamp
    
    ps14_mutex_lock(&g_regions_mutex);
    Ps14MemoryRegion* curr = g_regions;
    while (curr && tampered_count < max_results) {
        if (ps14_memory_scan_region(curr)) {
            results[tampered_count].region = curr;
            results[tampered_count].timestamp = timestamp;
            results[tampered_count].tampered = true;
            results[tampered_count].tamper_type = PS14_TAMPER_DATA_MODIFIED;
            snprintf(results[tampered_count].description, PS14_MAX_MESSAGE, 
                    "Hash mismatch in region %s", curr->name);
            tampered_count++;
        }
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_region
