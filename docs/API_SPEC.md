# ps14 API Specification

**Version:** 1.0.0  
**Last Updated:** August 23, 2026  
**Author:** Mistral Vibe

---

## Overview

This document describes the public API for the ps14 Anti-Tamper Game Engine. The API is organized by component and provides function signatures, parameters, return values, and usage examples.

---

## Table of Contents

1. [Core Library](#core-library)
2. [Memory Monitor](#memory-monitor)
3. [Integrity Checker](#integrity-checker)
4. [Authentication Gateway](#authentication-gateway)
5. [Repair System](#repair-system)
6. [Advanced Detection](#advanced-detection)
7. [Kernel Driver](#kernel-driver)
8. [Client Launcher](#client-launcher)
9. [Server Components](#server-components)

---

## Core Library

### Initialization

```c
// Initialize the ps14 protection engine
bool ps14_init();

// Shutdown the ps14 protection engine
void ps14_shutdown();

// Get engine version
void ps14_get_version(u32* major, u32* minor, u32* patch);
```

---

## Memory Monitor

### Memory Protection

```c
// Protect a memory region
typedef struct {
    void* address;
    usize size;
    bool read_only;
    bool write_watch;
    bool execute_protect;
} Ps14MemoryRegion;

bool ps14_memory_protect(Ps14MemoryRegion* region);
bool ps14_memory_unprotect(Ps14MemoryRegion* region);

// Scan memory for tampering
u32 ps14_memory_scan(Ps14MemoryRegion* regions, u32 count, bool* violations);

// Set async auditing parameters
void ps14_memory_set_audit_interval(u32 milliseconds);
void ps14_memory_set_thread_count(u32 threads);
```

### Hashing

```c
// Supported hash algorithms
typedef enum {
    PS14_HASH_CRC32,
    PS14_HASH_MD5,
    PS14_HASH_SHA256
} Ps14HashAlgorithm;

// Calculate hash of memory region
u32 ps14_hash_crc32(void* data, usize size);
void ps14_hash_md5(void* data, usize size, u8* output);
void ps14_hash_sha256(void* data, usize size, u8* output);

// Generic hash function
bool ps14_hash(Ps14HashAlgorithm algo, void* data, usize size, u8* output, usize* output_size);
```

---

## Integrity Checker

### File Verification

```c
// Verify a file's integrity
typedef struct {
    char path[PS14_MAX_PATH];
    u8 hash[64];
    Ps14HashAlgorithm algo;
} Ps14FileChecksum;

bool ps14_verify_file(const char* path, Ps14FileChecksum* expected);
bool ps14_verify_file_with_hash(const char* path, Ps14HashAlgorithm algo, const u8* expected_hash);

// Add file to checksum database
bool ps14_checksum_db_add(const char* path, Ps14HashAlgorithm algo, const u8* hash);
bool ps14_checksum_db_verify(const char* path);

// Verify all registered files
bool ps14_checksum_db_verify_all();
```

### Code Signature Verification

```c
// Verify PE file signature
bool ps14_verify_pe_signature(const char* path);
bool ps14_verify_module_signature(HMODULE module);
```

---

## Authentication Gateway

### Session Management

```c
// Connect to authentication server
HANDLE ps14_auth_connect(const char* server_ip, u16 port);
void ps14_auth_disconnect(HANDLE connection);

// Login and get session token
bool ps14_auth_login(HANDLE connection, const char* username, const char* password, char* token_out, u32 token_size);
bool ps14_auth_logout(HANDLE connection);

// Validate session token
bool ps14_auth_validate_token(HANDLE connection, const char* token);
```

### Token Management

```c
// Generate a new session token
bool ps14_token_generate(char* token_out, u32 token_size);
bool ps14_token_validate(const char* token);

// Set token expiry
void ps14_token_set_expiry(u64 seconds);
u64 ps14_token_get_expiry();
```

---

## Repair System

### Memory Repair

```c
// Restore memory from backup
bool ps14_repair_memory_restore(void* address, usize size, const void* backup);

// Create memory backup
bool ps14_repair_memory_backup(void* address, usize size, void* backup);

// Rollback memory to last known good state
bool ps14_repair_memory_rollback(void* address, usize size);
```

### File Restoration

```c
// Restore file from backup
bool ps14_repair_file_restore(const char* path);

// Create file backup
bool ps14_repair_file_backup(const char* path);

// Verify file can be restored
bool ps14_repair_file_verify_backup(const char* path);
```

---

## Advanced Detection

### Debugger Detection

```c
// Check if debugger is attached
bool ps14_debugger_is_present();

// Advanced debugger detection
typedef enum {
    PS14_DEBUGGER_BASIC,
    PS14_DEBUGGER_NTQUERY,
    PS14_DEBUGGER_PARENT,
    PS14_DEBUGGER_TIMING,
    PS14_DEBUGGER_ALL
} Ps14DebuggerCheck;

bool ps14_debugger_check(Ps14DebuggerCheck check);
```

### Hook Detection

```c
// Check for function hooks
bool ps14_hook_detect(void* function_address);
bool ps14_hook_detect_range(void* start, void* end);

// Check for inline hooks
bool ps14_hook_detect_inline(void* function_address);
```

### Tamper Detection

```c
// Detect code tampering
bool ps14_tamper_detect_code(void* address, usize size);

// Detect NOP sleds
bool ps14_tamper_detect_nop_sleds(void* address, usize size);

// Detect unexpected jumps
bool ps14_tamper_detect_jumps(void* address, usize size);
```

### Buffer Overflow Protection

```c
// Enable stack canaries
void ps14_overflow_enable_stack_canaries(bool enabled);

// Enable heap protection
void ps14_overflow_enable_heap_protection(bool enabled);

// Register buffer for overflow checking
bool ps14_overflow_register_buffer(void* buffer, usize size);

// Check buffer for overflow
bool ps14_overflow_check_buffer(void* buffer);
```

---

## Kernel Driver

### Driver Communication

```c
// Open kernel device
HANDLE ps14_kernel_open();
void ps14_kernel_close(HANDLE hDevice);

// Send IO control
bool ps14_kernel_io_control(HANDLE hDevice, DWORD ioctl_code, void* in_buffer, DWORD in_size, void* out_buffer, DWORD out_size, DWORD* bytes_returned);
```

### Memory Protection (Kernel)

```c
// Protect memory from kernel
typedef enum {
    PS14_MEM_PROTECT_READ = 0x01,
    PS14_MEM_PROTECT_WRITE = 0x02,
    PS14_MEM_PROTECT_EXECUTE = 0x04,
    PS14_MEM_PROTECT_GUARD = 0x08,
    PS14_MEM_PROTECT_NOACCESS = 0x10
} Ps14MemoryProtectionFlags;

Ps14MemoryProtectResult ps14_kernel_protect_memory(HANDLE hDevice, void* address, usize size, Ps14MemoryProtectionFlags flags);
Ps14MemoryProtectResult ps14_kernel_unprotect_memory(HANDLE hDevice, void* address, usize size);
```

### Process Monitoring (Kernel)

```c
// Monitor process
i32 ps14_kernel_monitor_process(HANDLE hDevice, u32 process_id, bool enable);
i32 ps14_kernel_stop_monitoring(HANDLE hDevice);

// Get process info
i32 ps14_kernel_get_process_info(HANDLE hDevice, u32 process_id, Ps14ProcessInfo* info);
```

### Shadow Page Table Detection

```c
// Check for shadow page tables
Ps14ShadowPTResult ps14_kernel_check_shadow_pt(HANDLE hDevice, Ps14ShadowPTInfo* info);

// Enable/disable detection
i32 ps14_kernel_enable_shadow_pt_detection(HANDLE hDevice, bool enable);
```

---

## Client Launcher

### Configuration

```c
// Load configuration
bool ps14_launcher_init();
void ps14_launcher_shutdown();

// Configuration management
bool ps14_config_manager_load(const char* path);
bool ps14_config_manager_save(const char* path);

// Set protection features
bool ps14_config_manager_set_memory_protection(bool enabled);
bool ps14_config_manager_set_integrity_check(bool enabled);
bool ps14_config_manager_set_debugger_detection(bool enabled);
```

### Network

```c
// Connect to server
bool ps14_network_client_connect(const char* server_ip, u16 port);
void ps14_network_client_disconnect();
bool ps14_network_client_is_connected();

// Send/receive data
bool ps14_network_client_send_auth_token(const char* token, u32 token_len);
bool ps14_network_client_send_state_update(void* state_data, u32 data_size);
bool ps14_network_client_receive(void* buffer, u32 buffer_size, u32* bytes_received);
```

---

## Server Components

### Authentication Server

```c
bool ps14_auth_server_init();
void ps14_auth_server_shutdown();

bool ps14_auth_server_register_user(const char* username, const char* password);
bool ps14_auth_server_login(const char* username, const char* password, char* token_out);
bool ps14_auth_server_validate_token(const char* token);
