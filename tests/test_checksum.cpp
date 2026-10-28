#include "checksum.h"
#include "common.hpp"

#include <cstring>
#include <vector>
#include <random>

// RFC 1071 reference implementation
static uint16_t rfc1071_reference(const unsigned char *data, size_t len) {
    uint32_t sum = 0;
    size_t i = 0;
    while (i + 1 < len) {
        uint16_t word = (static_cast<uint16_t>(data[i]) << 8) | data[i + 1];
        sum += word;
        i += 2;
    }
    if (i < len) {
        uint16_t word = static_cast<uint16_t>(data[i]) << 8;
        sum += word;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

int main() {
    // Test 1: Standard 20-byte IPv4 Header
    {
        uint8_t hdr[20] = {
            0x45, 0x00, 0x00, 0x14, // Ver=4, IHL=5, Len=20
            0x00, 0x00, 0x00, 0x00, // ID=0, Flags/Frag=0
            0x40, 0x00, 0x00, 0x00, // TTL=64, Proto=0, Checksum=0
            192,  0,    2,    1,    // Src: 192.0.2.1
            192,  168,  1,    50    // Dst: 192.168.1.50
        };

        uint16_t csum = ip4sum_ipv4_header_checksum_20(hdr);
        uint16_t ref = rfc1071_reference(hdr, 20);
        CHECK(csum == 0xF70E);
        CHECK(csum == ref);

        // Put checksum into header
        hdr[10] = static_cast<uint8_t>(csum >> 8);
        hdr[11] = static_cast<uint8_t>(csum & 0xFF);

        // Validated header must sum to 0
        uint16_t verify = ip4sum_ipv4_header_checksum_20(hdr);
        CHECK(verify == 0);
    }

    // Test 2: Checksum corruption detection
    {
        uint8_t hdr[20] = {
            0x45, 0x00, 0x00, 0x14,
            0x00, 0x00, 0x00, 0x00,
            0x40, 0x00, 0xF7, 0x0E,
            192,  0,    2,    1,
            192,  168,  1,    50
        };
        // Corrupt 1 bit in destination IP
        hdr[19] ^= 1;
        CHECK(ip4sum_ipv4_header_checksum_20(hdr) != 0);

        // Corrupt TTL
        hdr[19] ^= 1; // restore
        hdr[8] ^= 0x20;
        CHECK(ip4sum_ipv4_header_checksum_20(hdr) != 0);
    }

    // Test 3: IPv4 header with options (IHL = 7, 28 bytes)
    {
        uint8_t hdr[28] = {
            0x47, 0x00, 0x00, 0x1C, // IHL=7 (28 bytes)
            0x12, 0x34, 0x40, 0x00,
            0x40, 0x06, 0x00, 0x00, // Checksum = 0
            10,   0,    0,    1,
            10,   0,    0,    2,
            0x01, 0x01, 0x01, 0x01, // Options NOPs
            0x01, 0x01, 0x01, 0x01
        };

        uint16_t csum = ip4sum_ipv4_header_checksum(hdr, 28);
        uint16_t ref = rfc1071_reference(hdr, 28);
        CHECK(csum == ref);

        hdr[10] = static_cast<uint8_t>(csum >> 8);
        hdr[11] = static_cast<uint8_t>(csum & 0xFF);
        CHECK(ip4sum_ipv4_header_checksum(hdr, 28) == 0);
    }

    // Test 4: Incremental / Streaming Checksum (Multi-part)
    {
        uint8_t data[64];
        for (size_t i = 0; i < sizeof(data); ++i) {
            data[i] = static_cast<uint8_t>(i * 3 + 7);
        }

        uint16_t oneshot = ip4sum_checksum_oneshot(data, sizeof(data));
        uint16_t ref = rfc1071_reference(data, sizeof(data));
        CHECK(oneshot == ref);

        // Split into chunks of 20 and 44 bytes
        ip4sum_checksum c = ip4sum_checksum_new();
        ip4sum_checksum_update(&c, data, 20);
        ip4sum_checksum_update(&c, data + 20, 44);
        uint16_t incremental = ip4sum_checksum_finalize(c);
        CHECK(incremental == oneshot);
    }

    // Test 5: Randomized 20-byte headers vs RFC 1071 reference
    {
        std::mt19937_64 rng(1337);
        for (int i = 0; i < 5000; ++i) {
            uint8_t hdr[20];
            for (auto &b : hdr) {
                b = static_cast<uint8_t>(rng());
            }
            hdr[10] = 0;
            hdr[11] = 0;

            uint16_t fast = ip4sum_ipv4_header_checksum_20(hdr);
            uint16_t ref = rfc1071_reference(hdr, 20);
            CHECKF(fast == ref, "Mismatch at iteration %d: fast=0x%04x ref=0x%04x", i, fast, ref);

            // Put checksum in and verify evaluates to 0
            hdr[10] = static_cast<uint8_t>(fast >> 8);
            hdr[11] = static_cast<uint8_t>(fast & 0xFF);
            uint16_t verify = ip4sum_ipv4_header_checksum_20(hdr);
            CHECKF(verify == 0, "Verify non-zero at iteration %d: got 0x%04x", i, verify);
        }
    }

    // Test 6: Arbitrary lengths (1 to 1500 bytes)
    {
        std::mt19937 rng(42);
        for (size_t len = 1; len <= 128; ++len) {
            std::vector<uint8_t> buf(len);
            for (auto &b : buf) b = static_cast<uint8_t>(rng());

            uint16_t fast = ip4sum_checksum_oneshot(buf.data(), len);
            uint16_t ref = rfc1071_reference(buf.data(), len);
            CHECKF(fast == ref, "Len %zu mismatch: fast=0x%04x ref=0x%04x", len, fast, ref);
        }
    }

    return report();
}

