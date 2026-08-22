#include "ps14/advanced.h"
#include "ps14/logger.h"
#include <string.h>
#include <stdlib.h>

// Global state
static bool g_advanced_initialized = false;
static bool g_advanced_enabled = true;

// Detection statistics
static u32 g_debuggers_found = 0;
static u32 g_hooks_found = 0;
static u32 g_tampering_found = 0;
static u32 g_overflows_prevented = 0;

// Callback list
typedef struct CallbackNode {
    Ps14DetectionCallback callback;
    void* user_data;
    struct CallbackNode* next;
} CallbackNode;

static CallbackNode* g_callbacks = NULL;
static Ps14Mutex g_callbacks_mutex;

// Initialize all advanced detection systems
PS14_API i32 ps14_advanced_init(void) {
    if (g_advanced_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_callbacks_mutex);
    g_callbacks = NULL;
    
    // Initialize all subsystems
    ps14_debugger_init();
    ps14_hook_init();
    ps14_tamper_init();
    ps14_async_auditor_init();
    ps14_bof_init();
    
    g_advanced_initialized = true;
    g_advanced_enabled = true;
    
    PS14_LOG_INFO("Advanced detection systems initialized");
    return PS14_SUCCESS;
}

// Shutdown all advanced detection systems
PS14_API void ps14_advanced_shutdown(void) {
    if (!g_advanced_initialized) {
        return;
    }
    
    // Shutdown all subsystems
    ps14_debugger_shutdown();
    ps14_hook_shutdown();
    ps14_tamper_shutdown();
    ps14_async_auditor_shutdown();
    ps14_bof_shutdown();
    
    // Free callbacks
    ps14_mutex_lock(&g_callbacks_mutex);
    CallbackNode* curr = g_callbacks;
    while (curr) {
        CallbackNode* next = curr->next;
        free(curr);
        curr = next;
    }
    g_callbacks = NULL;
    ps14_mutex_unlock(&g_callbacks_mutex);
    
    ps14_mutex_destroy(&g_callbacks_mutex);
    g_advanced_initialized = false;
    
    PS14_LOG_INFO("Advanced detection systems shutdown");
}

// Register detection callback
PS14_API void* ps14_advanced_register_callback(Ps14DetectionCallback callback, void* user_data) {
    if (!callback) {
        return NULL;
    }
    
    CallbackNode* node = (CallbackNode*)malloc(sizeof(CallbackNode));
    if (!node) {
        return NULL;
    }
    
    node->callback = callback;
    node->user_data = user_data;
    node->next = NULL;
    
    ps14_mutex_lock(&g_callbacks_mutex);
    node->next = g_callbacks;
    g_callbacks = node;
    ps14_mutex_unlock(&g_callbacks_mutex);
    
    return node;
}

// Unregister detection callback
PS14_API void ps14_advanced_unregister_callback(void* handle) {
    if (!handle) {
        return;
    }
    
    ps14_mutex_lock(&g_callbacks_mutex);
    
    CallbackNode** curr = &g_callbacks;
    while (*curr) {
        if (*curr == (CallbackNode*)handle) {
            *curr = (*curr)->next;
            free(*curr);
            break;
        }
        curr = &(*curr)->next;
    }
    
    ps14_mutex_unlock(&g_callbacks_mutex);
}

// Notify callbacks about a detection event
static void notify_callbacks(Ps14DetectionEvent* event) {
    ps14_mutex_lock(&g_callbacks_mutex);
    CallbackNode* curr = g_callbacks;
    while (curr) {
        curr->callback(event, curr->user_data);
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_callbacks_mutex);
}

// Run all detection checks
PS14_API u32 ps14_advanced_run_all_checks(void) {
    if (!g_advanced_enabled) {
        return 0;
    }
    
    u32 detections = 0;
    
    // Check for debugger
    if (ps14_debugger_is_attached()) {
        Ps14DetectionEvent event;
        event.type = PS14_DETECTION_DEBUGGER;
        event.severity = PS14_SEVERITY_CRITICAL;
        strncpy(event.description, "Debugger attached to process", PS14_MAX_MESSAGE - 1);
        event.description[PS14_MAX_MESSAGE - 1] = '\0';
        event.address = NULL;
        event.size = 0;
        event.timestamp = ps14_thread_get_tick_count();
        
        notify_callbacks(&event);
        g_debuggers_found++;
        detections++;
    }
    
    // In a real implementation, we would also:
    // - Scan for hooks in critical functions
    // - Check code integrity
    // - Check for buffer overflows
    
    return detections;
}

// Get detection statistics
PS14_API void ps14_advanced_get_stats(u32* debuggers_found, u32* hooks_found, 
                                     u32* tampering_found, u32* overflows_prevented) {
    if (debuggers_found) {
        *debuggers_found = g_debuggers_found;
    }
    if (hooks_found) {
        *hooks_found = g_hooks_found;
    }
    if (tampering_found) {
        *tampering_found = g_tampering_found;
    }
    if (overflows_prevented) {
        *overflows_prevented = g_overflows_prevented;
    }
}

// Enable/disable detection systems
PS14_API void ps14_advanced_set_enabled(bool enabled) {
    g_advanced_enabled = enabled;
    PS14_LOG_INFO("Advanced detection %s", enabled ? "enabled" : "disabled");
}

PS14_API bool ps14_advanced_is_enabled(void) {
    return g_advanced_enabled;
}

// Check if any detection system has found something
PS14_API bool ps14_advanced_has_detections(void) {
    return (g_debuggers_found > 0 || g_hooks_found > 0 || 
            g_tampering_found > 0 || g_overflows_prevented > 0);
}

// Reset detection statistics
PS14_API void ps14_advanced_reset_stats(void) {
    g_debuggers_found = 0;
    g_hooks_found = 0;
    g_tampering_found = 0;
    g_overflows_prevented = 0;
    PS14_LOG_INFO("Advanced detection statistics reset");
}
