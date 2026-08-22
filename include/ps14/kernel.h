#ifndef PS14_KERNEL_H
#define PS14_KERNEL_H

#pragma once

#include "ps14.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// KERNEL DRIVER IO CONTROL CODES
// ============================================================================

// IOCTL base value
#define PS14_KERNEL_IOCTL_BASE 0x22000

// Memory protection IOCTLs
#define PS14_IOCTL_PROTECT_MEMORY    CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0001, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PS14_IOCTL_UNPROTECT_MEMORY  CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PS14_IOCTL_SCAN_MEMORY       CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0003, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Process monitoring IOCTLs
#define PS14_IOCTL_MONITOR_PROCESS  CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PS14_IOCTL_STOP_MONITORING  CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0011, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PS14_IOCTL_GET_PROCESS_INFO CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0012, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Callback IOCTLs
#define PS14_IOCTL_REGISTER_CALLBACK CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0020, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PS14_IOCTL_UNREGISTER_CALLBACK CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0021, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Shadow page table IOCTLs
#define PS14_IOCTL_CHECK_SHADOW_PT  CTL_CODE(PS14_KERNEL_IOCTL_BASE, 0x0030, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ============================================================================
// MEMORY PROTECTION TYPES
// ============================================================================

// Memory protection flags
typedef enum {
    PS14_MEM_PROTECT_READ = 0x01,
    PS14_MEM_PROTECT_WRITE = 0x02,
    PS14_MEM_PROTECT_EXECUTE = 0x04,
    PS14_MEM_PROTECT_GUARD = 0x08,
    PS14_MEM_PROTECT_NOACCESS = 0x10
} Ps14MemoryProtectionFlags;

// Memory region for kernel protection
typedef struct Ps14KernelMemoryRegion {
    void* address;
    usize size;
    Ps14MemoryProtectionFlags protection;
    bool is_protected;
} Ps14KernelMemoryRegion;

// Memory protection result
typedef enum {
    PS14_MEM_PROTECT_SUCCESS = 0,
    PS14_MEM_PROTECT_FAILED,
    PS14_MEM_PROTECT_ALREADY_PROTECTED,
    PS14_MEM_PROTECT_INVALID_ADDRESS
} Ps14MemoryProtectResult;

// ============================================================================
// PROCESS MONITORING TYPES
// ============================================================================

// Process information
typedef struct Ps14ProcessInfo {
    u32 process_id;
    u32 parent_process_id;
    char process_name[PS14_MAX_PATH];
    char full_path[PS14_MAX_PATH];
    bool is_suspended;
    u64 create_time;
} Ps14ProcessInfo;

// Process event types
typedef enum {
    PS14_PROCESS_EVENT_CREATE = 0,
    PS14_PROCESS_EVENT_TERMINATE,
    PS14_PROCESS_EVENT_DLL_LOAD,
    PS14_PROCESS_EVENT_DLL_UNLOAD,
    PS14_PROCESS_EVENT_THREAD_CREATE,
    PS14_PROCESS_EVENT_THREAD_TERMINATE
} Ps14ProcessEventType;

// Process event
typedef struct Ps14ProcessEvent {
    Ps14ProcessEventType type;
    u32 process_id;
    u32 thread_id;
    char module_name[PS14_MAX_PATH];
    u64 timestamp;
} Ps14ProcessEvent;

// ============================================================================
// CALLBACK TYPES
// ============================================================================

// Callback types
typedef enum {
    PS14_CALLBACK_TYPE_MEMORY_ACCESS = 0,
    PS14_CALLBACK_TYPE_PROCESS_EVENT,
    PS14_CALLBACK_TYPE_THREAD_EVENT,
    PS14_CALLBACK_TYPE_MODULE_LOAD
} Ps14CallbackType;

// Callback information
typedef struct Ps14KernelCallback {
    Ps14CallbackType type;
    void* callback_address;
    void* context;
    struct Ps14KernelCallback* next;
} Ps14KernelCallback;

