#include "ps14/kernel.h"

#ifdef PS14_MODE_KERNEL
#include <ntifs.h>
#endif

static Ps14KernelCallback* g_callbacks = NULL;
static KSPIN_LOCK g_callback_lock;

NTSTATUS ps14_callback_handler_init() {
#ifdef PS14_MODE_KERNEL
    KeInitializeSpinLock(&g_callback_lock);
#else
    // User-mode: nothing to init
#endif
    return 0;
}

NTSTATUS ps14_callback_handler_shutdown() {
#ifdef PS14_MODE_KERNEL
    KIRQL irql;
    KeAcquireSpinLock(&g_callback_lock, &irql);
    while (g_callbacks) {
        Ps14KernelCallback* next = g_callbacks->next;
        ExFreePoolWithTag(g_callbacks, 'ps14');
        g_callbacks = next;
    }
    KeReleaseSpinLock(&g_callback_lock, irql);
#else
    // User-mode: nothing to shutdown
#endif
    return 0;
}

NTSTATUS ps14_callback_handler_register(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    if (!input || size < sizeof(Ps14KernelCallback)) return STATUS_INVALID_PARAMETER;
    
    Ps14KernelCallback* new_cb = (Ps14KernelCallback*)input;
    
    if (!MmIsAddressValid(new_cb->callback_address)) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Check if already registered
    KIRQL irql;
    KeAcquireSpinLock(&g_callback_lock, &irql);
    for (Ps14KernelCallback* cb = g_callbacks; cb; cb = cb->next) {
        if (cb->callback_address == new_cb->callback_address && cb->type == new_cb->type) {
            KeReleaseSpinLock(&g_callback_lock, irql);
            return STATUS_ALREADY_REGISTERED;
        }
    }
    
    // Allocate and add
    Ps14KernelCallback* stored_cb = ExAllocatePoolWithTag(NonPagedPool, sizeof(Ps14KernelCallback), 'ps14');
    if (!stored_cb) {
        KeReleaseSpinLock(&g_callback_lock, irql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *stored_cb = *new_cb;
    stored_cb->next = g_callbacks;
    g_callbacks = stored_cb;
    KeReleaseSpinLock(&g_callback_lock, irql);
    return STATUS_SUCCESS;
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(size);
#endif
    return 0;
}

NTSTATUS ps14_callback_handler_unregister(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    if (!input || size < sizeof(void*)) return STATUS_INVALID_PARAMETER;
    void* addr = *(void**)input;
    
    KIRQL irql;
    KeAcquireSpinLock(&g_callback_lock, &irql);
    Ps14KernelCallback* prev = NULL;
    for (Ps14KernelCallback* cb = g_callbacks; cb; cb = cb->next) {
        if (cb->callback_address == addr) {
            if (prev) prev->next = cb->next;
            else g_callbacks = cb->next;
            ExFreePoolWithTag(cb, 'ps14');
            break;
        }
        prev = cb;
    }
    KeReleaseSpinLock(&g_callback_lock, irql);
    return STATUS_SUCCESS;
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(size);
#endif
    return 0;
}

// Invoke all callbacks of a specific type
VOID ps14_callback_invoke(Ps14CallbackType type, void* data) {
#ifdef PS14_MODE_KERNEL
    KIRQL irql;
    KeAcquireSpinLock(&g_callback_lock, &irql);
    for (Ps14KernelCallback* cb = g_callbacks; cb; cb = cb->next) {
        if (cb->type == type) {
            // In kernel, we cannot directly call user-mode addresses
            // Queue event for user-mode retrieval
            UNREFERENCED_PARAMETER(data);
        }
    }
    KeReleaseSpinLock(&g_callback_lock, irql);
#endif
}
