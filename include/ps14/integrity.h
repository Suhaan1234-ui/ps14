#ifndef PS14_INTEGRITY_H
#define PS14_INTEGRITY_H

#pragma once

#include "ps14.h"
#include "ps14/hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Ps14FileInfo {
    char path[PS14_MAX_PATH];
    Ps14Hash hash;
    u64 last_modified;
    bool is_valid;
} Ps14FileInfo;

typedef struct Ps14IntegrityChecker Ps14IntegrityChecker;

PS14_API i32 ps14_integrity_init(void);
PS14_API void ps14_integrity_shutdown(void);

PS14_API i32 ps14_integrity_add_file(const char* path);
PS14_API i32 ps14_integrity_remove_file(const char* path);
PS14_API bool ps14_integrity_verify_file(const char* path);
PS14_API i32 ps14_integrity_scan_all(void);

PS14_API bool ps14_integrity_verify_code_signature(const char* path);

PS14_API i32 ps14_checksum_db_init(const char* db_path);
PS14_API void ps14_checksum_db_shutdown(void);
PS14_API i32 ps14_checksum_db_add(const char* path, const Ps14Hash* hash);
PS14_API bool ps14_checksum_db_verify(const char* path);

#ifdef __cplusplus
}
#endif
#endif
