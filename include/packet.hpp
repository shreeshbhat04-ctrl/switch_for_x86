#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace switchmodel{
constexpr std::size_t min_frame_sz = 64;
constexpr std::size_t max_frame_sz = 1518;
constexpr std::size_t control_que = 7;

struct packetmetadata {
    uint32_t src_ip{0};
    uint32_t dst_ip{0};
    uint32_t vlan_id{0};
    uint32_t ttl{0};
    uint8_t ingress_port{0};
    uint8_t egress_port{0};
    bool has_vlan{false};
    bool is_corrupted{false};
    uint64_t entry_timestamp{0};
};

struct packet {
    std::array<uint8_t, min_frame_sz> data{};
    std::size_t length{min_frame_sz};
    packetmetadata metadata;
};
}

// class packet_generator {
// public:
//     static constexpr std::size_t port_count = 4;
//     static constexpr uint64_t line_rate_bps = 10'000'000'000ULL;
//     static constexpr uint64_t wire_bits_per_frame = (min_frame_sz + 20) * 8;

//     packet next(std::size_t port)
//     {
//         if (port >= port_count) {
//             throw std::out_of_range("packet generator port must be between 0 and 3");
//         }

//         const uint64_t sequence = sequences_[port]++;
//         packet result;
//         result.metadata.ingress_port = static_cast<uint8_t>(port);
//         result.metadata.entry_timestamp =
//             sequence * wire_bits_per_frame * 1'000'000'000ULL / line_rate_bps;

//         result.data[0] = static_cast<uint8_t>(port);
//         for (std::size_t byte = 0; byte < sizeof(sequence); ++byte) {
//             result.data[byte + 1] = static_cast<uint8_t>(sequence >> (byte * 8));
//         }
//         for (std::size_t byte = sizeof(sequence) + 1; byte < result.data.size(); ++byte) {
//             result.data[byte] = static_cast<uint8_t>(port + byte);
//         }
//         return result;
//     }

//     void reset()
//     {
//         sequences_.fill(0);
//     }

// private:
//     std::array<uint64_t, port_count> sequences_{};
// };
// }