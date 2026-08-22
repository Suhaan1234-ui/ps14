cmake_minimum_required(VERSION 3.20)
project(ps14 VERSION 1.0.0 LANGUAGES C CXX)

# Project description
set(PROJECT_DESCRIPTION "Anti-Tamper Game Engine - Kernel-Level Sync & Repair")
set(PROJECT_AUTHOR "Mistral Vibe")
set(PROJECT_LICENSE "MIT")

# Set C standard
set(C_STANDARD 17)
set(C_STANDARD_REQUIRED ON)

# Set C++ standard
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    add_definitions(-D_WINDOWS -DWIN32_LEAN_AND_MEAN)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-D_WIN64)
        set(ARCH_X64 TRUE)
    endif()
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/shared
)

# Link directories
link_directories(${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# SHARED LIBRARY (Common code for all components)
# ============================================================================

add_library(ps14_shared STATIC
    src/shared/config.c
    src/shared/logger.c
    src/shared/hash.c
    src/shared/network.c
    src/shared/thread_pool.c
)

target_include_directories(ps14_shared PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/shared
)

# ============================================================================
# MEMORY MONITOR (User Mode)
# ============================================================================

add_library(ps14_memory_monitor STATIC
    src/core/memory_monitor/memory_scanner.c
    src/core/memory_monitor/page_auditor.c
    src/core/memory_monitor/hash_database.c
)

target_include_directories(ps14_memory_monitor PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/memory_monitor
)

target_link_libraries(ps14_memory_monitor PRIVATE ps14_shared)

# ============================================================================
# INTEGRITY CHECKER (User Mode)
# ============================================================================

add_library(ps14_integrity_check STATIC
    src/core/integrity_check/file_verifier.c
    src/core/integrity_check/code_signature.c
    src/core/integrity_check/checksum_db.c
)

target_include_directories(ps14_integrity_check PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/integrity_check
)

target_link_libraries(ps14_integrity_check PRIVATE ps14_shared)

# ============================================================================
# AUTH GATEWAY (User Mode)
# ============================================================================

add_library(ps14_auth_gateway STATIC
    src/core/auth_gateway/auth_client.c
    src/core/auth_gateway/session_manager.c
    src/core/auth_gateway/token_validator.c
    src/core/auth_gateway/state_sync.c
)

target_include_directories(ps14_auth_gateway PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/auth_gateway
)

target_link_libraries(ps14_auth_gateway PRIVATE ps14_shared)

# ============================================================================
# REPAIR SYSTEM (User Mode)
# ============================================================================

add_library(ps14_repair_system STATIC
    src/core/repair_system/memory_repair.c
    src/core/repair_system/file_restorer.c
    src/core/repair_system/state_roller.c
    src/core/repair_system/backup_manager.c
)

target_include_directories(ps14_repair_system PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/repair_system
)

target_link_libraries(ps14_repair_system PRIVATE ps14_shared)

# ============================================================================
# ADVANCED DETECTION SYSTEMS (Phase 2)
# ============================================================================

add_library(ps14_advanced STATIC
    src/core/advanced/advanced.c
    src/core/advanced/debugger_detector.c
    src/core/advanced/hook_detector.c
    src/core/advanced/tamper_detector.c
    src/core/advanced/async_auditor.c
    src/core/advanced/buffer_overflow.c
)

target_include_directories(ps14_advanced PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core/advanced
)

target_link_libraries(ps14_advanced PRIVATE
    ps14_shared
    ps14_memory_monitor
    ps14_repair_system
)

# ============================================================================
# CORE PROTECTION ENGINE (User Mode DLL)
# ============================================================================

add_library(ps14_core SHARED
    src/core/core.c
)

target_include_directories(ps14_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(ps14_core PRIVATE
    ps14_shared
    ps14_memory_monitor
    ps14_integrity_check
    ps14_auth_gateway
    ps14_repair_system
    ps14_advanced
)

# Windows-specific settings for DLL
if(WIN32)
    set_target_properties(ps14_core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS TRUE
        OUTPUT_NAME "ps14_core"
    )
endif()

# ============================================================================
# CLIENT WRAPPER (User Mode)
# ============================================================================

add_executable(ps14_launcher
    src/client/launcher.c
    src/client/config_manager.c
    src/client/network_client.c
    src/client/api_hooks.c
)

target_include_directories(ps14_launcher PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/client
)

target_link_libraries(ps14_launcher PRIVATE
    ps14_shared
    ps14_core
)

if(WIN32)
    set_target_properties(ps14_launcher PROPERTIES
        OUTPUT_NAME "ps14_launcher"
    )
endif()

# ============================================================================
# AUTH SERVER (User Mode)
# ============================================================================

add_executable(ps14_server
    src/server/auth_server.c
    src/server/session_store.c
    src/server/ban_manager.c
    src/server/state_validator.c
    src/server/network_handler.c
    src/server/main.c
)

target_include_directories(ps14_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/server
)

target_link_libraries(ps14_server PRIVATE
    ps14_shared
)

if(WIN32)
    set_target_properties(ps14_server PROPERTIES
        OUTPUT_NAME "ps14_server"
    )
endif()

# ============================================================================
# KERNEL DRIVER (Kernel Mode - Separate Build)
# ============================================================================

option(BUILD_KERNEL_DRIVER "Build kernel-mode driver" OFF)

if(BUILD_KERNEL_DRIVER AND WIN32)
    message(STATUS "Configuring kernel driver...")
    find_path(WDK_INCLUDE_DIR NAMES ntddk.h ntifs.h PATHS "${WDK_DIR}/inc/ddk" "${WDK_DIR}/inc")
    
    if(WDK_INCLUDE_DIR)
        add_definitions(-DPS14_MODE_KERNEL -DKERNEL_MODE)
        
        add_library(ps14_driver MODULE
            src/kernel/driver.c
            src/kernel/memory_guard.c
            src/kernel/process_watcher.c
            src/kernel/callback_handler.c
            src/kernel/shadow_pt_detector.c
        )
        
        target_include_directories(ps14_driver PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${WDK_INCLUDE_DIR}
        )
        
        set_target_properties(ps14_driver PROPERTIES
            SUFFIX ".sys"
            OUTPUT_NAME "ps14_driver"
            POSITION_INDEPENDENT_CODE OFF
        )
        
        set_target_properties(ps14_driver PROPERTIES
            LINK_FLAGS "/NOIMPORTLIB /DRIVER /ENTRY:DriverEntry"
        )
        
        message(STATUS "Kernel driver configured. Requires WDK for full build.")
    else()
        message(WARNING "WDK not found. Kernel driver will not be built.")
    endif()
endif()

# ============================================================================
# TESTS
# ============================================================================

option(BUIL
