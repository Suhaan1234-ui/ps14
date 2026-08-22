#ifndef PS14_H
#define PS14_H

#pragma once

// ============================================================================
// ps14 Anti-Tamper Game Engine - Main Header
// 
// This header contains common definitions, macros, and includes for the
// entire ps14 project.
//
// Author: Mistral Vibe
// Version: 1.0
// ============================================================================

// ============================================================================
// PLATFORM DETECTION
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define PS14_PLATFORM_WINDOWS 1
    #define PS14_PLATFORM_NAME "Windows"
    
    #ifdef _WIN64
        #define PS14_ARCH_X64 1
        #define PS14_ARCH_NAME "x64"
    #else
        #define PS14_ARCH_X86 1
        #define PS14_ARCH_NAME "x86"
    #endif
        
#elif defined(__linux__)
    #define PS14_PLATFORM_LINUX 1
    #define PS14_PLATFORM_NAME "Linux"
    
    #if defined(__x86_64__) || defined(_M_X64)
        #define PS14_ARCH_X64 1
        #define PS14_ARCH_NAME "x64"
    #else
        #define PS14_ARCH_X86 1
        #define PS14_ARCH_NAME "x86"
    #endif

#else
    #error "Unsupported platform"
#endif

// ============================================================================
// BUILD MODE
// ============================================================================

#ifdef _DEBUG
    #define PS14_BUILD_DEBUG 1
    #define PS14_BUILD_NAME "Debug"
#else
    #define PS14_BUILD_RELEASE 1
    #define PS14_BUILD_NAME "Release"
#endif

// ============================================================================
// MODE DETECTION
// ============================================================================

#ifdef _KERNEL_MODE
    #define PS14_MODE_KERNEL 1
    #define PS14_MODE_NAME "Kernel"
#elif defined(_DRIVER)
    #define PS14_MODE_KERNEL 1
    #define PS14_MODE_NAME "Driver"
#else
    #define PS14_MODE_USER 1
    #define PS14_MODE_NAME "User"
#endif

// ============================================================================
// EXPORT/IMPORT MACROS
// ============================================================================

#if defined(_WIN32) && defined(PS14_MODE_USER)
    #ifdef PS14_CORE_EXPORTS
        #define PS14_API __declspec(dllexport)
    #else
        #define PS14_API __declspec(dllimport)
    #endif
#else
    #define PS14_API
#endif

// ============================================================================
// STANDARD TYPES
// ============================================================================

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Fixed-width types (ensure portability)
typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef size_t   usize;
typedef ptrdiff_t isize;

// ============================================================================
// COMPILER SPECIFIC
// ============================================================================

#if defined(_MSC_VER)
    #define PS14_COMPILER_MSVC 1
    #define PS14_COMPILER_NAME "MSVC"
    #define PS14_COMPILER_VERSION _MSC_VER
    
    #pragma warning(disable: 4996) // Disable deprecation warnings
    #pragma warning(disable: 4251) // Disable DLL interface warnings
    
    #define PS14_INLINE __inline
    #define PS14_NOINLINE __declspec(noinline)
    #define PS14_RESTRICT __restrict
    
    #define PS14_UNREACHABLE() __assume(0)
    
    #define PS14_PRINTF_FORMAT(string_index, first_to_check) \
        __declspec(format(printf, string_index, first_to_check))
    
#elif defined(__GNUC__) || defined(__clang__)
    #define PS14_COMPILER_GCC 1
    #define PS14_COMPILER_NAME "GCC/Clang"
    #define PS14_COMPILER_VERSION __GNUC__
    
    #define PS14_INLINE inline
    #define PS14_NOINLINE __attribute__((noinline))
    #define PS14_RESTRICT __restrict
    
    #define PS14_UNREACHABLE() __builtin_unreachable()
    
    #define PS14_PRINTF_FORMAT(string_index, first_to_check) \
        __attribute__((format(printf, string_index, first_to_check)))

#else
    #error "Unsupported compiler"
#endif

// ============================================================================
// ALIGNMENT
// ============================================================================

#if defined(_MSC_VER)
    #define PS14_ALIGN(n) __declspec(align(n))
#else
    #define PS14_ALIGN(n) __attribute__((aligned(n)))
#endif

#define PS14_ALIGN_CACHE PS14_ALIGN(64)
#define PS14_ALIGN_PAGE PS14_ALIGN(4096)

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

#include <stdlib.h>
#include <string.h>

