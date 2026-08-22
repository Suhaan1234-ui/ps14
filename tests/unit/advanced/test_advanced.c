#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <stdio.h>

// Test advanced initialization
void test_advanced_init(void) {
    i32 result = ps14_advanced_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] Advanced detection initialized\n");
    } else {
        printf("[FAIL] Advanced detection initialization failed: %d\n", result);
    }
}

// Test debugger detection
void test_debugger_detection(void) {
    if (ps14_debugger_is_attached()) {
        printf("[WARN] Debugger is attached (expected for development)\n");
    } else {
        printf("[PASS] No debugger detected\n");
    }
}

// Test hook detection
void test_hook_detection(void) {
    // Create a test function pointer
    void* test_func = (void*)0x00000001;
    
    // This should return false for a non-hooked address
    bool is_hooked = ps14_hook_check_function(test_func);
    if (!is_hooked) {
        printf("[PASS] Hook detection: no hook detected at test address\n");
    } else {
        printf("[INFO] Hook detection: possible hook at test address (0x%p)\n", test_func);
    }
}

// Test tamper detection
void test_tamper_detection(void) {
    u8 test_data[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    
    // Check for NOP sled (should not find any)
    bool has_nop_sled = ps14_tamper_check_nop_sled(test_data, sizeof(test_data));
    if (!has_nop_sled) {
        printf("[PASS] Tamper detection: no NOP sled detected\n");
    } else {
        printf("[FAIL] Tamper detection: false positive on NOP sled\n");
    }
}

// Test async auditor
void test_async_auditor(void) {
    i32 result = ps14_async_auditor_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] Async auditor initialized\n");
        
        // Add a test region
        u32 test_value = 42;
        result = ps14_async_auditor_add_region("test_region", &test_value, sizeof(test_value), 5);
        if (result == PS14_SUCCESS) {
            printf("[PASS] Async auditor: region added\n");
        } else {
            printf("[FAIL] Async auditor: failed to add region\n");
        }
        
        ps14_async_auditor_shutdown();
    } else {
        printf("[FAIL] Async auditor initialization failed\n");
    }
}

// Test buffer overflow protection
void test_buffer_overflow(void) {
    i32 result = ps14_bof_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] Buffer overflow protection initialized\n");
        
        // Test bounds checking
        u32 test_array[10];
        bool in_bounds = ps14_bof_check_bounds(test_array, 5, 10);
        if (in_bounds) {
            printf("[PASS] Bounds check: index 5 is in bounds\n");
        } else {
            printf("[FAIL] Bounds check: false negative\n");
        }
        
        bool out_of_bounds = ps14_bof_check_bounds(test_array, 15, 10);
        if (!out_of_bounds) {
            printf("[PASS] Bounds check: index 15 is out of bounds\n");
        } else {
            printf("[FAIL] Bounds check: false positive\n");
        }
        
        ps14_bof_shutdown();
    } else {
        printf("[FAIL] Buffer overflow protection initialization failed\n");
    }
}

// Test all detection checks
void test_all_checks(void) {
    u32 detections = ps14_advanced_run_all_checks();
    printf("[INFO] All detection checks: %u detections found\n", detections);
    
    if (detections == 0) {
        printf("[PASS] No detections (clean environment)\n");
    } else {
        printf("[INFO] Detections found (may be expected in dev environment)\n");
    }
}

// Test statistics
void test_statistics(void) {
    u32 debuggers, hooks, tampering, overflows;
    ps14_advanced_get_stats(&debuggers, &hooks, &tampering, &overflows);
    
    printf("[INFO] Statistics: debuggers=%u, hooks=%u, tampering=%u, overflows=%u\n",
           debuggers, hooks, tampering, overflows);
}

// Run all advanced tests
void run_advanced_tests(void) {
    printf("\n=== Advanced Detection Tests ===\n");
    
    ps14_config_init();
    ps14_logger_init(PS14_LOG_LEVEL_DEBUG, "test_advanced.log");
    
    test_advanced_init();
    test_debugger_detection();
    test_hook_detection();
    test_tamper_detection();
    test_async_auditor();
    test_buffer_overflow();
    test_all_checks();
    test_statistics();
    
    ps14_advanced_shutdown();
    ps14_logger_shutdown();
    ps14_config_shutdown();
    
    printf("=== Advanced Detection Tests Complete ===\n\n");
}
