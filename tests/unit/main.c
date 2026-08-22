#include <stdio.h>

// Forward declarations
extern void run_memory_monitor_tests(void);
extern void run_integrity_check_tests(void);
extern void run_auth_gateway_tests(void);
extern void run_repair_system_tests(void);
extern void run_advanced_tests(void);

int main() {
    printf("==========================================\n");
    printf("ps14 Anti-Tamper Engine - Unit Tests\n");
    printf("==========================================\n\n");
    
    // Run all test suites
    run_memory_monitor_tests();
    run_integrity_check_tests();
    run_auth_gateway_tests();
    run_repair_system_tests();
    run_advanced_tests();
    
    printf("==========================================\n");
    printf("All Unit Tests Complete\n");
    printf("==========================================\n");
    
    return 0;
}
