#include "ps14/auth.h"
#include "ps14/logger.h"
#include "ps14/config.h"
#include <stdio.h>

// Test auth callback
static void test_auth_callback(Ps14AuthResult* result, void* user_data) {
    if (result->code == PS14_SUCCESS) {
        printf("[AUTH CALLBACK] Success: %s\n", result->message);
    } else {
        printf("[AUTH CALLBACK] Error: %s\n", result->message);
    }
}

// Test authentication initialization
void test_auth_init(void) {
    i32 result = ps14_auth_init("localhost", 8080);
    if (result == PS14_SUCCESS) {
        printf("[PASS] Auth gateway initialized\n");
    } else {
        printf("[FAIL] Auth gateway initialization failed: %d\n", result);
    }
}

// Test login
void test_auth_login(void) {
    i32 result = ps14_auth_login("testuser", "testpass", test_auth_callback, NULL);
    if (result == PS14_SUCCESS) {
        printf("[PASS] Login request initiated\n");
    } else {
        printf("[FAIL] Login failed: %d\n", result);
    }
}

// Test session creation
void test_session_create(void) {
    Ps14Session* session = ps14_session_create("user123", "testuser", 3600);
    if (session) {
        printf("[PASS] Session created: %s for user %s\n", session->session_id, session->username);
        ps14_session_destroy(session);
    } else {
        printf("[FAIL] Session creation failed\n");
    }
}

// Test session validation
void test_session_validation(void) {
    Ps14Session* session = ps14_session_create("user456", "testuser2", 3600);
    if (session) {
        i32 result = ps14_auth_validate_session(session->session_id);
        if (result == PS14_SUCCESS) {
            printf("[PASS] Session validation passed\n");
        } else {
            printf("[FAIL] Session validation failed: %d\n", result);
        }
        ps14_session_destroy(session);
    }
}

// Test logout
void test_auth_logout(void) {
    Ps14Session* session = ps14_session_create("user789", "testuser3", 3600);
    if (session) {
        i32 result = ps14_auth_logout(session->session_id);
        if (result == PS14_SUCCESS) {
            printf("[PASS] Logout successful\n");
        } else {
            printf("[FAIL] Logout failed: %d\n", result);
        }
    }
}

// Test state synchronization
void test_state_sync(void) {
    i32 result = ps14_state_sync_init();
    if (result == PS14_SUCCESS) {
        printf("[PASS] State sync initialized\n");
        
        // Test sending update
        u32 test_data = 42;
        result = ps14_state_send_update(1, &test_data, sizeof(test_data));
        if (result == PS14_SUCCESS) {
            printf("[PASS] State update sent\n");
        } else {
            printf("[INFO] State update send returned: %d (expected if no server)\n", result);
        }
        
        ps14_state_sync_shutdown();
    } else {
        printf("[FAIL] State sync initialization failed: %d\n", result);
    }
}

// Test token generation
void test_token_generation(void) {
    char token[256];
    i32 result = ps14_token_generate(token, sizeof(token));
    if (result == PS14_SUCCESS) {
        printf("[PASS] Token generated: %.10s...\n", token);
    } else {
        printf("[FAIL] Token generation failed: %d\n", result);
    }
}

// Run all auth gateway tests
void run_auth_gateway_tests(void) {
    printf("\n=== Auth Gateway Tests ===\n");
    
    ps14_config_init();
    ps14_logger_init(PS14_LOG_LEVEL_DEBUG, "test_auth.log");
    
    test_auth_init();
    test_auth_login();
    test_session_create();
    test_session_validation();
    test_auth_logout();
    test_state_sync();
    test_token_generation();
    
    ps14_auth_shutdown();
    ps14_logger_shutdown();
    ps14_config_shutdown();
    printf("=== Auth Gateway Tests Complete ===\n\n");
}
