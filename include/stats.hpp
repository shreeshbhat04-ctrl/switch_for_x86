#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <algorithm>
#include <chrono>
namespace switchmodel {
class PerformanceStats {
private:
    std::atomic<uint64_t> total_packets_received_{0};
    std::atomic<uint64_t> total_packets_forwarded_{0};
    std::atomic<uint64_t> total_packets_dropped_{0};
    std::atomic<uint64_t> malformed_packets_dropped_{0};
    std::atomic<uint64_t> buffer_exhaustion_dropped_{0};

    std::vector<uint64_t> latency_samples_;
    double cpu_frequency_ghz_{3.0}; // Assumed 3.0 GHz CPU frequency for rdtsc conversion

public:
    PerformanceStats(double cpu_ghz = 3.0) : cpu_frequency_ghz_(cpu_ghz) {
        latency_samples_.reserve(100000); // Pre-allocate storage to avoid runtime allocations
    }

    // High-resolution timestamp reader using CPU cycle counter (rdtsc)
    [[nodiscard]] static inline uint64_t get_rdtsc() noexcept {
#if defined(_MSC_VER)
        return __rdtsc();
#elif defined(__x86_64__) || defined(__i386__)
        unsigned int lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return ((uint64_t)hi << 32) | lo;
#else
        // Fallback to high_resolution_clock if rdtsc is unavailable on architecture (e.g. ARM)
        return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
    }

    // Convert CPU cycles to microseconds
    [[nodiscard]] double cycles_to_microseconds(uint64_t cycles) const noexcept {
        return static_cast<double>(cycles) / (cpu_frequency_ghz_ * 1000.0);
    }

    void record_received() noexcept {
        total_packets_received_.fetch_add(1, std::memory_order_relaxed);
    }

    void record_forwarded(uint64_t entry_timestamp) noexcept {
        total_packets_forwarded_.fetch_add(1, std::memory_order_relaxed);
        uint64_t current_tsc = get_rdtsc();
        if (current_tsc > entry_timestamp) {
            uint64_t elapsed_cycles = current_tsc - entry_timestamp;
            // Store cycle diff safely 
        }
    }

    void record_malformed_drop() noexcept {
        malformed_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
        total_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
        total_packets_received_.fetch_add(1, std::memory_order_relaxed);
    }

    void record_buffer_drop() noexcept {
        buffer_exhaustion_dropped_.fetch_add(1, std::memory_order_relaxed);
        total_packets_dropped_.fetch_add(1, std::memory_order_relaxed);
    }

    // Add latency sample in microseconds
    void add_latency_sample_us(double latency_us) {
        latency_samples_.push_back(static_cast<uint64_t>(latency_us * 1000.0)); // store in nanoseconds for precision
    }

    // Calculate p99.9 latency percentile
    [[nodiscard]] double get_p999_latency_us() {
        if (latency_samples_.empty()) return 0.0;

        std::sort(latency_samples_.begin(), latency_samples_.end());
        size_t index = static_cast<size_t>(0.999 * latency_samples_.size());
        if (index >= latency_samples_.size()) {
            index = latency_samples_.size() - 1;
        }

        return static_cast<double>(latency_samples_[index]) / 1000.0; // convert back to microseconds
    }

    // Print summary report
    void print_report() const {
        uint64_t rx = total_packets_received_.load(std::memory_order_relaxed);
        uint64_t fwd = total_packets_forwarded_.load(std::memory_order_relaxed);
        uint64_t dropped = total_packets_dropped_.load(std::memory_order_relaxed);

        // Note: const cast used just for sorting percentile calculation in report
        double p999 = const_cast<PerformanceStats*>(this)->get_p999_latency_us();

        std::printf("\n================ SWITCH PERFORMANCE REPORT ================\n");
        std::printf("Total Packets Received:      %llu\n", static_cast<unsigned long long>(rx));
        std::printf("Total Packets Forwarded:     %llu\n", static_cast<unsigned long long>(fwd));
        std::printf("Total Packets Dropped:       %llu\n", static_cast<unsigned long long>(dropped));
        std::printf("  - Malformed/Checksum Drops: %llu\n", static_cast<unsigned long long>(malformed_packets_dropped_.load()));
        std::printf("  - Buffer Exhaustion Drops:  %llu\n", static_cast<unsigned long long>(buffer_exhaustion_dropped_.load()));
        std::printf("p99.9 Forwarding Latency:    %.3f µs (Target: < 50 µs)\n", p999);
        std::printf("===========================================================\n");
    }
};

} 