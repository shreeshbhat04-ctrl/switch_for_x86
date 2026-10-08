#include "scheduler.hpp"
#include "common.hpp"

#include <vector>

using namespace switchmodel;

int main()
{
    std::vector<bufferblock> blocks(8000);
    std::size_t next = 0;
    auto make_block = [&](uint8_t port, std::size_t length, bool control) {
        auto* block = &blocks[next++];
        block->length = length;
        block->metadata.egress_port = port;
        block->metadata.is_control = control;
        return block;
    };

    egressscheduler scheduler;
    for (int queue = 0; queue < 7; ++queue) {
        for (int i = 0; i < 1000; ++i) {
            CHECK(scheduler.enqueue(make_block(static_cast<uint8_t>(queue), 64, false)));
        }
    }
    const int dequeue_count = 2000;
    long counts[7] = {};
    int actual_count = 0;
    for (int i = 0; i < dequeue_count; ++i) {
        auto* block = scheduler.schedule();
        if (block) {
            ++counts[block->metadata.egress_port % 7];
            ++actual_count;
        }
    }
    CHECK(actual_count == dequeue_count);
    for (int queue = 0; queue < 7; ++queue) {
        const double expected = dequeue_count * (queue + 1) / 28.0;
        CHECK(counts[queue] > expected * 0.80 &&
              counts[queue] < expected * 1.20);
    }
    return report();
}
