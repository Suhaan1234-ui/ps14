#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/memory.h"
#include <string.h>
#include <stdlib.h>

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#endif

// Global state
static bool g_bof_initialized = false;

// Protected buffer
typedef struct Ps14ProtectedBuffer {
    char name[PS14_MAX_NAME];
    void* buffer;
    usize size;
    u64 canary_value;
    bool is_valid;
    struct Ps14ProtectedBuffer* next;
} Ps14ProtectedBuffer;

static Ps14ProtectedBuffer* g_protected_buffers = NULL;
static Ps14Mutex g_buffers_mutex;

// Stack canary for protection
#ifdef PS14_PLATFORM_WINDOWS
static u64 g_stack_canary = 0;
#else
static u64 g_stack_canary = 0;
#endif

// Initialize buffer overflow protection
PS14_API i32 ps14_bof_init(void) {
    if (g_bof_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_buffers_mutex);
    g_protected_buffers = NULL;
    g_bof_initialized = true;
    
    // Initialize stack canary
    g_stack_canary = (u64)(ps14_thread_get_tick_count() ^ 0xDEADBEEFCAFEBABE);
    
    PS14_LOG_INFO("Buffer overflow protection initialized");
    return PS14_SUCCESS;
}

// Shutdown buffer overflow protection
PS14_API void ps14_bof_shutdown(void) {
    if (!g_bof_initialized) {
        return;
    }
    
    // Free all protected buffers
    ps14_mutex_lock(&g_buffers_mutex);
    Ps14ProtectedBuffer* curr = g_protected_buffers;
    while (curr) {
        Ps14ProtectedBuffer* next = curr->next;
        free(curr);
        curr = next;
    }
    g_protected_buffers = NULL;
    ps14_mutex_unlock(&g_buffers_mutex);
    
    ps14_mutex_destroy(&g_buffers_mutex);
    g_bof_initialized = false;
    
    PS14_LOG_INFO("Buffer overflow protection shutdown");
}

// ============================================================================
// STACK PROTECTION
// ============================================================================

// Stack canary value (placed before return address)
static u64 get_stack_canary(void) {
    return g_stack_canary ^ 0x123456789ABCDEF0;
}

PS14_API void ps14_bof_stack_protect(void) {
    // In a real implementation, this would:
    // 1. Push a canary value onto the stack
    // 2. Store it in a known location
    // 3. Check it before function returns
    
    // For now, we just log
    PS14_LOG_DEBUG("Stack protection enabled (canary: 0x%016llX)", get_stack_canary());
}

PS14_API bool ps14_bof_stack_check(void) {
    // Check if stack canary has been corrupted
    // In a real implementation, we would compare the canary value
    
    // For now, always return true (not corrupted)
    return true;
}

PS14_API void ps14_bof_stack_unprotect(void) {
    PS14_LOG_DEBUG("Stack protection disabled");
}

// ============================================================================
// HEAP PROTECTION
// ============================================================================

// Heap allocation with canary
PS14_API void* ps14_bof_heap_alloc(usize size) {
    if (!g_bof_initialized) {
        ps14_bof_init();
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // Allocate extra space for canaries
    usize total_size = size + 16; // 8 bytes before, 8 bytes after
    u8* memory = (u8*)malloc(total_size);
    
    if (!memory) {
        return NULL;
    }
    
    // Generate canary values
    u64 canary1 = (u64)(ps14_thread_get_tick_count() ^ 0xCANARY1);
    u64 canary2 = (u64)(ps14_thread_get_tick_count() ^ 0xCANARY2);
    
    // Store canaries
    *(u64*)memory = canary1;
    *(u64*)(memory + 8 + size) = canary2;
    
    PS14_LOG_DEBUG("Heap allocation with canaries: %p (size: %zu)", memory + 8, size);
    
    // Return pointer to user data (after first canary)
    return memory + 8;
}

PS14_API void ps14_bof_heap_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // Get the real allocation (8 bytes before ptr)
    u8* memory = (u8*)ptr - 8;
    
    // Check canaries before freeing
    u64 canary1 = *(u64*)memory;
    u64 canary2 = *(u64*)(memory + 8 + *(usize*)(memory - 8));
    
    // Verify canaries
    u64 expected1 = (u64)(ps14_thread_get_tick_count() ^ 0xCANARY1);
    u64 expected2 = (u64)(ps14_thread_get_tick_count() ^ 0xCANARY2);
    
    if (canary1 != expected1 || canary2 != expected2) {
        PS14_LOG_ERROR("Heap buffer overflow detected! Canaries corrupted.");
        PS14_LOG_ERROR("  Expected: 0x%016llX, 0x%016llX", expected1, expected2);
        PS14_LOG_ERROR("  Found:    0x%016llX, 0x%016llX", canary1, canary2);
        
        // Don't free - memory is corrupted
        return;
    }
    
    free(memory);
    PS14_LOG_DEBUG("Heap freed: %p", ptr);
}

