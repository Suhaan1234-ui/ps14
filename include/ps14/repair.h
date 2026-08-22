#ifndef PS14_REPAIR_H
#define PS14_REPAIR_H

#pragma once

#include "ps14.h"
#include "ps14/memory.h"
#include "ps14/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Repair types
typedef enum {
    PS14_REPAIR_TYPE_MEMORY = 0,
    PS14_REPAIR_TYPE_FILE,
    PS14_REPAIR_TYPE_STATE,
    PS14_REPAIR_TYPE_MAX
} Ps14RepairType;

// Repair result
typedef enum {
    PS14_REPAIR_RESULT_SUCCESS = 0,
    PS14_REPAIR_RESULT_FAILED,
    PS14_REPAIR_RESULT_PARTIAL,
    PS14_REPAIR_RESULT_SKIPPED
} Ps14RepairResult;

// Backup file info
typedef struct Ps14BackupInfo {
    char path[260];
    u64 timestamp;
    u64 file_size;
    Ps14Hash hash;
    bool is_valid;
    struct Ps14BackupInfo* next;
} Ps14BackupInfo;

// Memory backup (for restoring corrupted memory)
typedef struct Ps14MemoryBackup {
    void* address;
    usize size;
    u8* data; // Copy of original memory
    Ps14Hash hash;
    u64 timestamp;
    struct Ps14MemoryBackup* next;
} Ps14MemoryBackup;

// State checkpoint (for rolling back game state)
typedef struct Ps14StateCheckpoint {
    u32 sequence;
    u64 timestamp;
    void* state_data;
    usize data_size;
    struct Ps14StateCheckpoint* next;
} Ps14StateCheckpoint;

// Repair callback
typedef void (*Ps14RepairCallback)(Ps14RepairType type, const char* name, Ps14RepairResult result, void* user_data);

// ============================================================================
// MEMORY REPAIR
// ============================================================================

PS14_API i32 ps14_repair_init_memory_backup(void);
PS14_API void ps14_repair_shutdown_memory_backup(void);

// Create a backup of a memory region
PS14_API Ps14MemoryBackup* ps14_repair_backup_memory(const char* name, void* address, usize size);

// Restore memory from backup
PS14_API Ps14RepairResult ps14_repair_restore_memory(Ps14MemoryRegion* region);
PS14_API Ps14RepairResult ps14_repair_restore_memory_by_name(const char* name);

// Free a memory backup
PS14_API void ps14_repair_free_memory_backup(Ps14MemoryBackup* backup);

// ============================================================================
// FILE RESTORATION
// ============================================================================

PS14_API i32 ps14_repair_init_file_restoration(void);
PS14_API void ps14_repair_shutdown_file_restoration(void);

// Create backup of a file
PS14_API i32 ps14_repair_backup_file(const char* filepath);

// Restore file from backup
PS14_API Ps14RepairResult ps14_repair_restore_file(const char* filepath);

// Verify file integrity and repair if needed
PS14_API Ps14RepairResult ps14_repair_verify_and_fix_file(const char* filepath);

// ============================================================================
// STATE ROLLBACK
// ============================================================================

PS14_API i32 ps14_repair_init_state_roller(void);
PS14_API void ps14_repair_shutdown_state_roller(void);

// Create a state checkpoint
PS14_API i32 ps14_repair_create_checkpoint(u32 sequence, const void* state_data, usize data_size);

// Rollback to a specific checkpoint
PS14_API Ps14RepairResult ps14_repair_rollback_to_sequence(u32 sequence);

// Rollback to last known good state
PS14_API Ps14RepairResult ps14_repair_rollback_to_last_good(void);

// Get last checkpoint sequence
PS14_API u32 ps14_repair_get_last_checkpoint_sequence(void);

// ============================================================================
// BACKUP MANAGER
// ============================================================================

PS14_API i32 ps14_repair_init_backup_manager(void);
PS14_API void ps14_repair_shutdown_backup_manager(void);

// Initialize backup system
PS14_API i32 ps14_backup_init(const char* backup_dir);

// Shutdown backup system
PS14_API void ps14_backup_shutdown(void);

// List all backups
PS14_API Ps14BackupInfo* ps14_backup_list(const char* filepath);

// Free backup info list
PS14_API void ps14_backup_free_list(Ps14BackupInfo* list);

// Get best backup for restoration
PS14_API Ps14BackupInfo* ps14_backup_get_best(const char* filepath);

// Clean old backups (by age or count)
PS14_API i32 ps14_backup_clean_old(u64 max_age_seconds, u32 max_count);

// ============================================================================
// CALLBACKS
// ============================================================================

// Register repair callback
PS14_API void* ps14_repair_register_callback(Ps14RepairCallback callback, void* user_data);

// Unregister repair callback
PS14_API void ps14_repair_unregister_callback(void* handle);

// ============================================================================
// GENERAL REPAIR FUNCTIONS
// ============================================================================

// Attempt to repair any detected tampering
PS14_API Ps14RepairResult ps14_repair_attempt_auto_fix(Ps14MemoryRegion* region);

// Check system integrity and repair all issues
PS14_API u32 ps14_repair_check_and_fix_all(void);

// Get repair statistics
PS14_API void ps14_repair_get_stats(u32* repairs_attempted, u32* repairs_successful);

#ifdef __cplusplus
}
#endif
#endif
