#include "buffer.hpp"
#include "common.hpp"

#include <atomic>
#include <random>
#include <thread>
#include <vector>

using namespace switchmodel;

int main()
{
    egressbufferpool pool;
    std::atomic<long> clobbered{0};
    std::vector<std::thread> threads;
    for (int thread_id = 0; thread_id < 4; ++thread_id) {
        threads.emplace_back([&, thread_id] {
            std::mt19937 rng(thread_id + 1);
            std::vector<bufferblock*> held;
            for (int i = 0; i < 20000; ++i) {
                if (held.empty() || (rng() & 1)) {
                    if (auto* block = pool.allocate()) {
                        block->metadata.src_ip = static_cast<uint32_t>(thread_id + 1);
                        held.push_back(block);
                    }
                } else {
                    const std::size_t index = rng() % held.size();
                    if (held[index]->metadata.src_ip !=
                        static_cast<uint32_t>(thread_id + 1)) {
                        ++clobbered;
                    }
                    pool.deallocate(held[index]);
                    held[index] = held.back();
                    held.pop_back();
                }
            }
            for (auto* block : held) pool.deallocate(block);
        });
    }
    for (auto& thread : threads) thread.join();
    CHECK(clobbered.load() == 0);
    CHECK(pool.get_alloc_bytes() == 0);
    std::vector<bufferblock*> all;
    while (auto* block = pool.allocate()) all.push_back(block);
    CHECK(all.size() == num_blocks);
    for (auto* block : all) pool.deallocate(block);
    return report();
}
