#pragma once

#include <vector>
#include <queue>
#include <cstdint>
#include <algorithm>
#include "buffer.hpp"
namespace switchmodel{
    struct egressqueue{
        std::queue<bufferblock*>blocks;
        int weight{0};
        int deficit{0};
        size_t max_pckt{1000};
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

        uint8_t targ_que=block->metadata.is_corrupted?0:block->metadata.egress_port;

        int8_t q_idx =(block->metadata.ingress_port == 0)?control_que:(block->metadata.src_ip%7);
        if(q_idx>7)q_idx=0;
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
       size_t fl=0;
       while(fl<7){
        egressqueue& q1=q[cur_drr_idx];
        if(!q1.blocks.empty()){
            // add quantum (weight*mtu)
            q1.deficit+=q1.weight*100;
            bufferblock* head_blk=q1.blocks.front();
            size_t pkt_sz=head_blk->length;

            if(pkt_sz<=static_cast<size_t>(q1.deficit)){
                // dedcut credit
                q1.deficit-=static_cast<int>(pkt_sz);
                q1.blocks.pop();
                //advcan rr idx for schedul pass
                cur_drr_idx=(cur_drr_idx+1)%7;
                return  head_blk;
            }else{
                //pass
            }
        }else{
            //rst the que if empty
            q1.deficit=0;
        }
        cur_drr_idx=(cur_drr_idx+1)%7;
        fl++;
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
