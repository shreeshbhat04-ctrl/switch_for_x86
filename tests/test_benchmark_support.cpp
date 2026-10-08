#include "benchmark_support.hpp"
#include "common.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#if defined(__linux__)
#include <cerrno>
#include <sched.h>
#endif

using namespace switchmodel;

int main()
{
    CHECK(percentile({1, 2, 3, 4}, 0.50) == 2);
    CHECK(percentile({1, 2, 3, 4}, 0.999) == 4);
    CHECK(calibrated_nanoseconds(2'000, 2'000.0) == 1'000'000);
    CHECK(tsc_to_nanoseconds(2'000, 2.0) == 1'000);

    const auto maximum_nanoseconds =
        static_cast<long double>(std::numeric_limits<uint64_t>::max());
    try {
        CHECK(round_nanoseconds(maximum_nanoseconds) ==
              std::numeric_limits<uint64_t>::max());
    } catch (const std::overflow_error&) {
        // MSVC's long double aliases double, so UINT64_MAX may be represented
        // as 2^64 and must be rejected rather than wrapped by the cast.
    }

    bool boundary_threw = false;
    try {
        (void)round_nanoseconds(std::ldexp(1.0L, 64));
    } catch (const std::overflow_error&) {
        boundary_threw = true;
    }
    CHECK(boundary_threw);

    bool threw = false;
    try {
        (void)calibrated_nanoseconds(1, 0.0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)tsc_calibration(std::numeric_limits<double>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    try {
        (void)calibrated_nanoseconds(
            std::numeric_limits<uint64_t>::max(), 0.000001);
    } catch (const std::overflow_error&) {
        threw = true;
    }
    CHECK(threw);

    benchmark_counters counters;
    counters.ring_full_attempts = 3;
    CHECK(counters.ring_full_attempts == 3);

    const auto affinity = set_thread_affinity({0});
#if defined(__linux__)
    CHECK(affinity.status == affinity_status::success ||
          affinity.status == affinity_status::error);
    const auto invalid_affinity =
        set_thread_affinity({static_cast<unsigned>(CPU_SETSIZE)});
    CHECK(invalid_affinity.status == affinity_status::error);
    CHECK(invalid_affinity.error_code == EINVAL);
#else
    CHECK(affinity.status == affinity_status::unsupported);
#endif

    return report();
}
