#include "ps14/advanced.h"
#include "ps14/logger.h"
#include "ps14/thread.h"

#ifdef PS14_PLATFORM_WINDOWS
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(__readfsdword)
#endif

// Global state
static bool g_debugger_initialized = false;
static Ps14DetectionCallback g_callback = NULL;
static void* g_callback_user_data = NULL;

// Debugger detection methods
static bool check_is_debugger_present(void) {
#ifdef PS14_PLATFORM_WINDOWS
    return IsDebuggerPresent() != 0;
#else
    // Linux: check /proc/self/status for TracerPid
    FILE* f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "TracerPid:", 10) == 0) {
                int pid = atoi(line + 10);
                fclose(f);
                return pid != 0;
            }
        }
        fclose(f);
    }
    return false;
#endif
}

static bool check_nt_query_information_process(void) {
#ifdef PS14_PLATFORM_WINDOWS
    typedef NTSTATUS (NTAPI *pNtQueryInformationProcess)(
        HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG
    );
    
    pNtQueryInformationProcess NtQueryInformationProcess = 
        (pNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
    
    if (NtQueryInformationProcess) {
        PROCESS_BASIC_INFORMATION pbi;
        ULONG len;
        NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &len
        );
        
        if (status == 0 && pbi.PebBaseAddress) {
            // Check PEB->BeingDebugged flag
            if (*(BYTE*)((BYTE*)pbi.PebBaseAddress + 2) & 1) {
                return true;
            }
        }
    }
#endif
    return false;
}

static bool check_parent_process(void) {
#ifdef PS14_PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    DWORD currentPid = GetCurrentProcessId();
    DWORD parentPid = 0;
    
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == currentPid) {
                parentPid = pe32.th32ParentProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
    // Check if parent is a known debugger
    if (parentPid != 0) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, parentPid);
        if (hProcess) {
            char processName[MAX_PATH];
            if (GetModuleBaseNameA(hProcess, NULL, processName, MAX_PATH)) {
                char* lowerName = _strlwr(processName);
                if (strstr(lowerName, "cheatengine") ||
                    strstr(lowerName, "x64dbg") ||
                    strstr(lowerName, "x32dbg") ||
                    strstr(lowerName, "ida") ||
                    strstr(lowerName, "ollydbg") ||
                    strstr(lowerName, "debug") ||
                    strstr(lowerName, "devenv") ||
                    strstr(lowerName, "visualstudio")) {
                    CloseHandle(hProcess);
                    return true;
                }
            }
            CloseHandle(hProcess);
        }
    }
#endif
    return false;
}

static bool check_int3_breakpoints(void) {
#ifdef PS14_PLATFORM_WINDOWS
    // INT3 breakpoint is 0xCC
    // We can't reliably detect this without reading our own code
    // This would require more complex self-inspection
    return false;
#endif
    return false;
}

static bool check_hardware_breakpoints(void) {
#ifdef PS14_PLATFORM_WINDOWS
    CONTEXT context = {0};
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    
    __asm__ volatile ("mov %%eax, %0" : "=r" (context.Eax));
    
    // Check DR0-DR3 for breakpoints
    if (context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3) {
        return true;
    }
#endif
    return false;
}

static bool check_timing(void) {
    // Debuggers slow down execution
    // This is a simple timing check
    
    u64 start = ps14_thread_get_tick_count();
    
    // Perform some operations
    volatile u64 x = 0;
    for (volatile u32 i = 0; i < 100000; i++) {
        x += i;
    }
    
    u64 end = ps14_thread_get_tick_count();
    u64 elapsed = end - start;
    
    // If it took too long, a debugger might be attached
    // Threshold: 100ms for 100k iterations (adjust as needed)
    if (elapsed > 100) {
        return true;
    }
    
    return false;
}

static bool check_window_title(void) {
#ifdef PS14_PLATFORM_WINDOWS
    HWND hWnd = GetForegroundWindow();
    if (hWnd) {
        char title[256];
        if (GetWindowTextA(hWnd, title, sizeof(title)) > 0) {
            char* lowerTitle = _strlwr(title);
            if (strstr(lowerTitle, "cheat engine") ||
                strstr(lowerTitle, "x64dbg") ||
                strstr(lowerTitle, "ida pro") ||
                strstr(lowerTitle, "ollydbg")) {
                return true;
            }
        }
    }
#endif
    return false;
}

static bool check_known_debugger_modules(void) {
#ifdef PS14_PLATFORM_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    
    if (Module32First(hSnapshot, &me32)) {
        do {
            char* lowerName = _strlwr(me32.szModule);
            if (strstr(lowerName, "cheatengine") ||
                strstr(lowerName, "dbghelp") ||
                strstr(lowerName, "kernel32") || // Some debuggers inject this
                strstr(lowerName, "user32")) { // Some debuggers inject this
                // Note: kernel32 and user32 are normal, but some debuggers
                // inject additional instances
                // For simplicity, we'll just check for known debugger modules
                if (strstr(lowerName, "cheatengine")) {
                    CloseHandle(hSnapshot);
                    return true;
                }
            }
        } while (Module32Next(hSnapshot, &me32));
    }
    
    CloseHandle(hSnapshot);
#endif
    return false;
}

// Public API implementations
PS14_API i32 ps14_debugger_init(void) {
    if (g_debugger_initialized) {
        return PS14_ERROR_ALREADY_INITIALIZED;
    }
    
    g_debugger_initialized = true;
    PS14_LOG_INFO("Debugger detector initialized");
    return PS14_SUCCESS;
}

PS14_API void ps14_debugger_shutdown(void) {
    g_debugger_initialized = false;
    PS14_LOG_INFO("Debugger detector shutdown");
}

PS14_API bool ps14_debugger_is_attached(void) {
    // Run all detection methods
    if (check_is_debugger_present()) {
        PS14_LOG_WARNING("Debugger detected: IsDebuggerPresent");
        return true;
    }
    
    if (check_nt_query_information_process()) {
        PS14_LOG_WARNING("Debugger detected: NtQueryInformationProcess");
        return true;
    }
    
    if (check_parent_process()) {
        PS14_LOG_WARNING("Debugger detected: Parent process check");
        return true;
    }
    
    if (check_int3_breakpoints()) {
        PS14_LOG_WARNING("Debugger detected: INT3 breakpoints");
        return true;
    }
    
    if (check_hardware_breakpoints()) {
        PS14_LOG_WARNING("Debugger detected: Hardware breakpoints");
        return true;
    }
    
    if (check_timing()) {
        PS14_LOG_WARNING("Debugger detected: Timing check");
        return true;
    }
    
    if (check_window_title()) {
        PS14_LOG_WARNING("Debugger detected: Window title check");
        return true;
    }
    
    if (check_known_debugger_modules()) {
        PS14_LOG_WARNING("Debugger detected: Known debugger module");
        return true;
    }
    
    return false;
}

PS14_API bool ps14_debugger_check_cheat_engine(void) {
    return check_parent_process(); // Checks for Cheat Engine parent
}

PS14_API bool ps14_debugger
