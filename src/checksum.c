// Credits 
/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Khashayar Fereidani */
/*https://github.com/fereidani/ip4sum/blob/main/c/checksum.c*/



#include "checksum.h"

#include <string.h>

#if defined(_MSC_VER)
#include <stdlib.h>
#include <immintrin.h>
#endif

/* ---- Endianness detection (C99 portable) ---- */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define IP4SUM_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define IP4SUM_LITTLE_ENDIAN 0
#elif defined(_WIN32) || defined(_WIN64)
#define IP4SUM_LITTLE_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) \
   || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) \
   || defined(__MIPSEL__) || defined(__riscv) || defined(__loongarch64) \
   || defined(__i386__) || defined(__x86_64__) || defined(__X86__) \
   || defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM64)
#define IP4SUM_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) \
   || defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) \
   || defined(__MIPSEB__) || defined(__s390__) || defined(__s390x__) \
   || defined(__SPU__) || defined(__hppa__)
#define IP4SUM_LITTLE_ENDIAN 0
#else
/* Assume little-endian as default (covers 99%+ of modern platforms).
 * If building for a big-endian target not listed above, define
 * IP4SUM_LITTLE_ENDIAN=0 in your build flags. */
#define IP4SUM_LITTLE_ENDIAN 1
#endif

/* ---- Byte-swap primitives (C99 portable) ---- */

#if defined(__has_builtin)
#if __has_builtin(__builtin_bswap16)
#define IP4SUM_BSWAP16(x) __builtin_bswap16(x)
#endif
#if __has_builtin(__builtin_bswap64)
#define IP4SUM_BSWAP64(x) __builtin_bswap64(x)
#endif
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#define IP4SUM_BSWAP16(x) __builtin_bswap16(x)
#define IP4SUM_BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#define IP4SUM_BSWAP16(x) _byteswap_ushort(x)
#define IP4SUM_BSWAP64(x) _byteswap_uint64(x)
#endif

#if !defined(IP4SUM_BSWAP16)
static inline uint16_t ip4sum_bswap16(uint16_t value) {
    return (uint16_t)(((value << 8) & 0xFF00u) | ((value >> 8) & 0x00FFu));
}
#define IP4SUM_BSWAP16(x) ip4sum_bswap16(x)
#endif

#if !defined(IP4SUM_BSWAP64)
static inline uint64_t ip4sum_bswap64(uint64_t value) {
    return ((value << 56) & 0xFF00000000000000ULL) |
           ((value << 40) & 0x00FF000000000000ULL) |
           ((value << 24) & 0x0000FF0000000000ULL) |
           ((value << 8)  & 0x000000FF00000000ULL) |
           ((value >> 8)  & 0x00000000FF000000ULL) |
           ((value >> 24) & 0x0000000000FF0000ULL) |
           ((value >> 40) & 0x000000000000FF00ULL) |
           ((value >> 56) & 0x00000000000000FFULL);
}
#define IP4SUM_BSWAP64(x) ip4sum_bswap64(x)
#endif

/*
 * Fold a 64-bit native-order accumulator into a 16-bit one's-complement
 * checksum in network byte order.
 *
 * One's complement addition is endian-independent: accumulating in native
 * order and byte-swapping only the final u16 is equivalent to swapping
 * on every update. This saves two swap_bytes calls per update for the
 * cost of one swap_bytes at the end.
 *
 * All operations here are branch-free (shifts, adds, masks, bitwise NOT).
 */
static inline uint16_t fold(uint64_t sum) {
    sum = (sum >> 32) + (sum & 0xffffffffu);
    sum = (sum >> 16) + (sum & 0xffffu);
    sum += sum >> 16;
    uint16_t result = ~(uint16_t)sum;

#if IP4SUM_LITTLE_ENDIAN
    return IP4SUM_BSWAP16(result);
#else
    return result;
#endif
}

/*
 * Branch-free IPv4 header checksum for standard 20-byte IPv4 headers.
 *
 * Implements RFC 1071 style summation using 64-bit loads (two 64-bit loads
 * plus one 32-bit load zero-extended) with zero loops and zero conditional branches.
 */
uint16_t ip4sum_ipv4_header_checksum_20(const unsigned char *data) {
    uint64_t w0, w1;
    uint32_t w2;

    /*
     * Compiler intrinsics / memcpy: Compilers (GCC, Clang, MSVC) optimize
     * fixed-size memcpy calls into single unaligned load instructions (movq / ldr)
     * avoiding alignment faults or undefined behavior.
     */
    memcpy(&w0, data, 8);
    memcpy(&w1, data + 8, 8);
    memcpy(&w2, data + 16, 4);

    uint64_t acc;
#if defined(__SIZEOF_INT128__)
    /* GCC / Clang on 64-bit targets: compiles directly to mov, add, adc, adc 0 */
    unsigned __int128 sum = (unsigned __int128)w0 + w1 + w2;
    acc = (uint64_t)sum + (uint64_t)(sum >> 64);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
    /* MSVC: hardware add-with-carry intrinsics */
    unsigned char carry = _addcarry_u64(0, w0, w1, &acc);
    carry = _addcarry_u64(carry, acc, (uint64_t)w2, &acc);
    _addcarry_u64(carry, acc, 0, &acc);
#else
    /* Portable C99 branch-free carry propagation */
    uint64_t sum = w0 + w1;
    uint64_t c0 = (sum < w0);
    sum += w2;
    uint64_t c1 = (sum < (uint64_t)w2);
    acc = sum + c0 + c1;
#endif

    return fold(acc);
}

/*
 * IPv4 header checksum for headers with or without options (IHL 5..15).
 * Dispatches to the 100% branch-free 20-byte path when ihl_bytes == 20.
 */
uint16_t ip4sum_ipv4_header_checksum(const unsigned char *data, size_t ihl_bytes) {
    if (ihl_bytes == 20) {
        return ip4sum_ipv4_header_checksum_20(data);
    }
    return ip4sum_checksum_oneshot(data, ihl_bytes);
}

/*
 * Accumulate the Internet checksum of data into c->acc.
 *
 * Processes data in 64-bit chunks using 64-bit loads with carry accumulation,
 * followed by 32-bit, 16-bit, and 8-bit tails.
 */
void ip4sum_checksum_update(ip4sum_checksum *c, const unsigned char *data,
                            size_t len) {
    uint64_t ac = c->acc;
    const unsigned char *ptr = data;

    while (len >= 8) {
        uint64_t w;
        memcpy(&w, ptr, 8);
        ac += w;
        ac += (ac < w); /* propagate carry in 64-bit one's complement */
        ptr += 8;
        len -= 8;
    }
    if (len >= 4) {
        uint32_t w;
        memcpy(&w, ptr, 4);
        ac += w;
        ac += (ac < (uint64_t)w);
        ptr += 4;
        len -= 4;
    }
    if (len >= 2) {
        uint16_t w;
        memcpy(&w, ptr, 2);
        ac += w;
        ac += (ac < (uint64_t)w);
        ptr += 2;
        len -= 2;
    }
    if (len > 0) {
        uint8_t w = ptr[0];
        ac += w;
        ac += (ac < (uint64_t)w);
    }

    c->acc = ac;
}

uint16_t ip4sum_checksum_finalize(ip4sum_checksum c) {
    return fold(c.acc);
}

uint16_t ip4sum_checksum_oneshot(const unsigned char *data, size_t len) {
    ip4sum_checksum c = ip4sum_checksum_new();
    ip4sum_checksum_update(&c, data, len);
    return ip4sum_checksum_finalize(c);
}
