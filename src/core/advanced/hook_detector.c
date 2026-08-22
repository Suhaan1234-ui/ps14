#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/memory.h"
#include <string.h>

// Global state
static bool g_hook_initialized = false;

// Known hook signatures
typedef struct {
    const char* name;
    u8 signature[16];
    usize sig_length;
} HookSignature;

// Common detour hook signature (JMP to detour function)
static const u8 g_detour_signature[] = { 0xE9, 0x00, 0x00, 0x00, 0x00 }; // JMP rel32

// Common JMP hook signature
static const u8 g_jmp_hook_signature[] = { 0xEB, 0x00 }; // JMP rel8

// Common CALL hook signature
static const u8 g_call_hook_signature[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 }; // CALL rel32

// Initialize hook detector
PS14_API i32 ps14_hook_init(void) {
    if (g_hook_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    g_hook_initialized = true;
    PS14_LOG_INFO("Hook detector initialized");
    return PS14_SUCCESS;
}

// Shutdown hook detector
PS14_API void ps14_hook_shutdown(void) {
    g_hook_initialized = false;
    PS14_LOG_INFO("Hook detector shutdown");
}

// Check if a function is hooked by comparing with expected bytes
PS14_API bool ps14_hook_check_function(void* function_address) {
    if (!function_address) {
        return false;
    }
    
    // For now, we'll check if the first byte is a JMP (0xE9) or CALL (0xE8)
    // In a real implementation, we'd compare with known-good bytes
    u8 first_byte = *(u8*)function_address;
    
    // Common hook patterns:
    // 0xE9 = JMP near (detour)
    // 0xE8 = CALL near (some hooks)
    // 0xEB = JMP short
    if (first_byte == 0xE9 || first_byte == 0xE8 || first_byte == 0xEB) {
        PS14_LOG_WARNING("Potential hook detected at %p (first byte: 0x%02X)", 
                       function_address, first_byte);
        return true;
    }
    
    return false;
}

// Check for Detours library hook
PS14_API bool ps14_hook_check_detour(void* function_address) {
    if (!function_address) {
        return false;
    }
    
    u8 first_byte = *(u8*)function_address;
    if (first_byte == 0xE9) {
        // This looks like a Detours-style hook
        PS14_LOG_WARNING("Detours-style hook detected at %p", function_address);
        return true;
    }
    
    return false;
}

// Check for JMP hook
PS14_API bool ps14_hook_check_jmp_hook(void* function_address) {
    if (!function_address) {
        return false;
    }
    
    u8 first_byte = *(u8*)function_address;
    if (first_byte == 0xE9 || first_byte == 0xEB) {
        // JMP instruction (near or short)
        PS14_LOG_WARNING("JMP hook detected at %p", function_address);
        return true;
    }
    
    return false;
}

// Check for CALL hook
PS14_API bool ps14_hook_check_call_hook(void* function_address) {
    if (!function_address) {
        return false;
    }
    
    u8 first_byte = *(u8*)function_address;
    if (first_byte == 0xE8) {
        // CALL instruction
        PS14_LOG_WARNING("CALL hook detected at %p", function_address);
        return true;
    }
    
    return false;
}

// Check if code has been patched (compare with expected bytes)
PS14_API bool ps14_hook_check_code_patch(void* address, usize size, const u8* expected_bytes) {
    if (!address || !expected_bytes || size == 0) {
        return false;
    }
    
    u8* current = (u8*)address;
    for (usize i = 0; i < size; i++) {
        if (current[i] != expected_bytes[i]) {
            PS14_LOG_WARNING("Code patch detected at %p (byte %zu: 0x%02X vs 0x%02X)",
                           address, i, current[i], expected_bytes[i]);
            return true;
        }
    }
    
    return false;
}

// Scan a memory region for hooks
PS14_API u32 ps14_hook_scan_region(void* address, usize size) {
    if (!address || size < 5) {
        return 0;
    }
    
    u32 hooks_found = 0;
    u8* current = (u8*)address;
    
    for (usize i = 0; i < size - 5; i++) {
        // Check for JMP instruction (0xE9)
        if (current[i] == 0xE9) {
            PS14_LOG_WARNING("Found JMP hook at %p (offset %zu)", 
                           address, i);
            hooks_found++;
        }
        // Check for CALL instruction (0xE8)
        else if (current[i] == 0xE8) {
            PS14_LOG_WARNING("Found CALL hook at %p (offset %zu)", 
                           address, i);
            hooks_found++;
        }
        // Check for short JMP (0xEB)
        else if (current[i] == 0xEB) {
            PS14_LOG_WARNING("Found short JMP at %p (offset %zu)", 
                           address, i);
            hooks_found++;
        }
        // Check for NOP sleds (0x90) - often used before hooks
        else if (current[i] == 0x90) {
            // Count consecutive NOPs
            usize nop_count = 0;
            while (i < size && current[i] == 0x90) {
                nop_count++;
                i++;
            }
            if (nop_count >= 5) {
                PS14_LOG_WARNING("Found NOP sled (%zu bytes) at %p", 
                               nop_count, address + (i - nop_count));
                hooks_found++;
            }
        }
    }
    
    return hooks_found;
}

// Check for inline hooks (hooks within function body)
PS14_API bool ps14_hook_check_inline_hooks(void* address, usize size) {
    if (!address || size < 16) {
        return false;
    }
    
    // Look for unexpected JMP/CALL instructions in the middle of functions
    u8* current = (u8*)address;
    
    // Skip the first few bytes (function prologue)
    for (usize i = 8; i < size - 8; i++) {
        if (current[i] == 0xE9 || current[i] == 0xE8) {
            // Found JMP or CALL in the middle of function
            PS14_LOG_WARNING("Found inline hook at %p (offset %zu)", 
                           address, i);
            return true;
        }
    }
    
    return false;
}
