#include "ps14/integrity.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <stdio.h>

// Test integrity checker initialization
void test_integrity_init(void) {
    i32 result = ps14_integrity_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] Integrity checker initialized\n");
    } else {
        printf("[FAIL] Integrity checker initialization failed: %d\n", result);
    }
}

// Test adding a file for integrity checking
void test_integrity_add_file(void) {
    // Create a temporary test file
    FILE* file = fopen("test_file.txt", "w");
    if (file) {
        fprintf(file, "Test content for integrity check");
        fclose(file);
        
        i32 result = ps14_integrity_add_file("test_file.txt");
        if (result == PS14_SUCCESS) {
            printf("[PASS] Added file for integrity checking\n");
        } else {
            printf("[FAIL] Failed to add file: %d\n", result);
        }
        
        remove("test_file.txt");
    } else {
        printf("[SKIP] Could not create test file\n");
    }
}

// Test verifying a file
void test_integrity_verify_file(void) {
    // Create a test file
    FILE* file = fopen("test_verify.txt", "w");
    if (file) {
        fprintf(file, "Test content");
        fclose(file);
        
        // Add file
        ps14_integrity_add_file("test_verify.txt");
        
        // Verify file (should pass)
        bool valid = ps14_integrity_verify_file("test_verify.txt");
        if (valid) {
            printf("[PASS] File verification passed for clean file\n");
        } else {
            printf("[FAIL] File verification failed for clean file\n");
        }
        
        remove("test_verify.txt");
    } else {
        printf("[SKIP] Could not create test file\n");
    }
}

// Test checksum database
void test_checksum_db(void) {
    i32 result = ps14_checksum_db_init(":memory:");
    if (result == PS14_SUCCESS) {
        printf("[PASS] Checksum database initialized\n");
        ps14_checksum_db_shutdown();
    } else {
        printf("[FAIL] Checksum database initialization failed: %d\n", result);
    }
}

// Run all integrity checker tests
void run_integrity_check_tests(void) {
    printf("\n=== Integrity Checker Tests ===\n");
    
    ps14_config_init();
    ps14_logger_init(PS14_LOG_LEVEL_DEBUG, "test_integrity.log");
    
    test_integrity_init();
    test_integrity_add_file();
    test_integrity_verify_file();
    test_checksum_db();
    
    ps14_integrity_shutdown();
    ps14_logger_shutdown();
    ps14_config_shutdown();
    printf("=== Integrity Checker Tests Complete ===\n\n");
}
