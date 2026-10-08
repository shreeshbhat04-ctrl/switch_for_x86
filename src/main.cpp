#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#ifndef _WIN32
#include <pthread.h>
#endif

#include "packet.hpp"
#include "ring.hpp"
#include "parser.hpp"
#include "lpm.hpp"
#include "buffer.hpp"
#include "scheduler.hpp"
#include "stats.hpp"
#include "utils.hpp"

void pin_thread_to_core(std::thread& th,int c_id){
#ifndef _WIN32
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(c_id,&cpuset);
    int rc=pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset);
    if(rc!=0){
        std::cerr<<"Warning: failed in pin_thread_to_core()"<<c_id<<"\n";
    }
#endif
  
    
}

using namespace switchmodel;


int main(){
    std::cout<<"Starting 4-Port 10 GbE L3 Switch ...\n";
    egressbufferpool buffer_pool;
    lpmtable lpm_table;
    PerformanceStats stats(3.0);
    // Populate route table with sample entries (e.g., 500K routes simulation subset)
    lpm_table.insert(0xC0A80100, 24, 1); // 192.168.1.0/24 -> Port 1
    lpm_table.insert(0x0A000000, 8, 2);  // 10.0.0.0/8 -> Port 2
    lpm_table.insert(0x00000000, 0, 0);  // Default route -> Port 0
    //spsc ring buff (ingress->parse->lookup->scheduler->egress)
    constexpr size_t ring_capacity=4096;
    Spscringbuffer<packet,ring_capacity>rx_to_parse_ring;
    Spscringbuffer<packet,ring_capacity>parse_to_lookup_ring;

    std::atomic<bool>running{true};
    // spawn pipeline threads to core
    // core 0:traffic ingress /generator driver
    std::thread ingress_th([&](){
     uint64_t pckt_to_inject =1000000;
     for (uint64_t i = 0; i < pckt_to_inject && running; i++)
     {
        packet pkt;
        pkt.length=64; //worst case 64b frame
        pkt.metadata.ingress_port=static_cast<uint8_t>(i%4);
        pkt.metadata.entry_timestamp=PerformanceStats::get_rdtsc();
        //mock ipv4 
        pkt.data[12]=0x08;pkt.data[13]=0x00;
        uint32_t dst_ip=switchmodel::ntohl(0xC0A80132);
        std::memcpy(&pkt.data[30],&dst_ip,4);
        stats.record_received();
        while(!rx_to_parse_ring.push(std::move(pkt))&& running){
            std::this_thread::yield();
        }

     }

     std::cout<<"[Ingress] injection complete.\n";
    });
    pin_thread_to_core(ingress_th,0);
    //core 1:parse stage
    std::thread parser_th([&](){
     packet pkt;
     while(running){
        if(rx_to_parse_ring.pop(pkt)){
            bool ok= parser::parse(pkt);
            if(!ok){
                stats.record_malformed_drop();
                continue;
            }
            while(!parse_to_lookup_ring.push(std::move(pkt))&&running){
                std::this_thread::yield();
            }
        }else{
            std::this_thread::yield();
        }
     }
     
    });
    pin_thread_to_core(parser_th,1);
    //core 2:lookup & scheduling stage
    std::thread processing_th([&](){
        packet pkt;
        egressscheduler scheduler;

    while(running){
       if(parse_to_lookup_ring.pop(pkt)){
        //lookup
        int egress_port=lpm_table.lookup(pkt.metadata.dst_ip);
        pkt.metadata.egress_port=(egress_port>=0)?static_cast<uint8_t>(egress_port):0;
        // bufferblock 12mb
        bufferblock* block=buffer_pool.allocate();
        if(!block){
            stats.record_buffer_drop();
            continue;
        }
        //cpy packet to bufffer block and push to schedule
        block->length=pkt.length;
        block->metadata=pkt.metadata;
        std::memcpy(block->data.data(),pkt.data.data(), pkt.length);

        if(!scheduler.enqueue(block)){
            buffer_pool.deallocate(block);
            stats.record_buffer_drop();
            continue;
        }
        //simulate egress transmission and deque
        bufferblock* tx_block=scheduler.schedule();
        if(tx_block){
            stats.record_forwarded(tx_block->metadata.entry_timestamp);
            buffer_pool.deallocate(tx_block);
        }

       }else{
        std::this_thread::yield();
       }
    }
        });
    // 3 seconds timeout
    pin_thread_to_core(processing_th,2);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    running=false;
    //join
    if(ingress_th.joinable())ingress_th.join();
    if(parser_th.joinable())parser_th.join();
    if(processing_th.joinable())processing_th.join();


    stats.print_report();
    return 0;
}
