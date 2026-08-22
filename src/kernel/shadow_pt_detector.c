#include "ps14/kernel.h"

#ifdef PS14_MODE_KERNEL
#include <ntifs.h>
#include <intrin.h>
#endif

static CR3_REGISTER g_expected_cr3 = 0;
static bool g_detection_active = false;

VOID ps14_shadow_pt_detector_init() {
#ifdef PS14_MODE_KERNEL
    g_expected_cr3 = __readcr3();
    g_detection_active = true;
#else
    g_detection_active = true;
#endif
}

VOID ps14_shadow_pt_detector_shutdown() {
    g_detection_active = false;
}

Ps14ShadowPTResult ps14_shadow_pt_detector_check(void* input, ULONG is, Ps14ShadowPTInfo* out) {
#ifdef PS14_MODE_KERNEL
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is);
    Ps14ShadowPTInfo info = {0};
    
    if (!g_detection_active || !out) {
        return PS14_SHADOW_PT_NONE;
    }
    
    // Method 1: CR3 register check
    CR3_REGISTER current_cr3 = __readcr3();
    if (current_cr3 != g_expected_cr3) {
        info.shadow_pt_active = true;
        info.detection_confidence = 95;
        info.shadow_pt_address = (void*)(current_cr3 & ~0xFFF);
    }
    
    // Method 2: CPUID VMX check
    int cpuid_info[4];
    __cpuid(cpuid_info, 1);
    if (cpuid_info[2] & (1 << 5)) { // VMX bit
        // Check if we're in a VM
        __cpuid(cpuid_info, 0x480); // VMX leaf
        if (cpuid_info[0] != 0) {
            info.shadow_pt_active = true;
            info.detection_confidence = 85;
        }
    }
    
    // Method 3: Timing check (simplified)
    // Shadow page tables cause memory access to be slower
    // This is a placeholder for actual timing measurement
    
    if (info.shadow_pt_active) {
        *out = info;
        return PS14_SHADOW_PT_DETECTED;
    }
    
    return PS14_SHADOW_PT_NONE;
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(is); UNREFERENCED_PARAMETER(out);
    return PS14_SHADOW_PT_NONE;
#endif
}

NTSTATUS ps14_shadow_pt_detector_enable(void* input, ULONG size) {
#ifdef PS14_MODE_KERNEL
    if (size < sizeof(bool)) return STATUS_INVALID_PARAMETER;
    g_detection_active = *(bool*)input;
    if (g_detection_active) {
        g_expected_cr3 = __readcr3();
    }
#else
    UNREFERENCED_PARAMETER(input); UNREFERENCED_PARAMETER(size);
    g_detection_active = true;
#endif
    return 0;
}
