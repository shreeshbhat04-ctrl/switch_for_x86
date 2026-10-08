#include "packet.hpp"
#include "buffer.hpp"
#include "parser.hpp"
#include "ring.hpp"
#include "scheduler.hpp"
#include "stats.hpp"
#include "utils.hpp"
#include "lpm.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

int main()
{
    switchmodel::packet_generator generator;
    const auto first = generator.next(2);
    const auto second = generator.next(2);
    const auto other_port = generator.next(3);
    assert(first.length == switchmodel::min_frame_sz);
    assert(first.metadata.ingress_port == 2);
    assert(first.metadata.entry_timestamp == 0);
    assert(first.data[0] == 2);
    assert(first.data[1] == 0);
    assert(second.metadata.ingress_port == 2);
    assert(second.metadata.entry_timestamp == 67);
    assert(second.data[1] == 1);
    assert(other_port.metadata.ingress_port == 3);
    assert(other_port.metadata.entry_timestamp == 0);
    assert(switchmodel::packet_generator::port_count == 4);
    assert(switchmodel::packet_generator::line_rate_bps == 10'000'000'000ULL);
    assert(switchmodel::packet_generator::wire_bits_per_frame == 672);
    generator.reset();
    assert(generator.next(2).data[1] == 0);
    bool rejected_invalid_port = false;
    try {
        generator.next(4);
    } catch (const std::out_of_range&) {
        rejected_invalid_port = true;
    }
    assert(rejected_invalid_port);

    switchmodel::packet parsed;
    parsed.length = 34;
    const uint8_t frame[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x00,
        0x45, 0, 0, 20, 0, 0, 0, 0, 64, 64, 0x8e, 0x73,
        192, 0, 2, 1, 198, 51, 100, 2
    };
    std::memcpy(parsed.data.data(), frame, sizeof(frame));
    assert(switchmodel::parser::parse(parsed));
    assert(parsed.metadata.dst_ip == 0xC6336402U);

    Spscringbuffer<int, 4> ring;
    assert(ring.push(1));
    int value = 0;
    assert(ring.pop(value));
    assert(value == 1);
    assert(ring.size() == 0);

    switchmodel::egressbufferpool pool;
    auto* block = pool.allocate();
    assert(block != nullptr);
    pool.deallocate(block);
    assert(pool.get_alloc_bytes() == 0);

    switchmodel::PerformanceStats stats;
    stats.record_forwarded(switchmodel::PerformanceStats::get_rdtsc() - 3000);
    assert(stats.get_p999_latency_us() > 0.0);

    switchmodel::lpmtable routes;
    routes.insert(0, 0, 0);
    routes.insert(0x0A000000, 8, 2);
    routes.insert(0x0A010000, 16, 3);
    assert(routes.lookup(0x0A010001) == 3);
    assert(routes.lookup(0x0A020001) == 2);
    assert(routes.lookup(0xC0000201) == 0);
    assert(routes.get_nod_cnt() > 0);
    routes.clear();
    assert(routes.lookup(0x0A010001) == -1);
}