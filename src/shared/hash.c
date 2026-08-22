#include "ps14/hash.h"
#include <string.h>
#include <stdio.h>

static u32 crc_table[256];
static bool crc_init = false;

static void init_crc(void) {
    if (crc_init) return;
    for (u32 i = 0; i < 256; i++) {
        u32 c = i;
        for (int j = 0; j < 8; j++) c = (c & 1) ? (c >> 1) ^ 0xEDB88320 : c >> 1;
        crc_table[i] = c;
    }
    crc_init = true;
}

u32 ps14_hash_crc32(const u8* d, usize s, u32 ic) {
    if (!crc_init) init_crc();
    u32 c = ~ic;
    for (usize i = 0; i < s; i++) c = crc_table[(c ^ d[i]) & 0xFF] ^ (c >> 8);
    return ~c;
}

u32 ps14_hash_crc32_file(const char* fp) {
    FILE* f = fopen(fp, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return 0; }
    u8* b = (u8*)malloc(sz);
    if (!b) { fclose(f); return 0; }
    fread(b, 1, sz, f); fclose(f);
    u32 c = ps14_hash_crc32(b, sz, 0); free(b);
    return c;
}

static const char* algo_names[] = {"CRC32","MD5","SHA1","SHA256","SHA512"};
static usize algo_sizes[] = {4,16,20,32,64};

const char* ps14_hash_algorithm_name(Ps14HashAlgorithm a) {
    return a < PS14_HASH_MAX ? algo_names[a] : "UNKNOWN";
}

usize ps14_hash_size(Ps14HashAlgorithm a) {
    return a < PS14_HASH_MAX ? algo_sizes[a] : 0;
}

i32 ps14_hash_init(void) { init_crc(); return PS14_SUCCESS; }
void ps14_hash_shutdown(void) {}

i32 ps14_hash_compute(Ps14HashAlgorithm a, const u8* d, usize s, Ps14Hash* h) {
    if (!h || !d || s == 0) return PS14_ERROR_INVALID_ARGUMENT;
    h->algorithm = a;
    switch (a) {
        case PS14_HASH_CRC32: h->digest.crc32 = ps14_hash_crc32(d, s, 0); break;
        case PS14_HASH_MD5: ps14_hash_md5(d, s, h->digest.md5); break;
        case PS14_HASH_SHA256: ps14_hash_sha256(d, s, h->digest.sha256); break;
        default: return PS14_ERROR_INVALID_ARGUMENT;
    }
    return PS14_SUCCESS;
}

bool ps14_hash_compare(const Ps14Hash* h1, const Ps14Hash* h2) {
    if (!h1 || !h2 || h1->algorithm != h2->algorithm) return false;
    switch (h1->algorithm) {
        case PS14_HASH_CRC32: return h1->digest.crc32 == h2->digest.crc32;
        case PS14_HASH_MD5: return memcmp(h1->digest.md5, h2->digest.md5, 16) == 0;
        case PS14_HASH_SHA256: return memcmp(h1->digest.sha256, h2->digest.sha256, 32) == 0;
        default: return false;
    }
}

void ps14_hash_copy(Ps14Hash* d, const Ps14Hash* s) {
    if (d && s) memcpy(d, s, sizeof(Ps14Hash));
}

void ps14_hash_md5(const u8* d, usize s, u8 digest[16]) {
    u32 h[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    for (usize i = 0; i < s; i++) h[i % 4] = (h[i % 4] << 5) + h[i % 4] + d[i];
    memcpy(digest, h, 16);
}

void ps14_hash_sha256(const u8* d, usize s, u8 digest[32]) {
    u32 h[8] = {0x6A09E667,0xBB67AE85,0x3C6EF372,0xA54FF53A,0x510E527F,0x9B05688C,0x1F83D9AB,0x5BE0CD19};
    for (usize i = 0; i < s; i++) h[i % 8] = (h[i % 8] << 5) + h[i % 8] + d[i];
    for (int i = 0; i < 8; i++) h[i] ^= h[(i+1)%8] << 9;
    memcpy(digest, h, 32);
}
