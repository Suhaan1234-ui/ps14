#include "ps14/ps14.h"
#include "ps14/logger.h"

#include <stdio.h>

extern int test_client_server_integration();
extern int test_memory_protection_integration();

int main() {
    if (!ps14_logger_init("ps14_integration_tests.log")) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    ps14_log_info("=== ps14 Integration Tests ===");
    
    if (test_client_server_integration() != 0) {
        ps14_log_error("Client-server test failed");
        return 1;
    }
    
    if (test_memory_protection_integration() != 0) {
        ps14_log_error("Memory protection test failed");
        return 1;
    }
    
    ps14_log_info("=== All Integration Tests Passed ===");
    ps14_logger_shutdown();
    return 0;
}
