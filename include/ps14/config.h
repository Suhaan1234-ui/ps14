#ifndef PS14_CONFIG_H
#define PS14_CONFIG_H

#pragma once

#include "ps14.h"
#include "ps14/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory monitor configuration
typedef struct {
    bool enabled;
    u32 audit_interval_ms;
    u32 thread_count;
    bool protect_regions;
    char regions_file[PS14_MAX_PATH];
} Ps14MemoryMonitorConfig;

// Integrity checker configuration
typedef struct {
    bool enabled;
    u32 check_interval_ms;
    Ps14HashAlgorithm hash_algorithm;
    char checksum_db_path[PS14_MAX_PATH];
} Ps14IntegrityCheckerConfig;

// Authentication configuration
typedef struct {
    bool enabled;
    char server_address[PS14_MAX_PATH];
    u16 server_port;
    char default_token[PS14_MAX_NAME];
    u64 session_timeout_seconds;
} Ps14AuthConfig;

// Network configuration
typedef struct {
    bool use_ssl;
    char cert_path[PS14_MAX_PATH];
    char key_path[PS14_MAX_PATH];
    u32 connection_timeout;
    u32 retry_count;
} Ps14NetworkConfig;

// Repair system configuration
typedef struct {
    bool enabled;
    char backup_dir[PS14_MAX_PATH];
    u32 max_backups;
    bool auto_repair;
} Ps14RepairConfig;

// Kernel driver configuration
typedef struct {
    bool enabled;
    char driver_path[PS14_MAX_PATH];
    char device_name[PS14_MAX_NAME];
} Ps14KernelConfig;

// Performance configuration
typedef struct {
    u32 max_cpu_usage_percent;
    u32 max_memory_mb;
    bool enable_throttling;
} Ps14PerformanceConfig;

// Main configuration structure
typedef struct Ps14Config {
    bool enabled;
    Ps14LogLevel log_level;
    char log_file[PS14_MAX_PATH];
    
    Ps14MemoryMonitorConfig memory_monitor;
    Ps14IntegrityCheckerConfig integrity_checker;
    Ps14AuthConfig auth;
    Ps14NetworkConfig network;
    Ps14RepairConfig repair;
    Ps14KernelConfig kernel;
    Ps14PerformanceConfig performance;
} Ps14Config;

// Configuration functions
PS14_API i32 ps14_config_init(void);
PS14_API void ps14_config_shutdown(void);
PS14_API Ps14Config* ps14_config_get(void);
PS14_API const Ps14Config* ps14_config_get_const(void);
PS14_API i32 ps14_config_load(const char* filepath);
PS14_API i32 ps14_config_save(const char* filepath);

#ifdef __cplusplus
}
#endif
#endif