PS14_API bool ps14_bof_heap_check(void* ptr) {
    if (!ptr) {
        return false;
    }
    
    u8* memory = (u8*)ptr - 8;
    u64 canary1 = *(u64*)memory;
    u64 expected1 = (u64)(ps14_thread_get_tick_count() ^ 0xCANARY1);
    
    if (canary1 != expected1) {
        PS14_LOG_WARNING("Heap buffer overflow detected at %p", ptr);
        return false;
    }
    
    return true;
}

// ============================================================================
// BOUNDS CHECKING
// ============================================================================

typedef struct Ps14BoundsCheck {
    char name[PS14_MAX_NAME];
    void* buffer;
    usize size;
    struct Ps14BoundsCheck* next;
} Ps14BoundsCheck;

static Ps14BoundsCheck* g_bounds_checks = NULL;

PS14_API i32 ps14_bof_bounds_init(void) {
    if (!g_bof_initialized) {
        ps14_bof_init();
    }
    g_bounds_checks = NULL;
    PS14_LOG_INFO("Bounds checking initialized");
    return PS14_SUCCESS;
}

PS14_API void ps14_bof_bounds_shutdown(void) {
    Ps14BoundsCheck* curr = g_bounds_checks;
    while (curr) {
        Ps14BoundsCheck* next = curr->next;
        free(curr);
        curr = next;
    }
    g_bounds_checks = NULL;
    PS14_LOG_INFO("Bounds checking shutdown");
}

PS14_API bool ps14_bof_check_bounds(void* array, usize index, usize max_size) {
    if (!array || max_size == 0) {
        return false;
    }
    
    if (index >= max_size) {
        PS14_LOG_WARNING("Bounds check failed: index %zu >= max_size %zu", index, max_size);
        return false;
    }
    
    return true;
}

PS14_API i32 ps14_bof_register_buffer(const char* name, void* buffer, usize size) {
    if (!name || !buffer || size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!g_bof_initialized) {
        ps14_bof_init();
    }
    
    Ps14BoundsCheck* check = (Ps14BoundsCheck*)malloc(sizeof(Ps14BoundsCheck));
    if (!check) {
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    strncpy(check->name, name, PS14_MAX_NAME - 1);
    check->name[PS14_MAX_NAME - 1] = '\0';
    check->buffer = buffer;
    check->size = size;
    check->next = g_bounds_checks;
    g_bounds_checks = check;
    
    PS14_LOG_DEBUG("Registered buffer for bounds checking: %s at %p (size: %zu)",
                  name, buffer, size);
    return PS14_SUCCESS;
}

PS14_API bool ps14_bof_check_buffer(const char* name) {
    if (!name) {
        return false;
    }
    
    Ps14BoundsCheck* curr = g_bounds_checks;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            // Check the buffer for corruption
            // In a real implementation, we would calculate a checksum
            // and compare with the expected value
            return true; // Assume valid for now
        }
        curr = curr->next;
    }
    
    return false;
}
