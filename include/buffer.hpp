#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <cstddef>
#include "packet.hpp"

namespace switchmodel {
    constexpr size_t total_buffer_bytes = 12 * 1024 * 1024; // 12Mb shared egress
    constexpr size_t block_sz = max_frame_sz;
    constexpr size_t num_blocks = total_buffer_bytes / block_sz;
    constexpr uint32_t NULL_INDEX = 0xFFFFFFFFU;

    // Cache-line aligned buffer block
    struct alignas(64) bufferblock
    {
        std::array<uint8_t, block_sz> data{};
        size_t length{0};
        packetmetadata metadata{};
    };

    class egressbufferpool {
    private:
        struct taggedidx {
            uint32_t idx;
            uint32_t tag;
        };
        static_assert(sizeof(taggedidx) == sizeof(uint64_t), "taggedidx size mismatch");

        // Contiguous memory blocks
        std::vector<bufferblock> blocks;
        // Lock-free freelist next pointers
        std::vector<uint32_t> free_indices;
        // Atomic allocation tracker for double-free prevention
        std::unique_ptr<std::atomic<uint8_t>[]> allocated;
        alignas(64) std::atomic<uint64_t> head{0};
        alignas(64) std::atomic<size_t> allocated_bytes{0};

        static uint64_t pack(uint32_t idx, uint32_t tag) noexcept {
            return (static_cast<uint64_t>(tag) << 32) | static_cast<uint64_t>(idx);
        }

        static taggedidx unpack(uint64_t value) noexcept {
            taggedidx t;
            t.idx = static_cast<uint32_t>(value & 0xFFFFFFFF);
            t.tag = static_cast<uint32_t>(value >> 32);
            return t;
        }

    public:
        egressbufferpool()
            : blocks(num_blocks),
              free_indices(num_blocks),
              allocated(std::make_unique<std::atomic<uint8_t>[]>(num_blocks))
        {
            for (size_t i = 0; i < num_blocks; i++) {
                free_indices[i] = (i + 1 < num_blocks) ? static_cast<uint32_t>(i + 1) : NULL_INDEX;
                allocated[i].store(0, std::memory_order_relaxed);
            }
            head.store(pack(0, 0), std::memory_order_relaxed);
        }

        // Disable copying
        egressbufferpool(const egressbufferpool&) = delete;
        egressbufferpool& operator=(const egressbufferpool&) = delete;

        bufferblock* allocate() noexcept {
            uint64_t curr = head.load(std::memory_order_acquire);
            while (true) {
                taggedidx t = unpack(curr);
                if (t.idx == NULL_INDEX) {
                    return nullptr; 
                }
                uint32_t next_idx = free_indices[t.idx];
                uint32_t new_tag = t.tag + 1;
                uint64_t new_head = pack(next_idx, new_tag);
                if (head.compare_exchange_weak(curr, new_head, std::memory_order_acq_rel, std::memory_order_acquire)) {
                    allocated[static_cast<size_t>(t.idx)].store(1, std::memory_order_release);
                    allocated_bytes.fetch_add(block_sz, std::memory_order_relaxed);
                    return &blocks[static_cast<size_t>(t.idx)];
                }
            }
        }

        void deallocate(bufferblock* block) noexcept {
            if (!block) return;

            ptrdiff_t diff = block - blocks.data();
            if (diff < 0 || static_cast<size_t>(diff) >= blocks.size()) {
                return;
            }

            size_t idx = static_cast<size_t>(diff);
            if (allocated[idx].exchange(0, std::memory_order_acq_rel) == 0) {
                return; // Double-free guard
            }

            allocated_bytes.fetch_sub(block_sz, std::memory_order_relaxed);
            uint64_t curr = head.load(std::memory_order_acquire);
            while (true) {
                taggedidx t = unpack(curr);
                free_indices[idx] = t.idx;
                uint32_t new_tag = t.tag + 1;
                uint64_t new_head = pack(static_cast<uint32_t>(idx), new_tag);
                if (head.compare_exchange_weak(curr, new_head, std::memory_order_acq_rel, std::memory_order_acquire)) {
                    return;
                }
            }
        }

        [[nodiscard]] size_t get_alloc_bytes() const noexcept {
            return allocated_bytes.load(std::memory_order_relaxed);
        }

        [[nodiscard]] bool is_exhaust() const noexcept {
            uint64_t curr = head.load(std::memory_order_relaxed);
            return unpack(curr).idx == NULL_INDEX;
        }
    };
}
