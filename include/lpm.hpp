#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace switchmodel {

    class lpmtable {
    private:
    static constexpr std::size_t stride_bits = 4;
    static constexpr std::size_t stride_count = 1U << stride_bits;
    static constexpr std::size_t max_levels = 32 / stride_bits;

    struct Node {
        std::array<int32_t, stride_count> child{};
        int outport{-1};

        Node() noexcept
        {
            child.fill(-1);
        }
    };

    struct Snapshot {
        std::vector<Node> nodes;

        Snapshot() : nodes(1) {}
    };

    std::shared_ptr<const Snapshot> snapshot_;
    mutable std::mutex update_mutex_;

    static uint8_t nibble(uint32_t prefix, std::size_t level) noexcept
    {
        const std::size_t shift = 28 - level * stride_bits;
        return static_cast<uint8_t>((prefix >> shift) & 0x0FU);
    }

    static int32_t clone_child(Snapshot& snapshot, int32_t parent, uint8_t index)
    {
        const int32_t child = snapshot.nodes[parent].child[index];
        if (child >= 0) {
            return child;
        }
        snapshot.nodes[parent].child[index] =
            static_cast<int32_t>(snapshot.nodes.size());
        snapshot.nodes.emplace_back();
        return snapshot.nodes[parent].child[index];
    }

    static void insert_route(
        Snapshot& snapshot, uint32_t prefix, uint8_t prefix_len, int outport)
    {
        const std::size_t full_levels = prefix_len / stride_bits;
        const uint8_t remainder = prefix_len % stride_bits;
        int32_t node = 0;

        for (std::size_t level = 0; level < full_levels; ++level) {
            node = clone_child(snapshot, node, nibble(prefix, level));
        }

        if (remainder == 0) {
            snapshot.nodes[node].outport = outport;
            return;
        }

        const uint8_t selected = nibble(prefix, full_levels);
        const uint8_t mask = static_cast<uint8_t>(
            0x0FU << (stride_bits - remainder));
        const uint8_t first = selected & mask;
        const uint8_t last = static_cast<uint8_t>(first | ((1U << (stride_bits - remainder)) - 1U));
        for (uint8_t value = first; value <= last; ++value) {
            const int32_t child = clone_child(snapshot, node, value);
            snapshot.nodes[child].outport = outport;
        }
    }

    public:
    lpmtable() : snapshot_(std::make_shared<const Snapshot>()) {}

    void insert(uint32_t prefix, uint8_t prefix_len, int out_interf)
    {
        if (prefix_len > 32) {
            return;
        }

        std::lock_guard<std::mutex> lock(update_mutex_);
        const auto current = std::atomic_load_explicit(
            &snapshot_, std::memory_order_acquire);
        auto next = std::make_shared<Snapshot>(*current);
        insert_route(*next, prefix, prefix_len, out_interf);
        std::atomic_store_explicit(
            &snapshot_,
            std::shared_ptr<const Snapshot>(std::move(next)),
            std::memory_order_release);
    }

    [[nodiscard]] int lookup(uint32_t dstip) const noexcept
    {
        const auto snapshot = std::atomic_load_explicit(
            &snapshot_, std::memory_order_acquire);
        int best_port = snapshot->nodes[0].outport;
        int32_t node = 0;

        for (std::size_t level = 0; level < max_levels; ++level) {
            const int32_t child =
                snapshot->nodes[node].child[nibble(dstip, level)];
            if (child < 0) {
                break;
            }
            node = child;
            if (snapshot->nodes[node].outport >= 0) {
                best_port = snapshot->nodes[node].outport;
            }
        }
        return best_port;
    }

    [[nodiscard]] std::size_t get_nod_cnt() const noexcept
    {
        const auto snapshot = std::atomic_load_explicit(
            &snapshot_, std::memory_order_acquire);
        return snapshot->nodes.size();
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(update_mutex_);
        std::atomic_store_explicit(
            &snapshot_,
            std::make_shared<const Snapshot>(),
            std::memory_order_release);
    }
};

}
