#include "ps14/memory.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <stdio.h>
#include <string.h>

// Test data
static u32 g_test_value = 100;
static char g_test_buffer[256] = "Test string for memory monitoring";

// Test memory monitor initialization
void test_memory_init(void) {
    i32 result = ps14_memory_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] Memory monitor initialized\n");
    } else {
        printf("[FAIL] Memory monitor initialization failed: %d\n", result);
    }
}

// Test adding a memory region
void test_memory_add_region(void) {
    Ps14MemoryRegion* region = ps14_memory_add_region("test_value", &g_test_value, sizeof(g_test_value));
    if (region) {
        printf("[PASS] Added memory region: %s at %p\n", region->name, region->base_address);
    } else {
        printf("[FAIL] Failed to add memory region\n");
    }
}

// Test scanning a region
void test_memory_scan_region(void) {
    Ps14MemoryRegion* region = ps14_memory_find_region("test_value");
    if (!region) {
        printf("[FAIL] Region not found\n");
        return;
    }
    
    bool tampered = ps14_memory_scan_region(region);
    if (!tampered) {
        printf("[PASS] Region scan: no tampering detected\n");
    } else {
        printf("[FAIL] False positive: tampering detected on clean region\n");
    }
}

// Test tampering detection
void test_memory_tampering(void) {
    Ps14MemoryRegion* region = ps14_memory_find_region("test_value");
    if (!region) {
        printf("[FAIL] Region not found\n");
        return;
    }
    
    // Modify the value
    u32 original = g_test_value;
    g_test_value = 999;
    
    bool tampered = ps14_memory_scan_region(region);
    g_test_value = original; // Restore
    
    if (tampered) {
        printf("[PASS] Tampering detected successfully\n");
    } else {
        printf("[FAIL] Tampering not detected\n");
    }
}

// Test memory scan all
void test_memory_scan_all(void) {
    Ps14MemoryScanResult results[16];
    u32 count = ps14_memory_scan_all(results, 16);
    printf("[INFO] Scanned all regions, found %u tampered\n", count);
    
    if (count == 0) {
        printf("[PASS] No tampering detected in scan all\n");
    } else {
        printf("[FAIL] Unexpected tampering in scan all\n");
    }
}

// Test removing a region
void test_memory_remove_region(void) {
    Ps14MemoryRegion* region = ps14_memory_find_region("test_value");
    if (!region) {
        printf("[FAIL] Region not found\n");
        return;
    }
    
    ps14_memory_remove_region(region);
    
    region = ps14_memory_find_region("test_value");
    if (!region) {
        printf("[PASS] Region removed successfully\n");
    } else {
        printf("[FAIL] Region still exists after removal\n");
    }
}

// Test async auditing
void test_memory_async_audit(void) {
    i32 result = ps14_memory_start_audit(100);
    if (result == PS14_SUCCESS) {
        printf("[PASS] Async auditing started\n");
        ps14_thread_sleep(200); // Let it run a bit
        ps14_memory_stop_audit();
        printf("[PASS] Async auditing stopped\n");
    } else {
        printf("[FAIL] Failed to start async auditing: %d\n", result);
    }
}

// Run all memory monitor tests
void run_memory_monitor_tests(void) {
    printf("\n=== Memory Monitor Tests ===\n");
    
    // Initialize config and logger first
    ps14_config_init();
    ps14_logger_init(PS14_LOG_LEVEL_DEBUG, "test_memory.log");
    
    test_memory_init();
    test_memory_add_region();
    test_memory_scan_region();
    test_memory_tampering();
    test_memory_scan_all();
    test_memory_async_audit();
    test_memory_remove_region();
    
    ps14_memory_shutdown();
    ps14_logger_shutdown();
    ps14_config_shutdown();
    printf("=== Memory Monitor Tests Complete ===\n\n");
}
