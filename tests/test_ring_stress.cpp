#include "ring.hpp"
#include "common.hpp"

#include <cstdint>
#include <cstdlib>
#include <thread>

struct RingItem { uint64_t sequence; uint64_t inverse; uint64_t padding[6]; };

template <std::size_t Capacity>
static void run_ring(uint64_t count)
{
    auto* ring = new Spscringbuffer<RingItem, Capacity>();
    uint64_t errors = 0;
    std::thread producer([&] {
        for (uint64_t i = 0; i < count; ++i) {
            RingItem item{i, ~i, {}};
            while (!ring->push(item)) std::this_thread::yield();
        }
    });
    std::thread consumer([&] {
        RingItem item{};
        uint64_t expected = 0;
        while (expected < count) {
            if (ring->pop(item)) {
                if (item.sequence != expected || item.inverse != ~expected) ++errors;
                ++expected;
            } else {
                std::this_thread::yield();
            }
        }
    });
    producer.join();
    consumer.join();
    CHECK(errors == 0);
    CHECK(ring->empty());
    CHECK(ring->size() == 0);
    delete ring;
}

int main(int argc, char** argv)
{
    const uint64_t count = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 10000;
    run_ring<2>(count / 4);
    run_ring<8>(count);
    run_ring<1024>(count);
    return report();
}
