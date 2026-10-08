#include "parser.hpp"
#include "frame_builder.hpp"
#include "common.hpp"

#include <cstring>
#include <random>

using namespace switchmodel;

static bool parse(const std::vector<uint8_t>& frame, packet& value)
{
    value = tb::to_packet(frame);
    return parser::parse(value);
}

int main()
{
    packet value;
    { tb::FrameOpts options; CHECK(parse(tb::make_frame(options), value)); CHECK(value.metadata.src_ip == 0xC0000201); CHECK(value.metadata.dst_ip == 0xC6336402); CHECK(value.metadata.ttl == 64); CHECK(!value.metadata.has_vlan); }
    { tb::FrameOpts options; options.vlan = true; options.vlan_id = 0xABC; CHECK(parse(tb::make_frame(options), value)); CHECK(value.metadata.has_vlan); CHECK(value.metadata.vlan_id == 0xABC); }
    { tb::FrameOpts options; options.bad_checksum = true; CHECK(!parse(tb::make_frame(options), value)); CHECK(value.metadata.is_corrupted); }
    { tb::FrameOpts options; options.ttl = 0; CHECK(!parse(tb::make_frame(options), value)); CHECK(value.metadata.is_expired); CHECK(!value.metadata.is_corrupted); }
    { tb::FrameOpts options; options.version = 6; CHECK(!parse(tb::make_frame(options), value)); }
    { tb::FrameOpts options; options.ihl_words = 4; CHECK(!parse(tb::make_frame(options), value)); }
    { tb::FrameOpts options; options.total_len_delta = 40; CHECK(!parse(tb::make_frame(options), value)); }
    { tb::FrameOpts options; options.ihl_words = 7; CHECK(parse(tb::make_frame(options), value)); }
    { auto frame = tb::make_frame({}); frame[12] = 0x86; frame[13] = 0xDD; CHECK(!parse(frame, value)); }
    { packet big; big.length = big.data.size() + 1; CHECK(!parser::parse(big)); }

    for (bool vlan : {false, true}) {
        tb::FrameOpts options;
        options.vlan = vlan;
        auto frame = tb::make_frame(options);
        for (std::size_t length = 0; length < frame.size(); ++length) {
            packet truncated;
            truncated.data.fill(0xAA);
            std::memcpy(truncated.data.data(), frame.data(), length);
            truncated.length = length;
            CHECKF(!parser::parse(truncated), "truncated frame len=%zu", length);
        }
    }
    { tb::FrameOpts first; first.vlan = true; parse(tb::make_frame(first), value); auto frame = tb::make_frame({}); std::memcpy(value.data.data(), frame.data(), frame.size()); value.length = frame.size(); CHECK(parser::parse(value)); CHECK(!value.metadata.has_vlan); }

    std::mt19937 rng(7);
    for (int i = 0; i < 30000; ++i) {
        packet random_packet;
        for (auto& byte : random_packet.data) byte = static_cast<uint8_t>(rng());
        random_packet.length = rng() % 100;
        CHECK(!parser::parse(random_packet) || random_packet.length >= 34);
    }
    return report();
}
