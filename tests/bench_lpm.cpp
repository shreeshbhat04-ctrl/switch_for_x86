#include "lpm.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <thread>
#include <vector>

int main(int argc, char** argv)
{
    const int routes = argc > 1 ? std::atoi(argv[1]) : 5000;
    const int threads = argc > 2 ? std::atoi(argv[2]) : 1;
    const double ghz = argc > 3 ? std::atof(argv[3]) : 3.0;
    std::mt19937 rng(1);
    switchmodel::lpmtable table;
    for (int i = 0; i < routes; ++i) {
        const uint8_t length = static_cast<uint8_t>(16 + rng() % 9);
        table.insert(static_cast<uint32_t>(rng()) & (~0U << (32 - length)),
                     length, i % 4);
    }
    std::vector<uint32_t> ips(1U << 20);
    for (auto& ip : ips) ip = rng();
    const long iterations = 1000000;
    std::vector<double> ns(threads);
    std::vector<std::thread> workers;
    for (int thread = 0; thread < threads; ++thread) {
        workers.emplace_back([&, thread] {
            const auto start = std::chrono::steady_clock::now();
            long sink = 0;
            for (long i = 0; i < iterations; ++i) {
                sink += table.lookup(ips[(i + thread * 4099) & (ips.size() - 1)]);
            }
            (void)sink;
            ns[thread] = std::chrono::duration<double, std::nano>(
                std::chrono::steady_clock::now() - start).count() / iterations;
        });
    }
    for (auto& worker : workers) worker.join();
    double average = 0;
    for (double value : ns) average += value / threads;
    std::printf("threads=%d %.1f ns/lookup %.0f cycles @%.1fGHz nodes=%zu\n",
        threads, average, average * ghz, ghz, table.get_nod_cnt());
}