#ifdef PS14_PLATFORM_WINDOWS
    #include <malloc.h>
    #define PS14_ALLOC(size) _aligned_malloc(size, 16)
    #define PS14_FREE(ptr) _aligned_free(ptr)
#else
    #define PS14_ALLOC(size) aligned_alloc(16, size)
    #define PS14_FREE(ptr) free(ptr)
#endif

#define PS14_MALLOC(size) malloc(size)
#define PS14_CALLOC(count, size) calloc(count, size)
#define PS14_REALLOC(ptr, size) realloc(ptr, size)
#define PS14_FREE_SAFE(ptr) do { if (ptr) { free(ptr); ptr = NULL; } } while(0)

// ============================================================================
// STRING UTILITIES
// ============================================================================

#define PS14_STRINGIFY(x) #x
#define PS14_TOSTRING(x) PS14_STRINGIFY(x)

#define PS14_CONCAT(a, b) a ## b
#define PS14_CONCAT2(a, b) PS14_CONCAT(a, b)

// ============================================================================
// ERROR CODES
// ============================================================================

// Success
#define PS14_SUCCESS 0

// Errors (0x0001 - 0x7FFF for user mode)
#define PS14_ERROR_UNKNOWN 0x0001
#define PS14_ERROR_OUT_OF_MEMORY 0x0002
#define PS14_ERROR_INVALID_ARGUMENT 0x0003
#define PS14_ERROR_NOT_FOUND 0x0004
#define PS14_ERROR_ALREADY_EXISTS 0x0005
#define PS14_ERROR_NOT_INITIALIZED 0x0006
#define PS14_ERROR_ALREADY_INITIALIZED 0x0007
#define PS14_ERROR_MEMORY_SCAN_FAILED 0x0010
#define PS14_ERROR_FILE_VERIFICATION_FAILED 0x0011
#define PS14_ERROR_AUTHENTICATION_FAILED 0x0012
#define PS14_ERROR_NETWORK_ERROR 0x0013
#define PS14_ERROR_INVALID_CONFIGURATION 0x0014
#define PS14_ERROR_TAMPERING_DETECTED 0x0015
#define PS14_ERROR_REPAIR_FAILED 0x0016

// Kernel mode errors (0x8000 - 0xFFFF)
#define PS14_ERROR_KERNEL_BASE 0x8000
#define PS14_ERROR_MEMORY_PROTECTION_FAILED (PS14_ERROR_KERNEL_BASE + 1)
#define PS14_ERROR_CALLBACK_REGISTRATION_FAILED (PS14_ERROR_KERNEL_BASE + 2)
#define PS14_ERROR_PROCESS_MONITORING_FAILED (PS14_ERROR_KERNEL_BASE + 3)
#define PS14_ERROR_INVALID_PARAMETERS (PS14_ERROR_KERNEL_BASE + 4)
#define PS14_ERROR_ACCESS_DENIED (PS14_ERROR_KERNEL_BASE + 5)
#define PS14_ERROR_RESOURCE_EXHAUSTION (PS14_ERROR_KERNEL_BASE + 6)

// ============================================================================
// LOG LEVELS
// ============================================================================

typedef enum Ps14LogLevel {
    PS14_LOG_LEVEL_FATAL = 0,
    PS14_LOG_LEVEL_ERROR = 1,
    PS14_LOG_LEVEL_WARNING = 2,
    PS14_LOG_LEVEL_INFO = 3,
    PS14_LOG_LEVEL_DEBUG = 4,
    PS14_LOG_LEVEL_TRACE = 5,
    PS14_LOG_LEVEL_MAX
} Ps14LogLevel;

// ============================================================================
// HASH ALGORITHMS
// ============================================================================

typedef enum Ps14HashAlgorithm {
    PS14_HASH_CRC32 = 0,
    PS14_HASH_MD5,
    PS14_HASH_SHA1,
    PS14_HASH_SHA256,
    PS14_HASH_SHA512,
    PS14_HASH_MAX
} Ps14HashAlgorithm;

// ============================================================================
// VERSION INFORMATION
// ============================================================================

#define PS14_VERSION_MAJOR 1
#define PS14_VERSION_MINOR 0
#define PS14_VERSION_PATCH 0
#define PS14_V

// ============================================================================
// STRING LIMITS
// ============================================================================

#define PS14_MAX_PATH 260
#define PS14_MAX_NAME 64
#define PS14_MAX_MESSAGE 256
#define PS14_MAX_URL 512
