#include "packet.hpp"

#include <cassert>
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
}