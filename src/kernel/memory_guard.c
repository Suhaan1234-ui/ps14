#include "ps14/kernel.h"

#ifdef PS14_MODE_KERNEL
#include <ntifs.h>
#endif

typedef struct _PS14_PROTECTED_REGION {
    LIST_ENTRY list_entry;
    Ps14KernelMemoryRegion region;
    ULONG original_protect;
    PEPROCESS target_process;
} PS14_PROTECTED_REGION;

static LIST_ENTRY g_protected_regions_list;
static KSPIN_LOCK g_protected_regions_lock;
static bool g_memory_guard_initialized = false;

#ifdef PS14_MODE_KERNEL
static ULONG ps14_convert_flags(Ps14MemoryProtectionFlags flags) {
    ULONG p = 0;
    if (flags & PS14_MEM_PROTECT_NOACCESS) return PAGE_NOACCESS;
    if (flags & PS14_MEM_PROTECT_READ) p |= PAGE_READONLY;
    if (flags & PS14_MEM_PROTECT_WRITE) p = (p & ~PAGE_READONLY) | PAGE_READWRITE;
    if (flags & PS14_MEM_PROTECT_EXECUTE) {
        if (p & PAGE_READWRITE) p = PAGE_EXECUTE_READWRITE;
        else if (p & PAGE_READONLY) p = PAGE_EXECUTE_READ;
        else p = PAGE_EXECUTE;
    }
    if (flags & PS14_MEM_PROTECT_GUARD) p |= PAGE_GUARD;
    return p != 0 ? p : (PAGE_READONLY | PAGE_GUARD);
}
#endif

NTSTATUS ps14_memory_guard_init() {
#ifdef PS14_MODE_KERNEL
    InitializeListHead(&g_protected_regions_list);
    KeInitializeSpinLock(&g_protected_regions_lock);
    g_memory_guard_initialized = true;
#else
    g_memory_guard_initialized = true;
#endif
    return 0;
}

NTSTATUS ps14_memory_guard_shutdown() {
#ifdef PS14_MODE_KERNEL
    KIRQL irql;
    KeAcquireSpinLock(&g_protected_regions_lock, &irql);
    PLIST_ENTRY e = g_protected_regions_list.Flink;
    while (e != &g_protected_regions_list) {
        PS14_PROTECTED_REGION* r = CONTAINING_RECORD(e, PS14_PROTECTED_REGION, list_entry);
        e = e->Flink;
        SIZE_T s = r->region.size;
        ZwProtectVirtualMemory(r->target_process, &r->region.address, &s, r->original_protect, NULL);
        ObDereferenceObject(r->target_process);
        ExFreePoolWithTag(r, 'ps14');
    }
    KeReleaseSpinLock(&g_protected_regions_lock, irql);
    g_memory_guard_initialized = false;
#else
    g_memory_guard_initialized = false;
#endif
    return 0;
}

NTSTATUS ps14_memory_guard_protect(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    Ps14KernelMemoryRegion* r = (Ps14KernelMemoryRegion*)input;
    if (!g_memory_guard_initialized || !r || size < sizeof(*r) || !MmIsAddressValid(r->address))
        return STATUS_INVALID_PARAMETER;
    PEPROCESS p = PsGetCurrentProcess();
    MEMORY_BASIC_INFORMATION mbi;
    if (!NT_SUCCESS(ZwQueryVirtualMemory(p, r->address, MemoryBasicInformation, &mbi, sizeof(mbi), NULL)))
        return STATUS_ACCESS_VIOLATION;
    ULONG np = ps14_convert_flags(r->protection);
    SIZE_T s = r->size;
    ULONG op;
    if (!NT_SUCCESS(ZwProtectVirtualMemory(p, &r->address, &s, np, &op))) return STATUS_ACCESS_DENIED;
    PS14_PROTECTED_REGION* pr = ExAllocatePoolWithTag(NonPagedPool, sizeof(*pr), 'ps14');
    if (!pr) { ZwProtectVirtualMemory(p, &r->address, &s, op, NULL); return STATUS_NO_MEMORY; }
    pr->region = *r; pr->original_protect = mbi.Protect; pr->target_process = p;
    ObReferenceObject(p);
    KIRQL irql; KeAcquireSpinLock(&g_protected_regions_lock, &irql);
    InsertTailList(&g_protected_regions_list, &pr->list_entry);
    KeReleaseSpinLock(&g_protected_regions_lock, irql);
    r->is_protected = true;
#else
    if (input) ((Ps14KernelMemoryRegion*)input)->is_protected = true;
#endif
    return 0;
}

NTSTATUS ps14_memory_guard_unprotect(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    Ps14KernelMemoryRegion* r = (Ps14KernelMemoryRegion*)input;
    if (!g_memory_guard_initialized || !r || size < sizeof(*r)) return STATUS_INVALID_PARAMETER;
    KIRQL irql; KeAcquireSpinLock(&g_protected_regions_lock, &irql);
    PLIST_ENTRY e;
    for (e = g_protected_regions_list.Flink; e != &g_protected_regions_list; e = e->Flink) {
        PS14_PROTECTED_REGION* pr = CONTAINING_RECORD(e, PS14_PROTECTED_REGION, list_entry);
        if (pr->region.address == r->address) {
            SIZE_T s = pr->region.size; ULONG op;
            ZwProtectVirtualMemory(pr->target_process, &pr->region.address, &s, pr->original_protect, &op);
            RemoveEntryList(&pr->list_entry);
            ObDereferenceObject(pr->target_process);
            ExFreePoolWithTag(pr, 'ps14');
            r->is_protected = false; break;
        }
    }
    KeReleaseSpinLock(&g_protected_regions_lock, irql);
#else
    if (input) ((Ps14KernelMemoryRegion*)input)->is_protected = false;
#endif
    return 0;
}

NTSTATUS ps14_memory_guard_scan(void* input, ULONG is, Ps14KernelMemoryRegion* out, ULONG oc) {
#ifdef PS14_MODE_KERNEL
    if (!g_memory_guard_initialized || !out || oc == 0) return STATUS_INVALID_PARAMETER;
    KIRQL irql; KeAcquireSpinLock(&g_protected_regions_lock, &irql);
    ULONG c = 0; MEMORY_BASIC_INFORMATION mbi;
    PLIST_ENTRY e;
    for (e = g_protected_regions_list.Flink; e != &g_protected_regions_list && c < oc; e = e->Flink) {
        PS14_PROTECTED_REGION* pr = CONTAINING_RECORD(e, PS14_PROTECTED_REGION, list_entry);
        if (NT_SUCCESS(ZwQueryVirtualMemory(pr->target_process, pr->region.address, MemoryBasicInformation, &mbi, sizeof(mbi), NULL)))
            if (mbi.Protect != ps14_convert_flags(pr->region.protection))
                out[c++] = pr->region;
    }
    KeReleaseSpinLock(&g_protected_regions_lock, irql);
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is); UNREFERENCED_PARAMETER(out); UNREFERENCED_PARAMETER(oc);
#endif
    return 0;
}
