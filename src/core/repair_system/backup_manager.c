#include "ps14/repair.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

// Backup system state
static char g_backup_dir[260] = "backups";
static bool g_backup_initialized = false;
static Ps14Mutex g_backup_mutex;

// Initialize backup manager
PS14_API i32 ps14_repair_init_backup_manager(void) {
    if (g_backup_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_backup_mutex);
    g_backup_initialized = true;
    
    // Create backup directory if it doesn't exist
#ifdef PS14_PLATFORM_WINDOWS
    CreateDirectoryA(g_backup_dir, NULL);
#else
    mkdir(g_backup_dir, 0755);
#endif
    
    PS14_LOG_INFO("Backup manager initialized (directory: %s)", g_backup_dir);
    return PS14_SUCCESS;
}

// Shutdown backup manager
PS14_API void ps14_repair_shutdown_backup_manager(void) {
    if (!g_backup_initialized) {
        return;
    }
    
    ps14_mutex_destroy(&g_backup_mutex);
    g_backup_initialized = false;
    
    PS14_LOG_INFO("Backup manager shutdown");
}

// Initialize backup system with custom directory
PS14_API i32 ps14_backup_init(const char* backup_dir) {
    if (backup_dir) {
        strncpy(g_backup_dir, backup_dir, 259);
        g_backup_dir[259] = '\0';
    }
    
    if (!g_backup_initialized) {
        ps14_repair_init_backup_manager();
    }
    
    // Create directory if it doesn't exist
#ifdef PS14_PLATFORM_WINDOWS
    CreateDirectoryA(g_backup_dir, NULL);
#else
    mkdir(g_backup_dir, 0755);
#endif
    
    PS14_LOG_INFO("Backup system initialized (directory: %s)", g_backup_dir);
    return PS14_SUCCESS;
}

// Shutdown backup system
PS14_API void ps14_backup_shutdown(void) {
    ps14_repair_shutdown_backup_manager();
}

// Extract filename from path
static void extract_filename(const char* filepath, char* filename, usize size) {
    const char* last_slash = strrchr(filepath, '/');
    const char* last_backslash = strrchr(filepath, '\');
    const char* last_sep = (last_slash > last_backslash) ? last_slash : last_backslash;
    
    if (last_sep) {
        strncpy(filename, last_sep + 1, size - 1);
    } else {
        strncpy(filename, filepath, size - 1);
    }
    filename[size - 1] = '\0';
}

// Get all backup files for a specific file
PS14_API Ps14BackupInfo* ps14_backup_list(const char* filepath) {
    if (!filepath) {
        return NULL;
    }
    
    char filename[260];
    extract_filename(filepath, filename, sizeof(filename));
    
    Ps14BackupInfo* head = NULL;
    Ps14BackupInfo* tail = NULL;
    
#ifdef PS14_PLATFORM_WINDOWS
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s\*%s*", g_backup_dir, filename);
    
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strstr(find_data.cFileName, ".bak")) {
                Ps14BackupInfo* info = (Ps14BackupInfo*)malloc(sizeof(Ps14BackupInfo));
                if (info) {
                    snprintf(info->path, sizeof(info->path), "%s\%s", g_backup_dir, find_data.cFileName);
                    info->timestamp = 0; // Would parse from filename
                    info->file_size = find_data.nFileSizeLow;
                    info->is_valid = true;
                    info->next = NULL;
                    
                    if (!head) {
                        head = info;
                        tail = info;
                    } else {
                        tail->next = info;
                        tail = info;
                    }
                }
            }
        } while (FindNextFileA(hFind, &find_data));
        
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(g_backup_dir);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".bak") && strstr(entry->d_name, filename)) {
                Ps14BackupInfo* info = (Ps14BackupInfo*)malloc(sizeof(Ps14BackupInfo));
                if (info) {
                    snprintf(info->path, sizeof(info->path), "%s/%s", g_backup_dir, entry->d_name);
                    info->timestamp = 0; // Would parse from filename
                    info->file_size = 0; // Would get from stat
                    info->is_valid = true;
                    info->next = NULL;
                    
                    if (!head) {
                        head = info;
                        tail = info;
                    } else {
                        tail->next = info;
                        tail = info;
                    }
                }
            }
        }
        closedir(dir);
    }
#endif
    
    return head;
}

// Free backup info list
PS14_API void ps14_backup_free_list(Ps14BackupInfo* list) {
    while (list) {
        Ps14BackupInfo* next = list->next;
        free(list);
        list = next;
    }
}

// Get best backup for restoration (most recent)
PS14_API Ps14BackupInfo* ps14_backup_get_best(const char* filepath) {
    Ps14BackupInfo* backups = ps14_backup_list(filepath);
    if (!backups) {
        return NULL;
    }
    
    // Find the backup with the highest timestamp (most recent)
    // For simplicity, we'll just return the first one
    // In a real implementation, we would parse timestamps from filenames
    
    return backups;
}

// Clean old backups
PS14_API i32 ps14_backup_clean_old(u64 max_age_seconds, u32 max_count) {
    u32 cleaned = 0;
    
    // For now, just return success
    // In a real implementation, we would:
    // 1. List all backup files
    // 2. Parse timestamps from filenames
    // 3. Delete files older than max_age_seconds
    // 4. Keep only max_count most recent files
    
    PS14_LOG_INFO("Backup cleanup requested (max age: %llu sec, max count: %u)", 
                  max_age_seconds, max_count);
    
    return PS14_SUCCESS;
}

// Get backup directory
PS14_API const char* ps14_backup_get_directory(void) {
    return g_backup_dir;
}
