#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#if defined(__linux__)
#include <cerrno>
#include <sched.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace switchmodel {

inline uint64_t benchmark_tsc() noexcept
{
#if defined(_MSC_VER)
    _ReadWriteBarrier();
    _mm_lfence();
    const uint64_t value = __rdtsc();
    _mm_lfence();
    _ReadWriteBarrier();
    return value;
#elif defined(__i386__) || defined(__x86_64__)
    unsigned int low = 0;
    unsigned int high = 0;
    __asm__ __volatile__(
        "lfence\n\t"
        "rdtsc\n\t"
        "lfence"
        : "=a"(low), "=d"(high)
        :
        : "memory");
    return (static_cast<uint64_t>(high) << 32) | low;
#else
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
#endif
}

inline uint64_t round_nanoseconds(long double nanoseconds)
{
    if (!std::isfinite(nanoseconds) || nanoseconds < 0.0L) {
        throw std::overflow_error("calibrated nanoseconds are not representable");
    }

    const long double limit = std::ldexp(1.0L, 64);
    const long double maximum =
        static_cast<long double>(std::numeric_limits<uint64_t>::max());
    const long double rounded = std::floor(nanoseconds + 0.5L);
    if (rounded >= limit || rounded > maximum) {
        throw std::overflow_error("calibrated nanoseconds exceed uint64_t");
    }
    return static_cast<uint64_t>(rounded);
}

inline uint64_t calibrated_nanoseconds(uint64_t ticks, double ticks_per_microsecond)
{
    if (!std::isfinite(ticks_per_microsecond) || ticks_per_microsecond <= 0.0) {
        throw std::invalid_argument("ticks-per-microsecond calibration must be positive");
    }
    const long double nanoseconds =
        static_cast<long double>(ticks) /
        static_cast<long double>(ticks_per_microsecond) * 1'000'000.0L;
    return round_nanoseconds(nanoseconds);
}

struct tsc_calibration {
    double ticks_per_nanosecond;

    explicit tsc_calibration(double measured_ticks_per_nanosecond)
        : ticks_per_nanosecond(measured_ticks_per_nanosecond)
    {
        if (!std::isfinite(ticks_per_nanosecond) ||
            ticks_per_nanosecond <= 0.0) {
            throw std::invalid_argument(
                "ticks-per-nanosecond calibration must be positive");
        }
    }

    uint64_t tsc_to_nanoseconds(uint64_t ticks) const
    {
        const long double nanoseconds =
            static_cast<long double>(ticks) /
            static_cast<long double>(ticks_per_nanosecond);
        return round_nanoseconds(nanoseconds);
    }
};

inline tsc_calibration calibrate_tsc(std::chrono::milliseconds interval)
{
    if (interval.count() <= 0) {
        throw std::invalid_argument("TSC calibration interval must be positive");
    }

    const auto start_time = std::chrono::steady_clock::now();
    const uint64_t start_tsc = benchmark_tsc();
    std::this_thread::sleep_for(interval);
    const auto end_time = std::chrono::steady_clock::now();
    const uint64_t end_tsc = benchmark_tsc();

    const auto elapsed_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time).count();
    if (elapsed_ns <= 0 || end_tsc <= start_tsc) {
        throw std::runtime_error("TSC calibration produced an invalid measurement");
    }
    return tsc_calibration(
        static_cast<double>(end_tsc - start_tsc) /
        static_cast<double>(elapsed_ns));
}

inline uint64_t tsc_to_nanoseconds(
    uint64_t ticks, double ticks_per_nanosecond)
{
    return tsc_calibration(ticks_per_nanosecond).tsc_to_nanoseconds(ticks);
}

inline uint64_t percentile(std::vector<uint64_t> samples, double fraction)
{
    if (samples.empty()) {
        throw std::invalid_argument("percentile requires at least one sample");
    }
    if (!std::isfinite(fraction) || fraction < 0.0 || fraction > 1.0) {
        throw std::invalid_argument("percentile must be between zero and one");
    }

    std::sort(samples.begin(), samples.end());
    const std::size_t rank = static_cast<std::size_t>(
        std::ceil(fraction * static_cast<double>(samples.size())));
    const std::size_t index = rank == 0 ? 0 : rank - 1;
    return samples[index];
}

struct benchmark_counters {
    uint64_t attempted = 0;
    uint64_t completed = 0;
    uint64_t dropped = 0;
    uint64_t ring_full_attempts = 0;
    uint64_t warmup_completed = 0;
};

enum class affinity_status {
    success,
    unsupported,
    error
};

struct affinity_result {
    affinity_status status;
    int error_code;

    bool ok() const noexcept { return status == affinity_status::success; }
};

inline affinity_result set_thread_affinity(const std::vector<unsigned>& cores)
{
    if (cores.empty()) {
        return {affinity_status::error, 0};
    }

#if defined(__linux__)
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    for (const unsigned core : cores) {
        if (core >= CPU_SETSIZE) {
            return {affinity_status::error, EINVAL};
        }
        CPU_SET(core, &cpu_set);
    }
    if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) != 0) {
        return {affinity_status::error, errno};
    }
    return {affinity_status::success, 0};
#else
    (void)cores;
    return {affinity_status::unsupported, 0};
#endif
}

} // namespace switchmodel
