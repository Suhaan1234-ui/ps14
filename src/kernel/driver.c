#include "ps14/kernel.h"

// For kernel mode, we need Windows Driver Kit headers
#ifdef PS14_MODE_KERNEL
#include <ntddk.h>
#include <ntifs.h>
#include <wdm.h>
#else
// For user-mode compilation (stubs)
#include <windows.h>
#endif

// Driver object
#ifdef PS14_MODE_KERNEL
static PDRIVER_OBJECT g_driver_object = NULL;
static PDEVICE_OBJECT g_device_object = NULL;
static UNICODE_STRING g_device_name;
static UNICODE_STRING g_dos_device_name;
#else
// User-mode stubs
typedef struct _DRIVER_OBJECT DRIVER_OBJECT;
typedef struct _DEVICE_OBJECT DEVICE_OBJECT;
#endif

// Driver state
static bool g_driver_initialized = false;
static bool g_memory_protection_enabled = false;
static bool g_process_monitoring_enabled = false;
static bool g_callbacks_enabled = false;
static bool g_shadow_pt_detection_enabled = false;

// ============================================================================
// DRIVER ENTRY POINT (Kernel Mode Only)
// ============================================================================

#ifdef PS14_MODE_KERNEL

// Driver entry point - called when driver is loaded
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    
    NTSTATUS status;
    
    // Initialize device name
    RtlInitUnicodeString(&g_device_name, L"\Device\ps14_protection");
    RtlInitUnicodeString(&g_dos_device_name, PS14_KERNEL_DEVICE_NAME);
    
    // Create device object
    status = IoCreateDevice(
        DriverObject,
        0,
        &g_device_name,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_device_object
    );
    
    if (!NT_SUCCESS(status)) {
        KdPrint(("ps14: Failed to create device object (0x%X)\n", status));
        return status;
    }
    
    // Create symbolic link so user mode can access the device
    status = IoCreateSymbolicLink(&g_dos_device_name, &g_device_name);
    if (!NT_SUCCESS(status)) {
        KdPrint(("ps14: Failed to create symbolic link (0x%X)\n", status));
        IoDeleteDevice(g_device_object);
        return status;
    }
    
    // Set up dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = ps14_driver_create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ps14_driver_close;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ps14_driver_ioctl;
    DriverObject->DriverUnload = ps14_driver_unload;
    
    // Store driver object
    g_driver_object = DriverObject;
    
    // Initialize subsystems
    ps14_memory_guard_init();
    ps14_process_watcher_init();
    ps14_callback_handler_init();
    ps14_shadow_pt_detector_init();
    
    g_driver_initialized = true;
    
    KdPrint(("ps14: Driver loaded successfully\n"));
    return STATUS_SUCCESS;
}

// Driver unload routine
VOID ps14_driver_unload(PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    
    // Shutdown subsystems
    ps14_memory_guard_shutdown();
    ps14_process_watcher_shutdown();
    ps14_callback_handler_shutdown();
    ps14_shadow_pt_detector_shutdown();
    
    // Delete symbolic link
    if (g_dos_device_name.Buffer) {
        IoDeleteSymbolicLink(&g_dos_device_name);
    }
    
    // Delete device object
    if (g_device_object) {
        IoDeleteDevice(g_device_object);
    }
    
    g_driver_initialized = false;
    KdPrint(("ps14: Driver unloaded successfully\n"));
}

// Create dispatch
NTSTATUS ps14_driver_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Close dispatch
NTSTATUS ps14_driver_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// IOCTL dispatch - handles all communication from user mode
NTSTATUS ps14_driver_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    
    switch (ioStack->Parameters.DeviceIoControl.IoControlCode) {
        // Memory protection
        case PS14_IOCTL_PROTECT_MEMORY:
            status = ps14_memory_guard_protect(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength
            );
            break;
            
        case PS14_IOCTL_UNPROTECT_MEMORY:
            status = ps14_memory_guard_unprotect(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength
            );
            break;
            
        case PS14_IOCTL_SCAN_MEMORY:
            status = ps14_memory_guard_scan(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength,
                (Ps14KernelMemoryRegion*)Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.OutputBufferLength / sizeof(Ps14KernelMemoryRegion)
            );
            break;
            
        // Process monitoring
        case PS14_IOCTL_MONITOR_PROCESS:
            status = ps14_process_watcher_monitor(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength
            );
            break;
            
        case PS14_IOCTL_STOP_MONITORING:
            status = ps14_process_watcher_stop();
            break;
            
        case PS14_IOCTL_GET_PROCESS_INFO:
            status = ps14_process_watcher_get_info(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength,
                (Ps14ProcessInfo*)Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.OutputBufferLength / sizeof(Ps14ProcessInfo)
            );
            break;
            
        // Callback handling
        case PS14_IOCTL_REGISTER_CALLBACK:
            status = ps14_callback_handler_register(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength
            );
            break;
            
        case PS14_IOCTL_UNREGISTER_CALLBACK:
            status = ps14_callback_handler_unregister(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength
            );
            break;
            
        // Shadow page table detection
        case PS14_IOCTL_CHECK_SHADOW_PT:
            status = ps14_shadow_pt_detector_check(
                Irp->AssociatedIrp.SystemBuffer,
                ioStack->Parameters.DeviceIoControl.InputBufferLength,
                (Ps14ShadowPTInfo*)Irp->AssociatedIrp.SystemBuffer
            );
            break;
            
        default:
            KdPrint(("ps14: Unknown IOCTL code: 0x%X\n", ioStack->Parameters.DeviceIoControl.IoControlCode));
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

#else // PS14_MODE_KERNEL

// User-mode stubs for when not compiling as kernel driver

HANDLE ps14_kernel_open(void) {
    return INVALID_HANDLE_VALUE;
}

void ps14_kernel_close(HANDLE hDevice) {
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
}

bool ps14_kernel_io_control(HANDLE hDevice, DWORD dwIoControlCode,
                             void* lpInBuffer, DWORD nInBufferSize,
                             void* lpOutBuffer, DWORD nOutBufferSize,
                             DWORD* lpBytesReturned) {
    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    return DeviceIoControl(
        hDevice,
        dwIoControlCode,
        lpInBuffer, nInBufferSize,
  
