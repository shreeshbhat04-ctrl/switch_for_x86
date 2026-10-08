#pragma once

#include <vector>
#include <queue>
#include <array>
#include <cstdint>
#include <algorithm>
#include "buffer.hpp"
namespace switchmodel{
    struct egressqueue{
        std::queue<bufferblock*>blocks;
        int weight{0};
        std::size_t deficit{0};
        size_t max_pckt{1000};
        bool fresh{false};
    };
    class egressscheduler{
      private:
      std::array<egressqueue,8>q;// 8 egress que:0-6 are drr,7 is strict priority
      size_t cur_drr_idx{0};//round robin ptr for que 0-6
      public:
      egressscheduler(){
        //assign defalt weigh to q 0-6
        for(int i=0;i<7;i++){
            q[i].weight=(i+1)*10;// 10,20,30...
        }
      }
      bool enqueue(bufferblock* block)noexcept{
        if(!block)return false;

        const std::size_t targ_que = block->metadata.is_corrupted
            ? 0 : (block->metadata.egress_port % 7);
        const std::size_t q_idx = block->metadata.is_control
            ? control_que : targ_que;
        if(q[q_idx].blocks.size()>=q[q_idx].max_pckt){
            return false;// tail drop
        }
        q[q_idx].blocks.push(block);
        return true;
      }
      bufferblock* schedule() noexcept{
        // strict priority check: if queue 7 not empty then always first
       if(!q[control_que].blocks.empty()){
        bufferblock* b=q[control_que].blocks.front();
        q[control_que].blocks.pop();
        return b;
       }
       for(size_t step=0; step<7*4; ++step){
        egressqueue& q1=q[cur_drr_idx];
        if(q1.blocks.empty()){
            q1.deficit=0;
            q1.fresh=false;
            cur_drr_idx=(cur_drr_idx+1)%7;
            continue;
        }
        if(!q1.fresh){
            q1.deficit += static_cast<std::size_t>(q1.weight) * 100;
            q1.fresh=true;
        }
        bufferblock* head_blk=q1.blocks.front();
        const size_t pkt_sz=head_blk->length;
        if (pkt_sz <= q1.deficit) {
            q1.deficit -= pkt_sz;
            q1.blocks.pop();
            if (q1.blocks.empty()) {
                q1.deficit=0;
                q1.fresh=false;
                cur_drr_idx=(cur_drr_idx+1)%7;
            }
            return head_blk;
        }
        q1.fresh=false;
        cur_drr_idx=(cur_drr_idx+1)%7;
       } 
       return nullptr;
      }


      [[nodiscard]]bool is_empty()const noexcept{
        for(const auto& a:q){
            if(!a.blocks.empty())return false;
        }
        return true;
      }
    };
} 
