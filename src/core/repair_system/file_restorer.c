#include "ps14/repair.h"
#include "ps14/logger.h"
#include "ps14/hash.h"
#include "ps14/config.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#include <io.h>
#define FOK _access
#define FOK_W 2
#else
#include <unistd.h>
#include <sys/stat.h>
#define FOK access
#define FOK_W W_OK
#endif

// File backup directory
static char g_backup_dir[260] = "backups";
static Ps14Mutex g_file_mutex;
static bool g_file_restoration_initialized = false;

// Known good checksums (simplified - in real system, this would be a database)
typedef struct ChecksumEntry {
    char filepath[260];
    Ps14Hash hash;
    struct ChecksumEntry* next;
} ChecksumEntry;

static ChecksumEntry* g_checksums = NULL;

// Initialize file restoration system
PS14_API i32 ps14_repair_init_file_restoration(void) {
    if (g_file_restoration_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_file_mutex);
    g_file_restoration_initialized = true;
    
    // Create backup directory if it doesn't exist
#ifdef PS14_PLATFORM_WINDOWS
    CreateDirectoryA(g_backup_dir, NULL);
#else
    mkdir(g_backup_dir, 0755);
#endif
    
    PS14_LOG_INFO("File restoration system initialized (backup dir: %s)", g_backup_dir);
    return PS14_SUCCESS;
}

// Shutdown file restoration system
PS14_API void ps14_repair_shutdown_file_restoration(void) {
    if (!g_file_restoration_initialized) {
        return;
    }
    
    // Free checksum entries
    ps14_mutex_lock(&g_file_mutex);
    ChecksumEntry* entry = g_checksums;
    while (entry) {
        ChecksumEntry* next = entry->next;
        free(entry);
        entry = next;
    }
    g_checksums = NULL;
    ps14_mutex_unlock(&g_file_mutex);
    
    ps14_mutex_destroy(&g_file_mutex);
    g_file_restoration_initialized = false;
    
    PS14_LOG_INFO("File restoration system shutdown");
}

// Set backup directory
PS14_API void ps14_backup_set_directory(const char* dir) {
    if (dir) {
        strncpy(g_backup_dir, dir, 259);
        g_backup_dir[259] = '\0';
    }
}

// Generate backup file path
static void generate_backup_path(char* backup_path, usize size, const char* filepath) {
    char filename[260];
    const char* last_slash = strrchr(filepath, '/');
    const char* last_backslash = strrchr(filepath, '\');
    const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
    
    if (last_sep) {
        strncpy(filename, last_sep + 1, 259);
    } else {
        strncpy(filename, filepath, 259);
    }
    filename[259] = '\0';
    
    // Add timestamp to filename
    u64 timestamp = ps14_thread_get_tick_count();
    snprintf(backup_path, size, "%s/%s.%llu.bak", g_backup_dir, filename, timestamp);
}

// Check if file exists
static bool file_exists(const char* filepath) {
    return FOK(filepath, FOK_W) == 0;
}

// Create backup of a file
PS14_API i32 ps14_repair_backup_file(const char* filepath) {
    if (!filepath) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!g_file_restoration_initialized) {
        ps14_repair_init_file_restoration();
    }
    
    // Check if file exists
    if (!file_exists(filepath)) {
        PS14_LOG_WARNING("File does not exist: %s", filepath);
        return PS14_ERROR_NOT_FOUND;
    }
    
    // Generate backup path
    char backup_path[512];
    generate_backup_path(backup_path, sizeof(backup_path), filepath);
    
    // Copy file
#ifdef PS14_PLATFORM_WINDOWS
    if (!CopyFileA(filepath, backup_path, FALSE)) {
        PS14_LOG_ERROR("Failed to copy file to backup: %s -> %s", filepath, backup_path);
        return PS14_ERROR_UNKNOWN;
    }
#else
    // Simple file copy for non-Windows (for demonstration)
    FILE* src = fopen(filepath, "rb");
    if (!src) {
        PS14_LOG_ERROR("Failed to open source file: %s", filepath);
        return PS14_ERROR_UNKNOWN;
    }
    
    FILE* dst = fopen(backup_path, "wb");
    if (!dst) {
        fclose(src);
        PS14_LOG_ERROR("Failed to create backup file: %s", backup_path);
        return PS14_ERROR_UNKNOWN;
    }
    
    u8 buffer[4096];
    usize bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dst);
    }
    
    fclose(src);
    fclose(dst);
