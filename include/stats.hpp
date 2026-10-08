#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>

namespace switchmodel {

class PerformanceStats {
private:
    std::atomic<uint64_t> total_packets_received_{0};
    std::atomic<uint64_t> total_packets_forwarded_{0};
    std::atomic<uint64_t> total_packets_dropped_{0};
    std::atomic<uint64_t> malformed_packets_dropped_{0};
    std::atomic<uint64_t> buffer_exhaustion_dropped_{0};
    std::array<uint64_t, 64> latency_buckets_{};
    mutable std::mutex latency_mutex_;
    double cpu_frequency_ghz_;

    void add_latency_cycles(uint64_t cycles) noexcept
    {
        std::size_t bucket = 0;
        while (cycles > 1 && bucket + 1 < latency_buckets_.size()) {
            cycles >>= 1;
            ++bucket;
        }
        std::lock_guard<std::mutex> lock(latency_mutex_);
        ++latency_buckets_[bucket];
    }

public:
    explicit PerformanceStats(double cpu_ghz = 3.0) noexcept
        : cpu_frequency_ghz_(cpu_ghz > 0.0 ? cpu_ghz : 3.0) {}

    [[nodiscard]] static uint64_t get_rdtsc() noexcept
    {
#if defined(_MSC_VER)
        return __rdtsc();
#elif defined(__x86_64__) || defined(__i386__)
        unsigned int lo, hi;
        __asm__ __volatile__("lfence\nrdtsc" : "=a"(lo), "=d"(hi) :: "memory");
        return (static_cast<uint64_t>(hi) << 32) | lo;
#else
        return static_cast<uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
#endif
    }

    [[nodiscard]] double cycles_to_microseconds(uint64_t cycles) const noexcept
    {
        return static_cast<double>(cycles) / (cpu_frequency_ghz_ * 1000.0);
    }

    void record_received() noexcept
    {
        total_packets_received_.fetch_add(1, std::memory_order_relaxed);
    }

    void record_forwarded(uint64_t entry_timestamp) noexcept
    {
        total_packets_forwarded_.fetch_add(1, std::memory_order_relaxed);
        const uint64_t current_tsc = get_rdtsc();
        if (current_tsc >= entry_timestamp) {
            add_latency_cycles(current_tsc - entry_timestamp);
        }
    }

    void record_malformed_drop() noexcept
    {
        malformed_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
        total_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
        record_received();
    }

    void record_buffer_drop() noexcept
    {
        buffer_exhaustion_dropped_.fetch_add(1, std::memory_order_relaxed);
        total_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] double get_p999_latency_us() const noexcept
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        uint64_t total = 0;
        for (const auto count : latency_buckets_) {
            total += count;
        }
        if (total == 0) {
            return 0.0;
        }
        const uint64_t target = (total * 999 + 999) / 1000;
        uint64_t seen = 0;
        for (std::size_t i = 0; i < latency_buckets_.size(); ++i) {
            seen += latency_buckets_[i];
            if (seen >= target) {
                return cycles_to_microseconds(uint64_t{1} << i);
            }
        }
        return 0.0;
    }

    void print_report() const
    {
        std::printf("\n================ SWITCH PERFORMANCE REPORT ================\n");
        std::printf("Total Packets Received:      %llu\n",
            static_cast<unsigned long long>(total_packets_received_.load()));
        std::printf("Total Packets Forwarded:     %llu\n",
            static_cast<unsigned long long>(total_packets_forwarded_.load()));
        std::printf("Total Packets Dropped:       %llu\n",
            static_cast<unsigned long long>(total_packets_dropped_.load()));
        std::printf("  - Malformed/Checksum Drops: %llu\n",
            static_cast<unsigned long long>(malformed_packets_dropped_.load()));
        std::printf("  - Buffer Exhaustion Drops:  %llu\n",
            static_cast<unsigned long long>(buffer_exhaustion_dropped_.load()));
        std::printf("p99.9 Forwarding Latency:    %.3f us (Target: < 50 us)\n",
            get_p999_latency_us());
        std::printf("===========================================================\n");
    }
};

}
