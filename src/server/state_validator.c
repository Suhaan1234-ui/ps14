#include "ps14/ps14.h"
#include "ps14/logger.h"
#include "ps14/hash.h"

typedef struct {
    u32 client_id;
    u64 last_state_hash;
    u64 last_update_time;
} ClientState;

static ClientState g_client_states[1024];
static u32 g_client_count = 0;

bool ps14_state_validator_init() {
    return true;
}

void ps14_state_validator_shutdown() {
}

bool ps14_state_validator_register_client(u32 client_id, u64 initial_hash) {
    if (g_client_count >= 1024) return false;
    g_client_states[g_client_count].client_id = client_id;
    g_client_states[g_client_count].last_state_hash = initial_hash;
    g_client_states[g_client_count].last_update_time = time(NULL);
    g_client_count++;
    return true;
}

bool ps14_state_validator_update_client(u32 client_id, u64 new_hash) {
    for (u32 i = 0; i < g_client_count; i++) {
        if (g_client_states[i].client_id == client_id) {
            g_client_states[i].last_state_hash = new_hash;
            g_client_states[i].last_update_time = time(NULL);
            return true;
        }
    }
    return false;
}

bool ps14_state_validator_validate_client(u32 client_id, u64 client_hash) {
    for (u32 i = 0; i < g_client_count; i++) {
        if (g_client_states[i].client_id == client_id) {
            return g_client_states[i].last_state_hash == client_hash;
        }
    }
    return false;
}
