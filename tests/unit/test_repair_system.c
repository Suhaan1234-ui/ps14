#include "ps14/repair.h"
#include "ps14/memory.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <stdio.h>
#include <stdlib.h>

// Test memory repair
void test_memory_repair(void) {
    // Initialize systems
    ps14_config_init();
    ps14_memory_init();
    ps14_repair_init_memory_backup();
    
    // Create test data
    u32 test_value = 100;
    
    // Add memory region
    ps14_memory_add_region("test_repair", &test_value, sizeof(test_value));
    
    // Create backup
    Ps14MemoryBackup* backup = ps14_repair_backup_memory("test_repair", &test_value, sizeof(test_value));
    if (backup) {
        printf("[PASS] Memory backup created\n");
        
        // Modify value
        test_value = 999;
        
        // Find region
        Ps14MemoryRegion* region = ps14_memory_find_region("test_repair");
        if (region) {
            // Check if tampered
            bool tampered = ps14_memory_scan_region(region);
            if (tampered) {
                printf("[PASS] Tampering detected\n");
                
                // Restore from backup
                Ps14RepairResult result = ps14_repair_restore_memory(region);
                if (result == PS14_REPAIR_RESULT_SUCCESS) {
                    printf("[PASS] Memory restored successfully\n");
                    
                    if (test_value == 100) {
                        printf("[PASS] Value restored to original\n");
                    } else {
                        printf("[FAIL] Value not restored (got %u, expected 100)\n", test_value);
                    }
                } else {
                    printf("[FAIL] Memory restore failed: %d\n", result);
                }
            } else {
                printf("[FAIL] Tampering not detected\n");
            }
        } else {
            printf("[FAIL] Region not found\n");
        }
        
        ps14_repair_free_memory_backup(backup);
    } else {
        printf("[FAIL] Failed to create memory backup\n");
    }
    
    // Cleanup
    ps14_repair_shutdown_memory_backup();
    ps14_memory_shutdown();
}

// Test file restoration
void test_file_restoration(void) {
    ps14_config_init();
    ps14_repair_init_file_restoration();
    
    // Create test file
    FILE* file = fopen("test_backup_file.txt", "w");
    if (file) {
        fprintf(file, "Original content");
        fclose(file);
        
        // Create backup
        i32 result = ps14_repair_backup_file("test_backup_file.txt");
        if (result == PS14_SUCCESS) {
            printf("[PASS] File backup created\n");
            
            // Modify file
            file = fopen("test_backup_file.txt", "w");
            if (file) {
                fprintf(file, "Modified content");
                fclose(file);
                
                // Restore file
                Ps14RepairResult restore_result = ps14_repair_restore_file("test_backup_file.txt");
                if (restore_result == PS14_REPAIR_RESULT_SUCCESS) {
                    printf("[PASS] File restored successfully\n");
                } else {
                    printf("[FAIL] File restore failed: %d\n", restore_result);
                }
            }
        } else {
            printf("[FAIL] File backup failed: %d\n", result);
        }
        
        remove("test_backup_file.txt");
    } else {
        printf("[SKIP] Could not create test file\n");
    }
    
    ps14_repair_shutdown_file_restoration();
}

// Test state roller
void test_state_roller(void) {
    ps14_config_init();
    ps14_repair_init_state_roller();
    
    // Create test state
    u32 state_data = 42;
    
    // Create checkpoint
    i32 result = ps14_repair_create_checkpoint(1, &state_data, sizeof(state_data));
    if (result == PS14_SUCCESS) {
        printf("[PASS] State checkpoint created\n");
        
        // Get last sequence
        u32 last_seq = ps14_repair_get_last_checkpoint_sequence();
        if (last_seq == 1) {
            printf("[PASS] Last sequence is 1\n");
        } else {
            printf("[FAIL] Last sequence is %u, expected 1\n", last_seq);
        }
        
        // Rollback
        Ps14RepairResult rollback_result = ps14_repair_rollback_to_last_good();
        if (rollback_result == PS14_REPAIR_RESULT_SUCCESS) {
            printf("[PASS] Rollback successful\n");
        } else {
            printf("[FAIL] Rollback failed: %d\n", rollback_result);
        }
    } else {
        printf("[FAIL] Checkpoint creation failed: %d\n", result);
    }
    
    ps14_repair_shutdown_state_roller();
}

// Test backup manager
void test_backup_manager(void) {
    ps14_config_init();
    ps14_repair_init_backup_manager();
    
    i32 result = ps14_backup_init("test_backups");
    if (result == PS14_SUCCESS) {
        printf("[PASS] Backup manager initialized\n");
        
        // Clean up
        ps14_backup_shutdown();
    } else {
        printf("[FAIL] Backup manager initialization failed: %d\n", result);
    }
}

// Test repair statistics
void test_repair_stats(void) {
    ps14_config_init();
    ps14_repair_init_memory_backup();
    
    u32 attempted, successful;
    ps14_repair_get_stats(&attempted, &successful);
    
    printf("[INFO] Repairs attempted: %u, successful: %u\n", attempted, successful);
    
    ps14_repair_shutdown_memory_backup();
}

// Run all repair system tests
void run_repair_system_tests(void) {
    printf("\n=== Repair System Tests ===\n");
    
    ps14_logger_init(PS14_LOG_LEVEL_DEBUG, "test_repair.log");
    
    test_memory_repair();
    test_file_restoration();
    test_state_roller();
    test_backup_manager();
    test_repair_stats();
    
    ps14_logger_shutdown();
    ps14_config_shutdown();
    printf("=== Repair System Tests Complete ===\n\n");
}
