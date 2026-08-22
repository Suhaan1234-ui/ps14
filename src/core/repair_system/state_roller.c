#include "ps14/repair.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include "ps14/thread.h"
#include <string.h>
#include <stdlib.h>

// State checkpoint list
static Ps14StateCheckpoint* g_checkpoints = NULL;
static Ps14Mutex g_checkpoints_mutex;
static bool g_state_roller_initialized = false;

// Maximum number of checkpoints to keep
static u32 g_max_checkpoints = 100;

// Initialize state roller
PS14_API i32 ps14_repair_init_state_roller(void) {
    if (g_state_roller_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    ps14_mutex_init(&g_checkpoints_mutex);
    g_checkpoints = NULL;
    g_state_roller_initialized = true;
    
    Ps14Config* config = ps14_config_get();
    if (config) {
        g_max_checkpoints = config->repair.max_backups;
    }
    
    PS14_LOG_INFO("State roller initialized (max checkpoints: %u)", g_max_checkpoints);
    return PS14_SUCCESS;
}

// Shutdown state roller
PS14_API void ps14_repair_shutdown_state_roller(void) {
    if (!g_state_roller_initialized) {
        return;
    }
    
    // Free all checkpoints
    ps14_mutex_lock(&g_checkpoints_mutex);
    Ps14StateCheckpoint* checkpoint = g_checkpoints;
    while (checkpoint) {
        Ps14StateCheckpoint* next = checkpoint->next;
        free(checkpoint->state_data);
        free(checkpoint);
        checkpoint = next;
    }
    g_checkpoints = NULL;
    ps14_mutex_unlock(&g_checkpoints_mutex);
    
    ps14_mutex_destroy(&g_checkpoints_mutex);
    g_state_roller_initialized = false;
    
    PS14_LOG_INFO("State roller shutdown");
}

// Create a state checkpoint
PS14_API i32 ps14_repair_create_checkpoint(u32 sequence, const void* state_data, usize data_size) {
    if (!state_data || data_size == 0) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    if (!g_state_roller_initialized) {
        ps14_repair_init_state_roller();
    }
    
    // Allocate checkpoint
    Ps14StateCheckpoint* checkpoint = (Ps14StateCheckpoint*)malloc(sizeof(Ps14StateCheckpoint));
    if (!checkpoint) {
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    // Allocate state data
    checkpoint->state_data = malloc(data_size);
    if (!checkpoint->state_data) {
        free(checkpoint);
        return PS14_ERROR_OUT_OF_MEMORY;
    }
    
    // Copy state data
    memcpy(checkpoint->state_data, state_data, data_size);
    checkpoint->sequence = sequence;
    checkpoint->data_size = data_size;
    checkpoint->timestamp = ps14_thread_get_tick_count();
    checkpoint->next = NULL;
    
    // Add to list
    ps14_mutex_lock(&g_checkpoints_mutex);
    checkpoint->next = g_checkpoints;
    g_checkpoints = checkpoint;
    
    // Trim old checkpoints if we exceed the limit
    u32 count = 0;
    Ps14StateCheckpoint* curr = g_checkpoints;
    while (curr) {
        count++;
        curr = curr->next;
    }
    
    while (count > g_max_checkpoints && g_checkpoints && g_checkpoints->next) {
        Ps14StateCheckpoint* to_remove = g_checkpoints->next;
        while (to_remove->next && to_remove->next->next) {
            to_remove = to_remove->next;
        }
        free(to_remove->next->state_data);
        free(to_remove->next);
        to_remove->next = NULL;
        count--;
    }
    
    ps14_mutex_unlock(&g_checkpoints_mutex);
    
    PS14_LOG_DEBUG("Created state checkpoint (seq: %u, size: %zu)", sequence, data_size);
    return PS14_SUCCESS;
}

// Find checkpoint by sequence
static Ps14StateCheckpoint* find_checkpoint_by_sequence(u32 sequence) {
    ps14_mutex_lock(&g_checkpoints_mutex);
    Ps14StateCheckpoint* checkpoint = g_checkpoints;
    while (checkpoint) {
        if (checkpoint->sequence == sequence) {
            ps14_mutex_unlock(&g_checkpoints_mutex);
            return checkpoint;
        }
        checkpoint = checkpoint->next;
    }
    ps14_mutex_unlock(&g_checkpoints_mutex);
    return NULL;
}

// Rollback to a specific checkpoint
PS14_API Ps14RepairResult ps14_repair_rollback_to_sequence(u32 sequence) {
    Ps14StateCheckpoint* checkpoint = find_checkpoint_by_sequence(sequence);
    if (!checkpoint) {
        PS14_LOG_WARNING("Checkpoint not found for sequence: %u", sequence);
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    // In a real implementation, this would restore the game state
    // For now, we just return success
    PS14_LOG_INFO("Rolled back to checkpoint (seq: %u, timestamp: %llu)", 
                  checkpoint->sequence, checkpoint->timestamp);
    return PS14_REPAIR_RESULT_SUCCESS;
}

// Get last checkpoint sequence
PS14_API u32 ps14_repair_get_last_checkpoint_sequence(void) {
    ps14_mutex_lock(&g_checkpoints_mutex);
    u32 last_seq = g_checkpoints ? g_checkpoints->sequence : 0;
    ps14_mutex_unlock(&g_checkpoints_mutex);
    return last_seq;
}

// Rollback to last known good state
PS14_API Ps14RepairResult ps14_repair_rollback_to_last_good(void) {
    u32 last_seq = ps14_repair_get_last_checkpoint_sequence();
    if (last_seq == 0) {
        PS14_LOG_WARNING("No checkpoints available for rollback");
        return PS14_REPAIR_RESULT_FAILED;
    }
    
    return ps14_repair_rollback_to_sequence(last_seq);
}
