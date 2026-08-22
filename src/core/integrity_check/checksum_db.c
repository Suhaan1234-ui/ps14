#include "ps14/integrity.h"
#include "ps14/hash.h"
#include "ps14/logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_CHECKSUM_ENTRIES 1024

typedef struct ChecksumEntry {
    char path[PS14_MAX_PATH];
    Ps14Hash hash;
    u64 timestamp;
    bool valid;
} ChecksumEntry;

static ChecksumEntry g_checksum_db[MAX_CHECKSUM_ENTRIES];
static bool g_checksum_db_initialized = false;
static char g_checksum_db_path[PS14_MAX_PATH] = "checksums.db";

// Initialize checksum database
i32 ps14_checksum_db_init(const char* db_path) {
    if (g_checksum_db_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    if (db_path) {
        strncpy(g_checksum_db_path, db_path, PS14_MAX_PATH - 1);
        g_checksum_db_path[PS14_MAX_PATH - 1] = '\0';
    }
    
    memset(g_checksum_db, 0, sizeof(g_checksum_db));
    g_checksum_db_initialized = true;
    
    // Try to load existing database
    ps14_checksum_db_load();
    
    PS14_LOG_INFO("Checksum database initialized");
    return PS14_SUCCESS;
}

// Shutdown checksum database
void ps14_checksum_db_shutdown(void) {
    if (!g_checksum_db_initialized) {
        return;
    }
    
    // Save database
    ps14_checksum_db_save();
    
    g_checksum_db_initialized = false;
    PS14_LOG_INFO("Checksum database shutdown");
}

// Add a checksum entry
i32 ps14_checksum_db_add(const char* path, const Ps14Hash* hash) {
    if (!path || !hash) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!g_checksum_db_initialized) {
        ps14_checksum_db_init(NULL);
    }
    
    // Check if already exists
    for (usize i = 0; i < MAX_CHECKSUM_ENTRIES; i++) {
        if (strcmp(g_checksum_db[i].path, path) == 0) {
            // Update existing
            ps14_hash_copy(&g_checksum_db[i].hash, hash);
            g_checksum_db[i].valid = true;
            PS14_LOG_DEBUG("Updated checksum: %s", path);
            return PS14_SUCCESS;
        }
    }
    
    // Find empty slot
    for (usize i = 0; i < MAX_CHECKSUM_ENTRIES; i++) {
        if (g_checksum_db[i].path[0] == '\0') {
            strncpy(g_checksum_db[i].path, path, PS14_MAX_PATH - 1);
            g_checksum_db[i].path[PS14_MAX_PATH - 1] = '\0';
            ps14_hash_copy(&g_checksum_db[i].hash, hash);
            g_checksum_db[i].timestamp = 0; // TODO: Get timestamp
            g_checksum_db[i].valid = true;
            PS14_LOG_DEBUG("Added checksum: %s", path);
            return PS14_SUCCESS;
        }
    }
    
    PS14_LOG_ERROR("Checksum database full");
    return PS14_ERROR_RESOURCE_EXHAUSTION;
}

// Verify a file against the checksum database
bool ps14_checksum_db_verify(const char* path) {
    if (!path) {
        return false;
    }
    
    if (!g_checksum_db_initialized) {
        ps14_checksum_db_init(NULL);
    }
    
    // Find entry
    for (usize i = 0; i < MAX_CHECKSUM_ENTRIES; i++) {
        if (strcmp(g_checksum_db[i].path, path) == 0 && g_checksum_db[i].valid) {
            // Compute current hash
            Ps14Config* config = ps14_config_get();
            Ps14HashAlgorithm algo = config ? config->integrity_checker.hash_algorithm : PS14_HASH_SHA256;
            
            Ps14Hash current_hash;
            if (ps14_hash_compute_file(algo, path, &current_hash) != PS14_SUCCESS) {
                PS14_LOG_WARNING("Failed to compute hash for: %s", path);
                return false;
            }
            
            bool valid = ps14_hash_compare(&current_hash, &g_checksum_db[i].hash);
            if (!valid) {
                PS14_LOG_WARNING("Checksum mismatch: %s", path);
            }
            return valid;
        }
    }
    
    PS14_LOG_WARNING("File not in checksum database: %s", path);
    return false;
}

// Save database to file
static i32 ps14_checksum_db_save(void) {
    FILE* file = fopen(g_checksum_db_path, "w");
    if (!file) {
        PS14_LOG_ERROR("Failed to open checksum database file: %s", g_checksum_db_path);
        return PS14_ERROR_UNKNOWN;
    }
    
    for (usize i = 0; i < MAX_CHECKSUM_ENTRIES; i++) {
        if (g_checksum_db[i].path[0] != '\0' && g_checksum_db[i].valid) {
            char hash_str[128];
            ps14_hash_to_string(&g_checksum_db[i].hash, hash_str, sizeof(hash_str));
            fprintf(file, "%s|%s\n", g_checksum_db[i].path, hash_str);
        }
    }
    
    fclose(file);
    PS14_LOG_INFO("Checksum database saved to: %s", g_checksum_db_path);
    return PS14_SUCCESS;
}

// Load database from file
static i32 ps14_checksum_db_load(void) {
    FILE* file = fopen(g_checksum_db_path, "r");
    if (!file) {
        PS14_LOG_DEBUG("No checksum database file found: %s", g_checksum_db_path);
        return PS14_SUCCESS;
