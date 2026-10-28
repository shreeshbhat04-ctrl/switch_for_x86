// Credits
/* SPDX-License-Identifier: MIT */
/* Copyright (c) 2026 Khashayar Fereidani */
/*https://github.com/fereidani/ip4sum/blob/main/c/checksum.h*/

#ifndef IP4SUM_CHECKSUM_H
#define IP4SUM_CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Incremental checksum state. */
typedef struct {
    uint64_t acc;
} ip4sum_checksum;

/* Initialize a new checksum calculator. */
static inline ip4sum_checksum ip4sum_checksum_new(void) {
    ip4sum_checksum c;
    c.acc = 0;
    return c;
}

/* Reset the calculator to its initial state. */
static inline void ip4sum_checksum_reset(ip4sum_checksum *c) {
    c->acc = 0;
}

/* Feed a byte slice into the running checksum using 64-bit loads. */
void ip4sum_checksum_update(ip4sum_checksum *c, const unsigned char *data,
                            size_t len);

/* Finalize and return the 16-bit one's-complement checksum
   in network byte order. */
uint16_t ip4sum_checksum_finalize(ip4sum_checksum c);

/* One-shot: compute the Internet checksum of a byte slice. */
uint16_t ip4sum_checksum_oneshot(const unsigned char *data, size_t len);

/*
 * Branch-free IPv4 header checksum for standard 20-byte IPv4 headers.
 *
 * Implements RFC 1071 style summation using 64-bit loads (two 64-bit
 * loads + one 32-bit load) with zero loops and zero conditional branches.
 *
 * Parameters:
 *   data - pointer to at least 20 bytes of contiguous IPv4 header.
 *
 * Returns:
 *   16-bit one's-complement checksum in network byte order.
 *   - When computing checksum: call with checksum field set to 0.
 *   - When validating checksum: returns 0 if header is valid.
 */
uint16_t ip4sum_ipv4_header_checksum_20(const unsigned char *data);

/*
 * IPv4 header checksum for headers with or without options (IHL 5..15).
 * Dispatches directly to the branch-free 20-byte path when ihl_bytes == 20.
 */
uint16_t ip4sum_ipv4_header_checksum(const unsigned char *data, size_t ihl_bytes);

#ifdef __cplusplus
}
#endif

#endif /* IP4SUM_CHECKSUM_H */
