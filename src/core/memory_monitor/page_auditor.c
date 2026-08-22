#include "ps14/memory.h"
#include "ps14/logger.h"
#include "ps14/config.h"

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

// Get memory page information for a process
i32 ps14_page_audit_process(void* process_handle, Ps14MemoryScanResult* results, usize max_results) {
    if (!process_handle) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    u32 tampered_count = 0;
    
    #ifdef PS14_PLATFORM_WINDOWS
    
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    usize page_size = sys_info.dwPageSize;
    
    MEMORY_BASIC_INFORMATION mbi;
    u8* addr = NULL;
    
    while (tampered_count < max_results) {
        if (VirtualQueryEx(process_handle, addr, &mbi, sizeof(mbi)) == 0) {
            break;
        }
        
        // Check if this page is readable and writable
        if (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE) {
            // Check if this page is in a protected region
            // For now, just log it
            PS14_LOG_DEBUG("Writable page found at %p (size: %zu, protect: 0x%X)", 
                          mbi.BaseAddress, mbi.RegionSize, mbi.Protect);
        }
        
        // Check for execute permissions on data pages
        if (mbi.Protect == PAGE_EXECUTE || mbi.Protect == PAGE_EXECUTE_READ || 
            mbi.Protect == PAGE_EXECUTE_READWRITE || mbi.Protect == PAGE_EXECUTE_WRITECOPY) {
            // This could be code injection
            PS14_LOG_WARNING("Executable page found at %p (size: %zu)", 
                           mbi.BaseAddress, mbi.RegionSize);
        }
        
        addr = (u8*)mbi.BaseAddress + mbi.RegionSize;
    }
    
    #endif
    
    return tampered_count;
}

// Check if a specific page is protected
i32 ps14_page_is_protected(void* address) {
    #ifdef PS14_PLATFORM_WINDOWS
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0) {
        return PS14_ERROR_UNKNOWN;
    }
    
    // Check if page has write protection
    if (mbi.Protect == PAGE_READONLY || mbi.Protect == PAGE_EXECUTE_READ) {
        return PS14_SUCCESS;
    }
    
    return PS14_ERROR_UNKNOWN;
    #else
    return PS14_ERROR_NOT_INITIALIZED; // Not implemented for Linux yet
    #endif
}

// Set page protection
i32 ps14_page_set_protection(void* address, usize size, u32 protection) {
    #ifdef PS14_PLATFORM_WINDOWS
    DWORD old_protect;
    if (VirtualProtect(address, size, protection, &old_protect)) {
        return PS14_SUCCESS;
    }
    return PS14_ERROR_UNKNOWN;
    #else
    // Linux implementation would use mprotect
    return PS14_ERROR_NOT_INITIALIZED;
    #endif
}

// Get page protection flags
i32 ps14_page_get_protection(void* address, u32* protection) {
    #ifdef PS14_PLATFORM_WINDOWS
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0) {
        return PS14_ERROR_UNKNOWN;
    }
    *protection = mbi.Protect;
    return PS14_SUCCESS;
    #else
    return PS14_ERROR_NOT_INITIALIZED;
    #endif
}

// Audit all pages in a region
i32 ps14_page_audit_region(Ps14MemoryRegion* region, Ps14MemoryScanResult* results, usize max_results) {
    if (!region) {
        return PS14_ERROR_INVALID_ARGUMENT;
    }
    
    #ifdef PS14_PLATFORM_WINDOWS
    
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    usize page_size = sys_info.dwPageSize;
    
    u8* start = (u8*)region->base_address;
    u8* end = start + region->size;
    u8* addr = start;
    
    u32 tampered_count = 0;
    
    while (addr < end && tampered_count < max_results) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) {
            break;
        }
        
        // Check if this page intersects with our region
        u8* page_end = (u8*)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.BaseAddress <= addr && page_end > addr) {
            // Check page protection
            if (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE) {
                results[tampered_count].region = region;
                results[tampered_count].tampered = true;
                results[tampered_count].tamper_type = PS14_TAMPER_PROTECTION_REMOVED;
                snprintf(results[tampered_count].description, PS14_MAX_MESSAGE,
                        "Page at %p is writable", mbi.BaseAddress);
                tampered_count++;
            }
        }
        
        addr = (u8*)mbi.BaseAddress + mbi.RegionSize;
    }
    
    return tampered_count;
    #else
    return 0; // Not implemented for Linux yet
    #endif
}
