#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/memory.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

// Audit region
typedef struct Ps14AuditRegion {
    char name[PS14_MAX_NAME];
    void* address;
    usize size;
    u32 priority; // 0-10, higher = audited more frequently
    u64 last_audit_time;
    u32 audit_count;
    struct Ps14AuditRegion* next;
} Ps14AuditRegion;

// Async auditor state
static bool g_auditor_initialized = false;
static Ps14ThreadPool* g_thread_pool = NULL;
static Ps14AuditRegion* g_regions = NULL;
static Ps14Mutex g_regions_mutex;
static u32 g_priority = 5; // Default priority
static bool g_auditor_running = false;
static u64 g_scans_performed = 0;
static u64 g_violations_found = 0;

// Initialize async auditor
PS14_API i32 ps14_async_auditor_init(void) {
    if (g_auditor_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_regions_mutex);
    g_regions = NULL;
    g_scans_performed = 0;
    g_violations_found = 0;
    g_auditor_initialized = true;
    
    PS14_LOG_INFO("Async auditor initialized");
    return PS14_SUCCESS;
}

// Shutdown async auditor
PS14_API void ps14_async_auditor_shutdown(void) {
    if (!g_auditor_initialized) {
        return;
    }
    
    ps14_async_auditor_stop();
    
    // Free all regions
    ps14_mutex_lock(&g_regions_mutex);
    Ps14AuditRegion* curr = g_regions;
    while (curr) {
        Ps14AuditRegion* next = curr->next;
        free(curr);
        curr = next;
    }
    g_regions = NULL;
    ps14_mutex_unlock(&g_regions_mutex);
    
    ps14_mutex_destroy(&g_regions_mutex);
    g_auditor_initialized = false;
    
    PS14_LOG_INFO("Async auditor shutdown");
}

// Audit thread function
static void audit_thread_func(void* arg) {
    while (g_auditor_running) {
        ps14_mutex_lock(&g_regions_mutex);
        
        Ps14AuditRegion* curr = g_regions;
        while (curr) {
            // Audit this region
            Ps14MemoryScanResult result;
            Ps14MemoryRegion temp_region;
            
            strncpy(temp_region.name, curr->name, PS14_MAX_NAME - 1);
            temp_region.name[PS14_MAX_NAME - 1] = '\0';
            temp_region.base_address = curr->address;
            temp_region.size = curr->size;
            
            // Perform the scan
            bool tampered = ps14_memory_scan_region(&temp_region);
            
            if (tampered) {
                PS14_LOG_WARNING("Tampering detected in region: %s", curr->name);
                ps14_mutex_lock(&g_regions_mutex);
                g_violations_found++;
                ps14_mutex_unlock(&g_regions_mutex);
            }
            
            curr->last_audit_time = ps14_thread_get_tick_count();
            curr->audit_count++;
            g_scans_performed++;
            
            curr = curr->next;
        }
        
        ps14_mutex_unlock(&g_regions_mutex);
        
        // Sleep based on priority (higher priority = shorter interval)
        u32 interval = 1000 / (g_priority + 1); // 1000ms / (1-11) = 500-100ms
        if (interval < 10) interval = 10;
        ps14_thread_sleep(interval);
    }
}

// Start async auditing
PS14_API i32 ps14_async_auditor_start(u32 interval_ms, u32 thread_count) {
    if (!g_auditor_initialized) {
        ps14_async_auditor_init();
    }
    
    if (g_auditor_running) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    // Create thread pool
    if (ps14_thread_pool_init(&g_thread_pool, thread_count, 100) != PS14_SUCCESS) {
        return PS14_ERROR_UNKNOWN;
    }
    
    g_auditor_running = true;
    
    // Submit audit tasks
    for (u32 i = 0; i < thread_count; i++) {
        char thread_name[32];
        snprintf(thread_name, sizeof(thread_name), "async_audit_%u", i);
        ps14_thread_pool_submit(g_thread_pool, audit_thread_func, NULL, thread_name);
    }
    
    PS14_LOG_INFO("Async auditor started (interval: %u ms, threads: %u)", interval_ms, thread_count);
    return PS14_SUCCESS;
}

// Stop async auditing
PS14_API void ps14_async_auditor_stop(void) {
    if (!g_auditor_running) {
        return;
    }
    
    g_auditor_running = false;
    
    if (g_thread_pool) {
        ps14_thread_pool_shutdown(g_thread_pool, true);
        g_thread_pool = NULL;
    }
    
    PS14_LOG_INFO("Async auditor stopped");
}

// Set audit priority
PS14_API void ps14_async_auditor_set_priority(u32 priority) {
    if (priority > 10) priority = 10;
    g_priority = priority;
    PS14_LOG_INFO("Async auditor priority set to: %u", priority);
}

// Add a memory region for async auditing
PS14_API i32 ps14_async_auditor_add_region(const char* name, void* address, usize size, u32 priority) {
    if (!name || !address || size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!g_auditor_initialized) {
        ps14_async_auditor_init();
    }
    
    Ps14AuditRegion* region = (Ps14AuditRegion*)malloc(sizeof(Ps14AuditRegion));
    if (!region) {
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    strncpy(region->name, name, PS14_MAX_NAME - 1);
    region->name[PS14_MAX_NAME - 1] = '\0';
    region->address = address;
    region->size = size;
    region->priority = priority;
    region->last_audit_time = 0;
    region->audit_count = 0;
    region->next = NULL;
    
    ps14_mutex_lock(&g_regions_mutex);
    region->next = g_regions;
    g_regions = region;
    ps14_mutex_unlock(&g_regions_mutex);
    
    PS14_LOG_DEBUG("Added async audit region: %s at %p (size: %zu, priority: %u)",
                  name, address, size, priority);
    return PS14_SUCCESS;
}

// Remove a region from async auditing
PS14_API void ps14_async_auditor_remove_region(const char* name) {
    if (!name) {
        return;
    }
    
    ps14_mutex_lock(&g_regions_mutex);
    
    Ps14AuditRegion** curr = &g_regions;
    while (*curr) {
        if (strcmp((*curr)->name, name) == 0) {
            Ps14AuditRegion* to_remove = *curr;
            *curr = to_remove->next;
            free(to_remove);
            PS14_LOG_DEBUG("Removed async audit region: %s", name);
            break;
        }
        curr = &(*curr)->next;
    }
    
    ps14_mutex_unlock(&g_regions_mutex);
}

// Get audit statistics
PS14_API void ps14_async_auditor_get_stats(u32* scans_performed, u32* violations_found) {
    if (scans_performed) {
        *scans_performed = (u32)g_scans_performed;
    }
    if (violations_found) {
        *violations_found = (u32)g_violations_found;
    }
}