// Callback registration result
typedef enum {
    PS14_CALLBACK_REGISTER_SUCCESS = 0,
    PS14_CALLBACK_REGISTER_FAILED,
    PS14_CALLBACK_ALREADY_REGISTERED,
    PS14_CALLBACK_INVALID_ADDRESS
} Ps14CallbackRegisterResult;

// ============================================================================
// SHADOW PAGE TABLE DETECTION
// ============================================================================

// Shadow page table detection result
typedef enum {
    PS14_SHADOW_PT_NONE = 0,
    PS14_SHADOW_PT_DETECTED,
    PS14_SHADOW_PT_SUSPICIOUS
} Ps14ShadowPTResult;

// Shadow page table information
typedef struct Ps14ShadowPTInfo {
    bool shadow_pt_active;
    void* shadow_pt_address;
    u64 detection_confidence;
} Ps14ShadowPTInfo;

// ============================================================================
// KERNEL DRIVER COMMUNICATION
// ============================================================================

// Device name
#define PS14_KERNEL_DEVICE_NAME L"\.\ps14_protection"

// Open kernel device
PS14_API HANDLE ps14_kernel_open(void);

// Close kernel device
PS14_API void ps14_kernel_close(HANDLE hDevice);

// Send IO control
PS14_API bool ps14_kernel_io_control(HANDLE hDevice, DWORD dwIoControlCode, 
                                     void* lpInBuffer, DWORD nInBufferSize,
                                     void* lpOutBuffer, DWORD nOutBufferSize,
                                     DWORD* lpBytesReturned);

// ============================================================================
// MEMORY PROTECTION FUNCTIONS
// ============================================================================

// Protect memory region from kernel
PS14_API Ps14MemoryProtectResult ps14_kernel_protect_memory(
    HANDLE hDevice, void* address, usize size, Ps14MemoryProtectionFlags flags);

// Unprotect memory region
PS14_API Ps14MemoryProtectResult ps14_kernel_unprotect_memory(
    HANDLE hDevice, void* address, usize size);

// Scan memory for violations
PS14_API u32 ps14_kernel_scan_memory(HANDLE hDevice, 
                                     void* address, usize size,
                                     Ps14MemoryRegion** violations, u32 max_violations);

// ============================================================================
// PROCESS MONITORING FUNCTIONS
// ============================================================================

// Start monitoring a process
PS14_API i32 ps14_kernel_monitor_process(HANDLE hDevice, u32 process_id, bool enable);

// Stop monitoring all processes
PS14_API i32 ps14_kernel_stop_monitoring(HANDLE hDevice);

// Get process information
PS14_API i32 ps14_kernel_get_process_info(HANDLE hDevice, u32 process_id, Ps14ProcessInfo* info);

// Get process event log
PS14_API u32 ps14_kernel_get_process_events(HANDLE hDevice, 
                                              Ps14ProcessEvent* events, u32 max_events);

// ============================================================================
// CALLBACK FUNCTIONS
// ============================================================================

// Register a callback in kernel
PS14_API Ps14CallbackRegisterResult ps14_kernel_register_callback(
    HANDLE hDevice, Ps14CallbackType type, void* callback_address, void* context);

// Unregister a callback
PS14_API i32 ps14_kernel_unregister_callback(HANDLE hDevice, void* callback_address);

// ============================================================================
// SHADOW PAGE TABLE FUNCTIONS
// ============================================================================

// Check for shadow page tables
PS14_API Ps14ShadowPTResult ps14_kernel_check_shadow_pt(HANDLE hDevice, Ps14ShadowPTInfo* info);

// Enable shadow page table detection
PS14_API i32 ps14_kernel_enable_shadow_pt_detection(HANDLE hDevice, bool enable);

// ============================================================================
// KERNEL DRIVER STATUS
// ============================================================================

// Check if kernel driver is loaded
PS14_API bool ps14_kernel_is_driver_loaded(void);

// Get kernel driver version
PS14_API i32 ps14_kernel_get_version(u32* major, u32* minor, u32* patch);

// Get kernel driver statistics
PS14_API i32 ps14_kernel_get_stats(HANDLE hDevice, 
       
