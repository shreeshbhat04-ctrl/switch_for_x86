#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace switchmodel {

/**
 * DIR-24-8 Longest Prefix Match (LPM) Data Structure
 *
 * Adapted from the formally verified DIR-24-8 LPM algorithm:
 *   simon-perriard/verified_DIR-24-8_LPM
 *   (Based on Gupta, Prabhakar, McKeown - Stanford Infocom '98)
 *
 * Architecture:
 * - Table 1 (lpm_24): 2^24 = 16,777,216 entries for prefixes <= 24 bits.
 *   Each entry is 16 bits:
 *     Bit 15 == 0: Bits 14..0 hold the next-hop port value directly.
 *     Bit 15 == 1: Bits 14..0 hold the chunk index in Table 2 (lpm_long).
 *     Value 0xFFFF (INVALID): No route / miss.
 * - Table 2 (lpm_long): Chunks of 2^8 = 256 entries for prefixes > 24 bits.
 *   Indexed by (chunk_index * 256 + last_byte_of_ip).
 *
 * Lookups require:
 * - 1 direct memory access for prefixes <= 24 bits (~99% of Internet routes).
 * - 2 direct memory accesses for prefixes > 24 bits.
 * - Zero pointer traversals, lock-free concurrency for packet forwarding.
 */

constexpr std::size_t lpm_PLEN_MAX = 32;
constexpr std::size_t BYTE_SIZE = 8;
constexpr uint16_t INVALID = 0xFFFF;
constexpr uint16_t lpm_24_FLAG_MASK = 0x8000; // Bit 15: 0->next hop, 1->lpm_long lookup
constexpr std::size_t lpm_24_MAX_ENTRIES = 16777216; // 2^24
constexpr uint16_t lpm_24_VAL_MASK = 0x7FFF;
constexpr uint8_t lpm_24_PLEN_MAX = 24;
constexpr std::size_t lpm_LONG_FACTOR = 256; // 2^8
constexpr std::size_t lpm_LONG_OFFSET_MAX = 32768; // Max chunks in lpm_long (15 bits)
constexpr std::size_t lpm_LONG_MAX_ENTRIES = lpm_LONG_OFFSET_MAX * lpm_LONG_FACTOR;
constexpr uint16_t MAX_NEXT_HOP_VALUE = 0x7FFF;

struct rule {
    uint32_t ipv4{0};
    uint8_t prefixlen{0};
    uint16_t route{INVALID};
};

inline uint32_t lpm_24_extract_first_index(uint32_t data) noexcept
{
    return data >> BYTE_SIZE;
}

inline bool lpm_24_entry_flag(uint16_t entry) noexcept
{
    return (entry & lpm_24_FLAG_MASK) != 0;
}

inline uint16_t lpm_24_entry_set_flag(uint16_t entry) noexcept
{
    return static_cast<uint16_t>(entry | lpm_24_FLAG_MASK);
}

inline uint32_t compute_rule_size(uint8_t prefixlen) noexcept
{
    if (prefixlen <= lpm_24_PLEN_MAX) {
        return 1U << (24 - prefixlen);
    } else {
        return 1U << (32 - prefixlen);
    }
}

inline uint32_t build_mask_from_prefixlen(uint8_t prefixlen) noexcept
{
    if (prefixlen == 0) return 0U;
    if (prefixlen >= 32) return 0xFFFFFFFFU;
    return ~0U << (32 - prefixlen);
}

inline uint32_t lpm_long_extract_first_index(uint32_t data, uint16_t base_index) noexcept
{
    return (static_cast<uint32_t>(base_index) * lpm_LONG_FACTOR) + (data & 0xFF);
}

class lpmtable {
private:
    std::vector<uint16_t> lpm_24_;
    std::vector<int8_t> depth_24_;
    std::vector<uint16_t> lpm_long_;
    std::vector<int8_t> depth_long_;
    std::size_t num_chunks_{0};
    mutable std::mutex update_mutex_;

public:
    lpmtable()
        : lpm_24_(lpm_24_MAX_ENTRIES, INVALID),
          depth_24_(lpm_24_MAX_ENTRIES, -1),
          lpm_long_(lpm_LONG_MAX_ENTRIES, INVALID),
          depth_long_(lpm_LONG_MAX_ENTRIES, -1)
    {}

    lpmtable(const lpmtable&) = delete;
    lpmtable& operator=(const lpmtable&) = delete;
    lpmtable(lpmtable&&) = default;
    lpmtable& operator=(lpmtable&&) = default;

