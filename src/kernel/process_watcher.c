#include "ps14/kernel.h"

#ifdef PS14_MODE_KERNEL
#include <ntifs.h>
#endif

#define MAX_EVENT_LOG 1024

static PVOID g_process_callback = NULL;
static PVOID g_thread_callback = NULL;
static PVOID g_image_callback = NULL;
static Ps14ProcessEvent g_event_log[MAX_EVENT_LOG];
static ULONG g_event_count = 0;
static KSPIN_LOCK g_event_lock;
static bool g_monitoring_enabled = false;

#ifdef PS14_MODE_KERNEL
static VOID ps14_process_notify_routine(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
    if (!g_monitoring_enabled) return;
    Ps14ProcessEvent ev = {0};
    if (CreateInfo) {
        ev.type = PS14_PROCESS_EVENT_CREATE;
        ev.process_id = (u32)(ULONG_PTR)ProcessId;
        if (CreateInfo->ImageFileName) {
            size_t len = CreateInfo->ImageFileName->Length / sizeof(WCHAR);
            len = len < PS14_MAX_PATH-1 ? len : PS14_MAX_PATH-1;
            for (size_t i = 0; i < len; i++) {
                ev.module_name[i] = (char)CreateInfo->ImageFileName->Buffer[i];
            }
        }
    } else {
        ev.type = PS14_PROCESS_EVENT_TERMINATE;
        ev.process_id = (u32)(ULONG_PTR)ProcessId;
    }
    KIRQL irql; KeAcquireSpinLock(&g_event_lock, &irql);
    if (g_event_count < MAX_EVENT_LOG) {
        g_event_log[g_event_count++] = ev;
    }
    KeReleaseSpinLock(&g_event_lock, irql);
}

static VOID ps14_thread_notify_routine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
    if (!g_monitoring_enabled) return;
    Ps14ProcessEvent ev = {0};
    ev.type = Create ? PS14_PROCESS_EVENT_THREAD_CREATE : PS14_PROCESS_EVENT_THREAD_TERMINATE;
    ev.process_id = (u32)(ULONG_PTR)ProcessId;
    ev.thread_id = (u32)(ULONG_PTR)ThreadId;
    KIRQL irql; KeAcquireSpinLock(&g_event_lock, &irql);
    if (g_event_count < MAX_EVENT_LOG) {
        g_event_log[g_event_count++] = ev;
    }
    KeReleaseSpinLock(&g_event_lock, irql);
}

static VOID ps14_image_notify_routine(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo) {
    if (!g_monitoring_enabled || !ImageInfo) return;
    Ps14ProcessEvent ev = {0};
    ev.type = ImageInfo->SystemModeImage ? PS14_PROCESS_EVENT_DLL_UNLOAD : PS14_PROCESS_EVENT_DLL_LOAD;
    ev.process_id = (u32)(ULONG_PTR)ProcessId;
    if (FullImageName && FullImageName->Buffer) {
        size_t len = FullImageName->Length / sizeof(WCHAR);
        len = len < PS14_MAX_PATH-1 ? len : PS14_MAX_PATH-1;
        for (size_t i = 0; i < len; i++) {
            ev.module_name[i] = (char)FullImageName->Buffer[i];
        }
    }
    KIRQL irql; KeAcquireSpinLock(&g_event_lock, &irql);
    if (g_event_count < MAX_EVENT_LOG) {
        g_event_log[g_event_count++] = ev;
    }
    KeReleaseSpinLock(&g_event_lock, irql);
}
#endif

NTSTATUS ps14_process_watcher_init() {
#ifdef PS14_MODE_KERNEL
    KeInitializeSpinLock(&g_event_lock);
    g_event_count = 0;
    g_monitoring_enabled = false;
    g_process_callback = PsSetCreateProcessNotifyRoutine(ps14_process_notify_routine, FALSE);
    g_thread_callback = PsSetCreateThreadNotifyRoutine(ps14_thread_notify_routine);
    g_image_callback = PsSetLoadImageNotifyRoutine(ps14_image_notify_routine);
    g_monitoring_enabled = true;
#else
    g_monitoring_enabled = true;
#endif
    return 0;
}

NTSTATUS ps14_process_watcher_shutdown() {
#ifdef PS14_MODE_KERNEL
    g_monitoring_enabled = false;
    if (g_process_callback) PsSetCreateProcessNotifyRoutine(ps14_process_notify_routine, TRUE);
    if (g_thread_callback) PsRemoveCreateThreadNotifyRoutine(ps14_thread_notify_routine);
    if (g_image_callback) PsRemoveLoadImageNotifyRoutine(ps14_image_notify_routine);
    g_process_callback = g_thread_callback = g_image_callback = NULL;
    g_event_count = 0;
#else
    g_monitoring_enabled = false;
#endif
    return 0;
}

NTSTATUS ps14_process_watcher_monitor(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    if (size < sizeof(u32)) return STATUS_INVALID_PARAMETER;
    u32* pid = (u32*)input;
    g_monitoring_enabled = (*pid != 0);
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(size);
#endif
    return 0;
}

NTSTATUS ps14_process_watcher_stop() {
#ifdef PS14_MODE_KERNEL
    g_monitoring_enabled = false;
#else
    g_monitoring_enabled = false;
#endif
    return 0;
}

NTSTATUS ps14_process_watcher_get_info(void* input, ULONG is, Ps14ProcessInfo* out, ULONG oc) {
#ifdef PS14_MODE_KERNEL
    if (!out || oc == 0 || !input || is < sizeof(u32)) return STATUS_INVALID_PARAMETER;
    u32 pid = *(u32*)input;
    PEPROCESS p; NTSTATUS s = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &p);
    if (!NT_SUCCESS(s)) return s;
    out->process_id = pid;
    out->parent_process_id = (u32)(ULONG_PTR)PsGetProcessParentProcessId(p);
    PsGetProcessImageFileName(p, (PUNICODE_STRING)out->process_name);
    ObDereferenceObject(p);
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is); UNREFERENCED_PARAMETER(out); UNREFERENCED_PARAMETER(oc);
#endif
    return 0;
}

NTSTATUS ps14_process_watcher_get_events(void* input, ULONG is, Ps14ProcessEvent* out, ULONG oc) {
#ifdef PS14_MODE_KERNEL
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is);
    if (!out || oc == 0) return STATUS_INVALID_PARAMETER;
    KIRQL irql; KeAcquireSpinLock(&g_event_lock, &irql);
    ULONG c = oc < g_event_count ? oc : g_event_count;
    for (ULONG i = 0; i < c; i++) out[i] = g_event_log[i];
    g_event_count = 0;
    KeReleaseSpinLock(&g_event_lock, irql);
    return c;
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is); UNREFERENCED_PARAMETER(out); UNREFERENCED_PARAMETER(oc);
#endif
    return 0;
}
