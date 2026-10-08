#pragma once

#include "packet.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace tb {
inline uint16_t ip_checksum(const uint8_t* header, std::size_t length)
{
    uint32_t sum = 0;
    for (std::size_t i = 0; i + 1 < length; i += 2) {
        sum += (uint32_t(header[i]) << 8) | header[i + 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

struct FrameOpts {
    bool vlan{false};
    uint16_t vlan_id{0x123};
    uint8_t ttl{64};
    uint8_t version{4};
    uint8_t ihl_words{5};
    bool bad_checksum{false};
    int payload{26};
    uint32_t src{0xC0000201};
    uint32_t dst{0xC6336402};
    int total_len_delta{0};
};

inline std::vector<uint8_t> make_frame(const FrameOpts& options)
{
    std::vector<uint8_t> frame(12, 0);
    frame[0] = 0x02;
    frame[6] = 0x04;
    auto put16 = [&](uint16_t value) {
        frame.push_back(static_cast<uint8_t>(value >> 8));
        frame.push_back(static_cast<uint8_t>(value));
    };
    if (options.vlan) {
        put16(0x8100);
        put16(options.vlan_id & 0x0FFF);
    }
    put16(0x0800);

    const std::size_t ip_offset = frame.size();
    const std::size_t header_length = std::size_t(options.ihl_words) * 4;
    frame.resize(ip_offset + header_length + options.payload, 0);
    frame[ip_offset] = static_cast<uint8_t>(
        (options.version << 4) | options.ihl_words);
    const uint16_t total_length = static_cast<uint16_t>(
        header_length + options.payload + options.total_len_delta);
    frame[ip_offset + 2] = static_cast<uint8_t>(total_length >> 8);
    frame[ip_offset + 3] = static_cast<uint8_t>(total_length);
    frame[ip_offset + 8] = options.ttl;
    frame[ip_offset + 9] = 17;
    for (int i = 0; i < 4; ++i) {
        frame[ip_offset + 12 + i] =
            static_cast<uint8_t>(options.src >> (24 - 8 * i));
        frame[ip_offset + 16 + i] =
            static_cast<uint8_t>(options.dst >> (24 - 8 * i));
    }
    uint16_t checksum = ip_checksum(frame.data() + ip_offset, header_length);
    if (options.bad_checksum) {
        checksum ^= 0x1234;
    }
    frame[ip_offset + 10] = static_cast<uint8_t>(checksum >> 8);
    frame[ip_offset + 11] = static_cast<uint8_t>(checksum);
    return frame;
}

inline switchmodel::packet to_packet(const std::vector<uint8_t>& frame)
{
    switchmodel::packet packet;
    std::memcpy(packet.data.data(), frame.data(), frame.size());
    packet.length = frame.size();
    return packet;
}
}
