#include "ps14/integrity.h"
#include "ps14/hash.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <string.h>
#include <stdlib.h>

// File info list
static Ps14FileInfo* g_files = NULL;
static Ps14Mutex g_files_mutex;

// Initialize integrity checker
i32 ps14_integrity_init(void) {
    ps14_mutex_init(&g_files_mutex);
    PS14_LOG_INFO("Integrity checker initialized");
    return PS14_SUCCESS;
}

// Shutdown integrity checker
void ps14_integrity_shutdown(void) {
    ps14_mutex_lock(&g_files_mutex);
    Ps14FileInfo* curr = g_files;
    while (curr) {
        Ps14FileInfo* next = curr;
        // Note: curr is not malloc'd, it's part of the array
        // We'll fix this when we implement proper memory management
        curr = next;
    }
    g_files = NULL;
    ps14_mutex_unlock(&g_files_mutex);
    ps14_mutex_destroy(&g_files_mutex);
    PS14_LOG_INFO("Integrity checker shutdown");
}

// Add a file to verify
i32 ps14_integrity_add_file(const char* path) {
    if (!path) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    // Check if file already exists
    ps14_mutex_lock(&g_files_mutex);
    for (usize i = 0; g_files && i < PS14_MAX_REGIONS; i++) {
        if (strcmp(g_files[i].path, path) == 0) {
            ps14_mutex_unlock(&g_files_mutex);
            return PS14_SUCCESS; // Already exists
        }
    }
    ps14_mutex_unlock(&g_files_mutex);
    
    // Calculate hash
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_SHA256;
    
    Ps14Hash hash;
    if (ps14_hash_compute_file(algo, path, &hash) != PS14_SUCCESS) {
        PS14_LOG_WARNING("Failed to compute hash for file: %s", path);
        return PS14_ERROR_UNKNOWN;
    }
    
    // Add to list
    ps14_mutex_lock(&g_files_mutex);
    
    // Find empty slot
    for (usize i = 0; i < PS14_MAX_REGIONS; i++) {
        if (g_files[i].path[0] == '\0') {
            strncpy(g_files[i].path, path, PS14_MAX_PATH - 1);
            g_files[i].path[PS14_MAX_PATH - 1] = '\0';
            ps14_hash_copy(&g_files[i].hash, &hash);
            g_files[i].last_modified = 0; // TODO: Get actual timestamp
            g_files[i].is_valid = true;
            ps14_mutex_unlock(&g_files_mutex);
            PS14_LOG_DEBUG("Added file to integrity check: %s", path);
            return PS14_SUCCESS;
        }
    }
    
    ps14_mutex_unlock(&g_files_mutex);
    PS14_LOG_ERROR("No space for more files in integrity checker");
    return PS14_ERROR_RESOURCE_EXHAUSTION;
}

// Remove a file from verification
void ps14_integrity_remove_file(const char* path) {
    if (!path) {
        return;
    }
    
    ps14_mutex_lock(&g_files_mutex);
    for (usize i = 0; g_files && i < PS14_MAX_REGIONS; i++) {
        if (strcmp(g_files[i].path, path) == 0) {
            g_files[i].path[0] = '\0';
            g_files[i].is_valid = false;
            break;
        }
    }
    ps14_mutex_unlock(&g_files_mutex);
    PS14_LOG_DEBUG("Removed file from integrity check: %s", path);
}

// Verify a file's integrity
bool ps14_integrity_verify_file(const char* path) {
    if (!path) {
        return false;
    }
    
    Ps14Config* config = ps14_config_get();
    Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_SHA256;
    
    Ps14Hash current_hash;
    if (ps14_hash_compute_file(algo, path, &current_hash) != PS14_SUCCESS) {
        PS14_LOG_WARNING("Failed to compute hash for file: %s", path);
        return false;
    }
    
    ps14_mutex_lock(&g_files_mutex);
    for (usize i = 0; g_files && i < PS14_MAX_REGIONS; i++) {
        if (strcmp(g_files[i].path, path) == 0) {
            bool valid = ps14_hash_compare(&current_hash, &g_files[i].hash);
            ps14_mutex_unlock(&g_files_mutex);
            
            if (!valid) {
                PS14_LOG_WARNING("File tampering detected: %s", path);
            }
            return valid;
        }
    }
    ps14_mutex_unlock(&g_files_mutex);
    
    PS14_LOG_WARNING("File not in integrity database: %s", path);
    return false;
}

// Scan all files for tampering
i32 ps14_integrity_scan_all(void) {
    if (!g_files) {
        return 0;
    }
    
    i32 tampered_count = 0;
    
    ps14_mutex_lock(&g_files_mutex);
    for (usize i = 0; i < PS14_MAX_REGIONS; i++) {
        if (g_files[i].path[0] != '\0' && g_files[i].is_valid) {
            if (!ps14_integrity_verify_file(g_files[i].path)) {
                tampered_count++;
            }
        }
    }
    ps14_mutex_unlock(&g_files_mutex);
    
    if (tampered_count > 0) {
        PS14_LOG_WARNING("Integrity scan found %d tampered files", tampered_count);
    }
    
    return tampered_count;
}

// Verify code signature (Windows-specific)
bool ps14_integrity_verify_code_signature(const char* path) {
    if (!path) {
        return false;
    }
    
    #ifdef PS14_PLATFORM_WINDOWS
    WINTRUST_FILE_INFO file_info = {0};
    WINTRUST_DATA wintrust_data = {0};
    GUID action_guid = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    file_info.cbStruct = sizeof(file_info);
    file_info.pcwszFilePath = (LPCWSTR)path; // Note: Should be wide char
    
    wintrust_data.cbStruct = sizeof(wintrust_data);
    wintrust_data.pPolicyCallbackData = NULL;
    wintrust_data.pSIPClientData = NULL;
    wintrust_data.dwUIChoice = WTD_UI_NONE;
    wintrust_data.fdwRevocationChecks = WTD_REVOCATION_CHECK_NONE;
    wintrust_data.dwUnionChoice = WTD_CHOICE_FILE;
    wintrust_data.pFile = &file_info;
    wintrust_data.dwStateAction = WTD_STATEACTION_VERIFY;
    wintrust_data.hWVTStateData = NULL;
    wintrust_data.pwszURLReference = NULL;
    wintrust_data.dwProvFlags = 0;
    wintrust_data.dwUIContext = 0;
    
    LONG result = WinVerifyTrust(NULL, &action_guid, &wintrust_data);
    
    if (result == ERROR_SUCCESS) {
        PS14_LOG_DEBUG("Code signature verified: %s", path);
        return true;
    } else {
        PS14_LOG_WARNING("Code signature verification failed: %s (error: %ld)", path, result);
        return false;
    }
    
    #else
    PS14_LOG_WARNING("Code signature verification not implemented for this platform");
    return false;
    #endif
}
