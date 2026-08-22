#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/hash.h"
#include "ps14/memory.h"
#include <string.h>

// Global state
static bool g_tamper_initialized = false;

// Known good checksums for critical functions
// In a real implementation, this would be populated at startup
typedef struct {
    const char* name;
    void* address;
    usize size;
    Ps14Hash hash;
} KnownFunction;

// Initialize tamper detector
PS14_API i32 ps14_tamper_init(void) {
    if (g_tamper_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    g_tamper_initialized = true;
    PS14_LOG_INFO("Tamper detector initialized");
    return PS14_SUCCESS;
}

// Shutdown tamper detector
PS14_API void ps14_tamper_shutdown(void) {
    g_tamper_initialized = false;
    PS14_LOG_INFO("Tamper detector shutdown");
}

// Check for code modifications by comparing hashes
PS14_API bool ps14_tamper_check_code_integrity(void* address, usize size) {
    if (!address || size == 0) {
        return false;
    }
    
    // Calculate current hash
    Ps14Hash current_hash;
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_CRC32;
    
    if (ps14_hash_compute(algo, (const u8*)address, size, &current_hash) != PS14_SUCCESS) {
        return false;
    }
    
    // In a real implementation, we would compare with a known-good hash
    // For now, we just return false (no tampering detected)
    // This is a placeholder - the actual check would use stored hashes
    
    return false;
}

// Check for NOP sleds (common in code patches)
PS14_API bool ps14_tamper_check_nop_sled(void* address, usize max_length) {
    if (!address || max_length < 5) {
        return false;
    }
    
    u8* current = (u8*)address;
    usize consecutive_nops = 0;
    
    for (usize i = 0; i < max_length; i++) {
        if (current[i] == 0x90) { // NOP instruction
            consecutive_nops++;
            if (consecutive_nops >= 5) {
                // Found a NOP sled (5 or more consecutive NOPs)
                PS14_LOG_WARNING("NOP sled detected at %p (%zu bytes)", 
                               address + (i - consecutive_nops + 1), consecutive_nops);
                return true;
            }
        } else {
            consecutive_nops = 0;
        }
    }
    
    return false;
}

// Check for unexpected jump/call instructions
PS14_API bool ps14_tamper_check_unexpected_jumps(void* address, usize size) {
    if (!address || size < 16) {
        return false;
    }
    
    u8* current = (u8*)address;
    
    // Check for jumps in unexpected places
    for (usize i = 0; i < size; i++) {
        u8 opcode = current[i];
        
        // Check for various jump/call instructions
        if (opcode == 0xE8) { // CALL relative
            PS14_LOG_WARNING("Unexpected CALL instruction at %p (offset %zu)", 
                           address, i);
            return true;
        } else if (opcode == 0xE9) { // JMP relative
            PS14_LOG_WARNING("Unexpected JMP instruction at %p (offset %zu)", 
                           address, i);
            return true;
        } else if (opcode == 0xEB) { // JMP short
            PS14_LOG_WARNING("Unexpected short JMP at %p (offset %zu)", 
                           address, i);
            return true;
        } else if (opcode == 0x74 || opcode == 0x75) { // JZ/JNZ
            // These are conditional jumps - less suspicious
        } else if (opcode == 0xFF) {
            // Could be CALL [address] or JMP [address]
            PS14_LOG_WARNING("Indirect CALL/JMP at %p (offset %zu)", 
                           address, i);
            return true;
        }
    }
    
    return false;
}

// Check for code caves (allocated memory in code sections)
PS14_API bool ps14_tamper_check_code_caves(void) {
    // This would require platform-specific code to:
    // 1. Get the current process's memory regions
    // 2. Check which regions have EXECUTE permission
    // 3. Check if any have been allocated (not part of original image)
    
    // For now, return false
    // In a real implementation, this would use VirtualQuery or similar
    
    PS14_LOG_DEBUG("Code cave detection not implemented yet");
    return false;
}

// Check for inline hooks (hooks within function body)
PS14_API bool ps14_tamper_check_inline_hooks(void* address, usize size) {
    // This is similar to hook detection but more focused on
    // detecting hooks in the middle of functions
    
    if (!address || size < 16) {
        return false;
    }
    
    u8* current = (u8*)address;
    
    // Skip the first few bytes (function prologue)
    for (usize i = 8; i < size - 8; i++) {
        u8 opcode = current[i];
        
        // Look for JMP or CALL instructions
        if (opcode == 0xE9 || opcode == 0xE8 || opcode == 0xEB) {
            PS14_LOG_WARNING("Inline hook detected at %p (offset %zu, opcode: 0x%02X)",
                           address, i, opcode);
            return true;
        }
    }
    
    return false;
}

// Scan all protected memory regions for tampering
PS14_API u32 ps14_tamper_scan_all_protected(void) {
    u32 violations = 0;
    
    // Get all memory regions from the memory monitor
    // This is a placeholder - in a real implementation, we would
    // integrate with the memory monitoring system
    
    PS14_LOG_DEBUG("Scanning all protected regions for tampering");
    
    // For now, just return 0
    return violations;
}

// Check if a specific range of memory has been modified
PS14_API bool ps14_tamper_check_range(void* address, usize size, const u8* expected_data) {
    if (!address || !expected_data || size == 0) {
        return false;
    }
    
    u8* current = (u8*)address;
    for (usize i = 0; i < size; i++) {
        if (current[i] != expected_data[i]) {
            PS14_LOG_WARNING("Memory tampering detected at %p (offset %zu: 0x%02X vs 0x%02X)",
                           address, i, current[i], expected_data[i]);
            return true;
        }
    }
    
    return false;
}
