#ifndef PS14_HASH_H
#define PS14_HASH_H

#pragma once

#include "ps14.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Hash System
//
// Provides various hash algorithms for integrity verification.
// Supports CRC32, MD5, SHA-1, SHA-256, SHA-512.
// ============================================================================

// Hash result structure (can hold any hash type)
typedef struct Ps14Hash {
    Ps14HashAlgorithm algorithm;
    union {
        u32 crc32;
        u8 md5[16];
        u8 sha1[20];
        u8 sha256[32];
        u8 sha512[64];
    } digest;
} Ps14Hash;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * @brief Initialize hash system
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_hash_init(void);

/**
 * @brief Shutdown hash system
 */
PS14_API void ps14_hash_shutdown(void);

/**
 * @brief Compute hash of data
 * @param algorithm Hash algorithm to use
 * @param data Data to hash
 * @param data_size Size of data in bytes
 * @param hash Output hash structure
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_hash_compute(
    Ps14HashAlgorithm algorithm,
    const u8* data,
    usize data_size,
    Ps14Hash* hash
);

/**
 * @brief Compute hash of a file
 * @param algorithm Hash algorithm to use
 * @param filepath Path to the file
 * @param hash Output hash structure
 * @return PS14_SUCCESS on success, error code on failure
 */
PS14_API i32 ps14_hash_compute_file(
    Ps14HashAlgorithm algorithm,
    const char* filepath,
    Ps14Hash* hash
);

/**
 * @brief Compare two hashes
 * @param hash1 First hash
 * @param hash2 Second hash
 * @return true if hashes are equal, false otherwise
 */
PS14_API bool ps14_hash_compare(const Ps14Hash* hash1, const Ps14Hash* hash2);

/**
 * @brief Copy a hash
 * @param dest Destination hash
 * @param src Source hash
 */
PS14_API void ps14_hash_copy(Ps14Hash* dest, const Ps14Hash* src);

/**
 * @brief Get hash algorithm name
 * @param algorithm Hash algorithm
 * @return String name of the algorithm
 */
PS14_API const char* ps14_hash_algorithm_name(Ps14HashAlgorithm algorithm);

/**
 * @brief Get hash size in bytes
 * @param algorithm Hash algorithm
 * @return Size of hash in bytes
 */
PS14_API usize ps14_hash_size(Ps14HashAlgorithm algorithm);

/**
 * @brief Convert hash to hex string
 * @pa
