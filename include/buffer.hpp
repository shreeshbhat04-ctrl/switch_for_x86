#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <atomic>
#include <memory>
#include "packet.hpp"

namespace switchmodel{
    constexpr size_t total_buffer_bytes = 12*1024*1024; //12Mb shared egress
    constexpr size_t block_sz=2048;
    constexpr size_t num_blocks=total_buffer_bytes/block_sz;
    //fixed-sz memory block
    struct bufferblock
    {
        std::array<uint8_t,block_sz>data;
        size_t length{0};
        packetmetadata metadata;
    };
    class egressbufferpool{
      private:
      // caching the contiguos memory
      std::vector<bufferblock>blocks;
      // buffer indices lock free
      std::unique_ptr<std::atomic<int32_t>[]>estack;
      std::atomic<int32_t>etop{-1};
      //track cur allocated byte
      std::atomic<size_t>curr_abyte{0};
      public:
      egressbufferpool(){
        blocks.resize(num_blocks);
        estack=std::make_unique<std::atomic<int32_t>[]>(num_blocks);
        for (size_t i = 0; i < num_blocks; i++)
        {
            estack[i].store(static_cast<int32_t>(i),std::memory_order_relaxed);

        }
        etop.store(static_cast<int32_t>(num_blocks-1),std::memory_order_release); 
      }
      //disable copying
      egressbufferpool(const egressbufferpool&)=delete;
      egressbufferpool& operator=(const egressbufferpool&)=delete;


      bufferblock* allocate()noexcept{
       int32_t cur_top=etop.load(std::memory_order_relaxed);
        while(cur_top>=0){
            if(etop.compare_exchange_weak(cur_top,cur_top-1,std::memory_order_acquire,std::memory_order_relaxed)){
              int32_t idx=estack[cur_top].load(std::memory_order_relaxed);
              curr_abyte.fetch_add(block_sz,std::memory_order_relaxed);
                return &blocks[idx];
            }
        }
        return nullptr;//pckt dropped
      }
     void deallocate(bufferblock* block)noexcept{
        if(!block)return;

        ptrdiff_t idx=block-blocks.data();

        int32_t cur_top=etop.load(std::memory_order_relaxed);
        while(1){
           if(etop.compare_exchange_weak(cur_top,cur_top+1,std::memory_order_release,std::memory_order_relaxed)){
            estack[cur_top+1].store(static_cast<int32_t>(idx),std::memory_order_relaxed);
            curr_abyte.fetch_sub(block_sz,std::memory_order_relaxed);
            break;
           }
        }
     }

     [[nodiscard]] size_t get_alloc_bytes()const noexcept{
        return curr_abyte.load(std::memory_order_relaxed);
     }
     [[nodiscard]] bool is_exhaust()const noexcept{
        return etop.load(std::memory_order_relaxed)<0;
     }

    };
    
}