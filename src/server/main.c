#include "ps14/ps14.h"
#include "ps14/logger.h"
#include "ps14/network.h"
#include "ps14/config.h"

#include <stdio.h>

int main(int argc, char** argv) {
    ps14_log_info("ps14 Auth Server starting...");
    
    if (!ps14_config_load("ps14_server_config.json")) {
        ps14_log_error("Failed to load server configuration");
        return 1;
    }
    
    if (!ps14_logger_init("ps14_server.log")) {
        ps14_log_error("Failed to initialize logger");
        return 1;
    }
    
    ps14_log_info("Server initialized. Press Ctrl+C to stop.");
    
    while (1) {
        Sleep(1000);
    }
    
    return 0;
}
