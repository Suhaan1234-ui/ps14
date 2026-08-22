#include "ps14/memory.h"
#include "ps14/hash.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <string.h>
#include <stdlib.h>

// Hash database entry
typedef struct HashDbEntry {
    char name[PS14_MAX_NAME];
    u64 base_address;
    usize size;
    Ps14Hash hash;
    Ps14HashAlgorithm algorithm;
    struct HashDbEntry* next;
} HashDbEntry;

// Global hash database
static HashDbEntry* g_hash_db = NULL;
static Ps14Mutex g_hash_db_mutex;

// Initialize hash database
i32 ps14_hash_db_init(void) {
    ps14_mutex_init(&g_hash_db_mutex);
    PS14_LOG_INFO("Hash database initialized");
    return PS14_SUCCESS;
}

// Shutdown hash database
void ps14_hash_db_shutdown(void) {
    ps14_mutex_lock(&g_hash_db_mutex);
    HashDbEntry* curr = g_hash_db;
    while (curr) {
        HashDbEntry* next = curr->next;
        free(curr);
        curr = next;
    }
    g_hash_db = NULL;
    ps14_mutex_unlock(&g_hash_db_mutex);
    ps14_mutex_destroy(&g_hash_db_mutex);
    PS14_LOG_INFO("Hash database shutdown");
}

// Add or update a hash entry
i32 ps14_hash_db_add_entry(const char* name, u64 address, usize size, const u8* data, Ps14HashAlgorithm algorithm) {
    if (!name || size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    Ps14Hash hash;
    if (ps14_hash_compute(algorithm, data, size, &hash) != PS14_SUCCESS) {
        return PS14_ERROR_UNKNOWN;
    }
    
    ps14_mutex_lock(&g_hash_db_mutex);
    
    // Check if entry already exists
    HashDbEntry* curr = g_hash_db;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            // Update existing entry
            curr->base_address = address;
            curr->size = size;
            curr->hash = hash;
            curr->algorithm = algorithm;
            ps14_mutex_unlock(&g_hash_db_mutex);
            PS14_LOG_DEBUG("Updated hash entry: %s", name);
            return PS14_SUCCESS;
        }
        curr = curr->next;
    }
    
    // Create new entry
    HashDbEntry* entry = (HashDbEntry*)malloc(sizeof(HashDbEntry));
    if (!entry) {
        ps14_mutex_unlock(&g_hash_db_mutex);
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    strncpy(entry->name, name, PS14_MAX_NAME - 1);
    entry->name[PS14_MAX_NAME - 1] = '\0';
    entry->base_address = address;
    entry->size = size;
    entry->hash = hash;
    entry->algorithm = algorithm;
    entry->next = g_hash_db;
    g_hash_db = entry;
    
    ps14_mutex_unlock(&g_hash_db_mutex);
    PS14_LOG_DEBUG("Added hash entry: %s", name);
    return PS14_SUCCESS;
}

// Remove a hash entry
void ps14_hash_db_remove_entry(const char* name) {
    if (!name) {
        return;
    }
    
    ps14_mutex_lock(&g_hash_db_mutex);
    HashDbEntry** curr = &g_hash_db;
    while (*curr) {
        if (strcmp((*curr)->name, name) == 0) {
            HashDbEntry* to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            break;
        }
        curr = &(*curr)->next;
    }
    ps14_mutex_unlock(&g_hash_db_mutex);
    PS14_LOG_DEBUG("Removed hash entry: %s", name);
}

// Find a hash entry
HashDbEntry* ps14_hash_db_find_entry(const char* name) {
    if (!name) {
        return NULL;
    }
    
    ps14_mutex_lock(&g_hash_db_mutex);
    HashDbEntry* curr = g_hash_db;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            ps14_mutex_unlock(&g_hash_db_mutex);
            return curr;
        }
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_hash_db_mutex);
    return NULL;
}

// Verify a memory region against the hash database
bool ps14_hash_db_verify_region(const char* name, void* address, usize size) {
    HashDbEntry* entry = ps14_hash_db_find_entry(name);
    if (!entry) {
        PS14_LOG_WARNING("No hash entry found for: %s", name);
        return false;
    }
    
    if (entry->base_address != (u64)address || entry->size != size) {
        PS14_LOG_WARNING("Region size/address mismatch: %s", name);
        return false;
    }
    
    Ps14Hash current_hash;
    if (ps14_hash_compute(entry->algorithm, (const u8*)address, size, &current_hash) != PS14_SUCCESS) {
        PS14_LOG_WARNING("Failed to compute hash for: %s", name);
        return false;
    }
    
    return ps14_hash_compare(&current_hash, &entry->hash);
}

// Save hash database to file
i32 ps14_hash_db_save(const char* filepath) {
    if (!filepath) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        PS14_LOG_ERROR("Failed to open hash database file: %s", filepath);
        return PS14_ERROR_UNKNOWN;
    }
    
    ps14_mutex_lock(&g_hash_db_mutex);
    HashDbEntry* curr = g_hash_db;
    while (curr) {
        char hash_str[128];
        ps14_hash_to_string(&curr->hash, hash_str, sizeof(hash_str));
        
        fprintf(file, "%s|%llu|%zu|%s|%u\n", 
                curr->name, 
                curr->base_address, 
                curr->size,
                hash_str,
                curr->algorithm);
        
        curr = curr->next;
    }
    ps14_mutex_unlock(&g_hash_db_mutex);
    
    fclose(file);
    PS14_LOG_INFO("Hash database saved to: %s", filepath);
    return PS14_SUCCESS;
}

// Load hash database from file
i32 ps14_hash_db_load(const char* filepath) {
    if (!filepath) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        PS14_LOG_WARNING("Failed to open hash database file: %s", filepath);
        return PS14_SUCCESS; // Not an error if file doesn't exist
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        char name[PS14_MAX_NAME];
        u64 address;
        usize size;
        char hash_str[128];
        u32 algorithm;
        
        if (sscanf(line, "%[^|]|%llu|%zu|%[^|]|%u", name, &address, &size, hash_str, &algorithm) == 5) {
            // Note: We can't restore the actual hash from string yet
            // This is a placeholder for the loading logic
            PS14_LOG_DEBUG("Loaded hash entry: %s", name);
        }
    }
    
    fclose(file);
    PS14_LOG_INFO("Hash database loaded from: %s", filepath);
    return PS14_SUCCESS;
}