    void insert(uint32_t prefix, uint8_t prefix_len, int out_interf)
    {
        if (prefix_len > 32 || out_interf < 0) {
            return;
        }

        std::lock_guard<std::mutex> lock(update_mutex_);
        const uint32_t mask = build_mask_from_prefixlen(prefix_len);
        prefix &= mask;
        const uint16_t route_val = static_cast<uint16_t>(out_interf & lpm_24_VAL_MASK);

        if (prefix_len <= lpm_24_PLEN_MAX) {
            const uint32_t first_idx = lpm_24_extract_first_index(prefix);
            const uint32_t count = compute_rule_size(prefix_len);
            const uint32_t last_idx = first_idx + count;

            for (uint32_t i = first_idx; i < last_idx; ++i) {
                if (lpm_24_[i] != INVALID && lpm_24_entry_flag(lpm_24_[i])) {
                    // Points to lpm_long chunk: update sub-entries with lower or equal prefix length
                    const uint32_t chunk_idx = lpm_24_[i] & lpm_24_VAL_MASK;
                    const uint32_t base = chunk_idx * lpm_LONG_FACTOR;
                    for (uint32_t j = 0; j < lpm_LONG_FACTOR; ++j) {
                        if (static_cast<int>(prefix_len) >= depth_long_[base + j]) {
                            lpm_long_[base + j] = route_val;
                            depth_long_[base + j] = static_cast<int8_t>(prefix_len);
                        }
                    }
                    if (static_cast<int>(prefix_len) >= depth_24_[i]) {
                        depth_24_[i] = static_cast<int8_t>(prefix_len);
                    }
                } else {
                    if (static_cast<int>(prefix_len) >= depth_24_[i]) {
                        lpm_24_[i] = route_val;
                        depth_24_[i] = static_cast<int8_t>(prefix_len);
                    }
                }
            }
        } else {
            // prefix_len > 24: requires lpm_long chunk
            const uint32_t idx24 = lpm_24_extract_first_index(prefix);
            uint32_t chunk_idx = 0;

            if (lpm_24_[idx24] != INVALID && lpm_24_entry_flag(lpm_24_[idx24])) {
                chunk_idx = lpm_24_[idx24] & lpm_24_VAL_MASK;
            } else {
                if (num_chunks_ >= lpm_LONG_OFFSET_MAX) {
                    return; // Table capacity reached
                }
                chunk_idx = num_chunks_++;
                const uint16_t existing_route = lpm_24_[idx24];
                const int8_t existing_depth = depth_24_[idx24];
                const uint32_t base = chunk_idx * lpm_LONG_FACTOR;

                for (uint32_t k = 0; k < lpm_LONG_FACTOR; ++k) {
                    lpm_long_[base + k] = existing_route;
                    depth_long_[base + k] = existing_depth;
                }
                std::atomic_thread_fence(std::memory_order_release);
                lpm_24_[idx24] = lpm_24_entry_set_flag(static_cast<uint16_t>(chunk_idx));
            }

            const uint32_t first_long = lpm_long_extract_first_index(prefix, static_cast<uint16_t>(chunk_idx));
            const uint32_t count = compute_rule_size(prefix_len);
            const uint32_t last_long = first_long + count;

            for (uint32_t i = first_long; i < last_long; ++i) {
                if (static_cast<int>(prefix_len) >= depth_long_[i]) {
                    lpm_long_[i] = route_val;
                    depth_long_[i] = static_cast<int8_t>(prefix_len);
                }
            }
        }
    }

    [[nodiscard]] int lookup(uint32_t dstip) const noexcept
    {
        const uint32_t idx = lpm_24_extract_first_index(dstip);
        const uint16_t entry = lpm_24_[idx];
        if (entry != INVALID && lpm_24_entry_flag(entry)) {
            const uint16_t chunk_idx = entry & lpm_24_VAL_MASK;
            const uint32_t long_idx = lpm_long_extract_first_index(dstip, chunk_idx);
            const uint16_t long_entry = lpm_long_[long_idx];
            return (long_entry == INVALID) ? -1 : static_cast<int>(long_entry);
        }
        return (entry == INVALID) ? -1 : static_cast<int>(entry);
    }

    [[nodiscard]] std::size_t get_nod_cnt() const noexcept
    {
        std::lock_guard<std::mutex> lock(update_mutex_);
        return 1 + num_chunks_;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(update_mutex_);
        std::fill(lpm_24_.begin(), lpm_24_.end(), INVALID);
        std::fill(depth_24_.begin(), depth_24_.end(), -1);
        if (num_chunks_ > 0) {
            const std::size_t used = std::min(num_chunks_ * lpm_LONG_FACTOR, lpm_LONG_MAX_ENTRIES);
            std::fill(lpm_long_.begin(), lpm_long_.begin() + used, INVALID);
            std::fill(depth_long_.begin(), depth_long_.begin() + used, -1);
        }
        num_chunks_ = 0;
    }
};

} 
