#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include "packet.hpp"

namespace switchmodel{
    constexpr size_t total_buffer_bytes = 12*1024*1024; //12Mb shared egress
    constexpr size_t block_sz=max_frame_sz;
    constexpr size_t num_blocks=total_buffer_bytes/block_sz;
    //fixed-sz memory block
    struct bufferblock
    {
        std::array<uint8_t, block_sz> data{};
        size_t length{0};
        packetmetadata metadata;
    };
    class egressbufferpool{
      private:
      // caching the contiguos memory
      std::vector<bufferblock>blocks;
      // buffer indices lock free
      std::vector<int32_t> free_indices;
      std::vector<bool> allocated;
      mutable std::mutex free_mutex;
      size_t allocated_bytes{0};
      public:
      egressbufferpool(){
        blocks.resize(num_blocks);
        free_indices.reserve(num_blocks);
        allocated.assign(num_blocks, false);
        for (size_t i = 0; i < num_blocks; i++)
        {
            free_indices.push_back(static_cast<int32_t>(i));
        }
      }
      //disable copying
      egressbufferpool(const egressbufferpool&)=delete;
      egressbufferpool& operator=(const egressbufferpool&)=delete;


      bufferblock* allocate() noexcept {
        std::lock_guard<std::mutex> lock(free_mutex);
        if (free_indices.empty()) {
            return nullptr;
        }
        const int32_t idx = free_indices.back();
        free_indices.pop_back();
        allocated[static_cast<size_t>(idx)] = true;
        allocated_bytes += block_sz;
        return &blocks[static_cast<size_t>(idx)];
      }
     void deallocate(bufferblock* block)noexcept{
        if(!block)return;

        ptrdiff_t idx=block-blocks.data();

        if (idx < 0 || static_cast<size_t>(idx) >= blocks.size()) {
            return;
        }
        std::lock_guard<std::mutex> lock(free_mutex);
        if (!allocated[static_cast<size_t>(idx)]) {
            return;
        }
        allocated[static_cast<size_t>(idx)] = false;
        free_indices.push_back(static_cast<int32_t>(idx));
        allocated_bytes -= block_sz;
     }

     [[nodiscard]] size_t get_alloc_bytes()const noexcept{
        std::lock_guard<std::mutex> lock(free_mutex);
        return allocated_bytes;
     }
     [[nodiscard]] bool is_exhaust()const noexcept{
        std::lock_guard<std::mutex> lock(free_mutex);
        return free_indices.empty();
     }

    };
    
}