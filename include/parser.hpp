#pragma once
#include "packet.hpp"
#include "utils.hpp"
#include <cstring>
#include <cstddef>
namespace switchmodel {
class parser {
public:
 static bool parse(packet& packets){
    packets.metadata.is_corrupted = false;
    packets.metadata.is_expired = false;
    if(packets.length < 14 || packets.length > packets.data.size()){
        packets.metadata.is_corrupted=true;
        return false;
    }
    const uint8_t* data=packets.data.data();
    size_t offset=0;
    // skiping dst and src (6 byt + 6byt) =12
    offset+=12;
    // 802.1Q Vlan tag(ETHER type=0x8100)
    uint16_t ethertype = 0;
    std::memcpy(&ethertype,data+offset,2);
    ethertype=switchmodel::ntohs(ethertype);
    offset+=2;

    if(ethertype==0x8100){
        if(packets.length<offset+4){
            packets.metadata.is_corrupted=true;
            return false;
        }
    

        uint16_t vlan_tci=0;
        std::memcpy(&vlan_tci,data+offset,2);
        vlan_tci = switchmodel::ntohs(vlan_tci);

        packets.metadata.has_vlan =true;
        packets.metadata.vlan_id =vlan_tci & 0x0FFF;// extract lower 12 bits
        offset+=2;
        //read ethertype following vlan
        std::memcpy(&ethertype,data+offset,2);
        ethertype=switchmodel::ntohs(ethertype);
        offset+=2;
    }

    // verify if pckt is ipv4 (ethertype = 0x0800)
    if(ethertype!=0x0800){
        packets.metadata.is_corrupted=true;
        return false;
    }
    //parse ipv4
    if(packets.length <offset+20){ //minimum ipv4 head header length 20
        packets.metadata.is_corrupted =true;
        return false;
    }
    
    uint8_t ver_ihl =data[offset];
    if ((ver_ihl >> 4) != 4) {
      packets.metadata.is_corrupted = true;
      return false;
    }
    uint8_t ihl =(ver_ihl & 0x0F)*4;

    if(ihl < 20 || packets.length < offset+ihl){
      packets.metadata.is_corrupted =true;
      return false;
    }
    uint16_t total_length = 0;
    std::memcpy(&total_length, data + offset + 2, sizeof(total_length));
    total_length = switchmodel::ntohs(total_length);
    if (total_length < ihl || total_length > packets.length - offset) {
      packets.metadata.is_corrupted = true;
      return false;
    }
    uint32_t checksum = 0;
    for (std::size_t i = 0; i < ihl; i += 2) {
      uint16_t word = 0;
      std::memcpy(&word, data + offset + i, sizeof(word));
      checksum += switchmodel::ntohs(word);
      checksum = (checksum & 0xFFFFU) + (checksum >> 16);
    }
    if ((checksum & 0xFFFFU) != 0xFFFFU) {
      packets.metadata.is_corrupted = true;
      return false;
    }
    // ttl extraction
    uint8_t ttl =data[offset+8];
    if(ttl == 0){
        packets.metadata.is_expired = true;
        return false;
    }
    packets.metadata.ttl= ttl;

    // extract src ip
    uint32_t src_ip = 0;
    std::memcpy(&src_ip,data+offset+12,4);
    packets.metadata.src_ip =switchmodel::ntohl(src_ip);

    //extract dst ip
    uint32_t dst_ip = 0;
    std::memcpy(&dst_ip,data+offset+16,4);
    packets.metadata.dst_ip =switchmodel::ntohl(dst_ip);

    packets.metadata.is_corrupted =false;
    return true;
    }
};
}