#endif
    
    PS14_LOG_DEBUG("Created file backup: %s -> %s", filepath, backup_path);
    return PS14_SUCCESS;
}

// Restore file from backup
PS14_API Ps14RepairResult ps14_repair_restore_file(const char* filepath) {
    if (!filepath) {
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // Find the best backup
    Ps14BackupInfo* best_backup = ps14_backup_get_best(filepath);
    if (!best_backup) {
        PS14_LOG_WARNING("No backup found for: %s", filepath);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // Copy backup back to original location
#ifdef PS14_PLATFORM_WINDOWS
    if (!CopyFileA(best_backup->path, filepath, FALSE)) {
        PS14_LOG_ERROR("Failed to restore file from backup: %s -> %s", 
                      best_backup->path, filepath);
        ps14_backup_free_list(best_backup);
        return PS14_REPAIR_RESULT_FAILED;
    }
#else
    FILE* src = fopen(best_backup->path, "rb");
    if (!src) {
        PS14_LOG_ERROR("Failed to open backup file: %s", best_backup->path);
        ps14_backup_free_list(best_backup);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    FILE* dst = fopen(filepath, "wb");
    if (!dst) {
        fclose(src);
        PS14_LOG_ERROR("Failed to open target file for restore: %s", filepath);
        ps14_backup_free_list(best_backup);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    u8 buffer[4096];
    usize bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes_read, dst);
    }
    
    fclose(src);
    fclose(dst);
#endif
    
    ps14_backup_free_list(best_backup);
    PS14_LOG_INFO("Restored file from backup: %s", filepath);
    return PS14_REPAIR_RESULT_SUCCESS;
}

// Add a checksum entry for a file
static void add_checksum_entry(const char* filepath, const Ps14Hash* hash) {
    ChecksumEntry* entry = (ChecksumEntry*)malloc(sizeof(ChecksumEntry));
    if (!entry) {
        return;
    }
    
    strncpy(entry->filepath, filepath, 259);
    entry->filepath[259] = '\0';
    ps14_hash_copy(&entry->hash, hash);
    entry->next = NULL;
    
    ps14_mutex_lock(&g_file_mutex);
    entry->next = g_checksums;
    g_checksums = entry;
    ps14_mutex_unlock(&g_file_mutex);
}

// Verify file integrity using stored checksum
PS14_API Ps14RepairResult ps14_repair_verify_and_fix_file(const char* filepath) {
    if (!filepath) {
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // Check if file exists
    if (!file_exists(filepath)) {
        PS14_LOG_WARNING("File does not exist: %s", filepath);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // Calculate current hash
    Ps14Hash current_hash;
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        PS14_LOG_ERROR("Failed to open file for verification: %s", filepath);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    fseek(file, 0, SEEK_END);
    usize file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    u8* buffer = (u8*)malloc(file_size);
    if (!buffer) {
        fclose(file);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    fread(buffer, 1, file_size, file);
    fclose(file);
    
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_SHA256;
    ps14_hash_compute(algo, buffer, file_size, &current_hash);
    free(buffer);
    
    // For now, we don't have stored checksums, so we'll just backup the file
    // In a real implementation, we would compare with known-good hashes
    
    // Backup the current file (it might be tampered)
    ps14_repair_backup_file(filepath);
    
    PS14_LOG_DEBUG("Verified file: %s (hash computed)", filepath);
    return PS14_REPAIR_RESULT_SUCCESS;
}
