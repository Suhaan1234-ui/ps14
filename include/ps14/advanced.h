#ifndef PS14_ADVANCED_H
#define PS14_ADVANCED_H

#pragma once

#include "ps14.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// DETECTION TYPES
// ============================================================================

// Detection result
typedef enum {
    PS14_DETECTION_NONE = 0,
    PS14_DETECTION_DEBUGGER,
    PS14_DETECTION_HOOK,
    PS14_DETECTION_TAMPER,
    PS14_DETECTION_BUFFER_OVERFLOW,
    PS14_DETECTION_MAX
} Ps14DetectionType;

// Detection severity
typedef enum {
    PS14_SEVERITY_INFO = 0,
    PS14_SEVERITY_WARNING,
    PS14_SEVERITY_CRITICAL,
    PS14_SEVERITY_MAX
} Ps14DetectionSeverity;

// Detection event
typedef struct Ps14DetectionEvent {
    Ps14DetectionType type;
    Ps14DetectionSeverity severity;
    char description[PS14_MAX_MESSAGE];
    void* address;
    usize size;
    u64 timestamp;
    struct Ps14DetectionEvent* next;
} Ps14DetectionEvent;

// Detection callback
typedef void (*Ps14DetectionCallback)(Ps14DetectionEvent* event, void* user_data);

// ============================================================================
// DEBUGGER DETECTION
// ============================================================================

PS14_API i32 ps14_debugger_init(void);
PS14_API void ps14_debugger_shutdown(void);

// Check if a debugger is attached
PS14_API bool ps14_debugger_is_attached(void);

// Check for specific debugger types
PS14_API bool ps14_debugger_check_cheat_engine(void);
PS14_API bool ps14_debugger_check_x64dbg(void);
PS14_API bool ps14_debugger_check_ida(void);
PS14_API bool ps14_debugger_check_ollydbg(void);

// Advanced debugger detection
PS14_API bool ps14_debugger_check_int3_breakpoints(void);
PS14_API bool ps14_debugger_check_memory_breakpoints(void);
PS14_API bool ps14_debugger_check_hardware_breakpoints(void);

// Timing-based detection (debuggers slow down execution)
PS14_API bool ps14_debugger_check_timing(void);

// ============================================================================
// HOOK DETECTION
// ============================================================================

PS14_API i32 ps14_hook_init(void);
PS14_API void ps14_hook_shutdown(void);

// Check if a function is hooked
PS14_API bool ps14_hook_check_function(void* function_address);

// Check common hook types
PS14_API bool ps14_hook_check_detour(void* function_address);
PS14_API bool ps14_hook_check_jmp_hook(void* function_address);
PS14_API bool ps14_hook_check_call_hook(void* function_address);

// Check if code has been patched
PS14_API bool ps14_hook_check_code_patch(void* address, usize size, const u8* expected_bytes);

// Scan a memory region for hooks
PS14_API u32 ps14_hook_scan_region(void* address, usize size);

// ============================================================================
// CODE TAMPERING DETECTION
// ============================================================================

PS14_API i32 ps14_tamper_init(void);
PS14_API void ps14_tamper_shutdown(void);

// Check for code modifications
PS14_API bool ps14_tamper_check_code_integrity(void* address, usize size);

// Check for NOP sleds (common in patches)
PS14_API bool ps14_tamper_check_nop_sled(void* address, usize max_length);

// Check for jump/call instructions that shouldn't be there
PS14_API bool ps14_tamper_check_unexpected_jumps(void* address, usize size);

// Check for code caves (memory allocations in code sections)
PS14_API bool ps14_tamper_check_code_caves(void);

// Check for inline hooks
PS14_API bool ps14_tamper_check_inline_hooks(void* address, usize size);

// ============================================================================
// ASYNCHRONOUS AUDITING
// ============================================================================

PS14_API i32 ps14_async_auditor_init(void);
PS14_API void ps14_async_auditor_shutdown(void);

// Start async auditing with priority
PS14_API i32 ps14_async_auditor_start(u32 interval_ms, u32 thread_count);
PS14_API void ps14_async_auditor_stop(void);

// Set audit priority (0-10, higher = more frequent)
PS14_API void ps14_async_auditor_set_priority(u32 priority);

// Add a memory region for async auditing
PS14_API i32 ps14_async_auditor_add_region(const char* name, void* address, usize size, u32 priority);

// Remove a region from async auditing
PS14_API void ps14_async_auditor_remove_region(const char* name);

// Get audit statistics
PS14_API void ps14_async_auditor_get_stats(u32* scans_performed, u32* violations_found);

// ============================================================================
// BUFFER OVERFLOW PROTECTION
// ============================================================================

PS14_API i32 ps14_bof_init(void);
PS14_API void ps14_bof_shutdown(void);

// Stack protection
PS14_API void ps14_bof_stack_protect(void);
PS14_API bool ps14_bof_stack_check(void);
PS14_API void ps14_bof_stack_unprotect(void);

// Heap protection
PS14_API void* ps14_bof_heap_alloc(usize size);
PS14_API void ps14_bof_heap_free(void* ptr);
PS14_API bool ps14_bof_heap_check(void* ptr);

// Bounds checking
PS14_API i32 ps14_bof_bounds_init(void);
PS14_API void ps14_bof_bounds_shutdown(void);

// Check array bounds (for known arrays)
PS14_API bool ps14_bof_check_bounds(void* array, usize index, usize max_size);

// Register a protected buffer
PS14_API i32 ps14_bof_register_buffer(const char* name, void* buffer, usize size);

// Check if a buffer overflow occurred
PS14_API bool ps14_bof_check_buffer(const char* name);

// ============================================================================
// GENERAL DETECTION FUNCTIONS
// ============================================================================

// Initialize all detection systems
PS14_API i32 ps14_advanced_init(void);
PS14_API void ps14_advanced_shutdown(void);

// Register detection callback
PS14_API void* ps14_advanced_register_callback(Ps14DetectionCallback callback, void* user_data);
PS14_API void ps14_advanced_unregister_callback(void* handle);

// Run all detection checks
PS14_API u32 ps14_advanced_run_all_checks(void);

// Get detection statistics
PS14_API void ps14_advanced_get_stats(u32* debuggers_found, u32* hooks_found, u32* tampering_found, u32* overflows_prevented);

// Enable/disable detection systems
PS14_API void ps14_advanced_set_enabled(bool enabled);
PS14_API bool ps14_advanced_is_enabled(void);

#ifdef __cplusplus
}
#endif
#endif
