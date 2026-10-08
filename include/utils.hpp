#pragma once
#include <cstdint>

namespace switchmodel {

#if defined(_WIN32) || defined(_MSC_VER)
inline constexpr bool host_is_little_endian = true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
inline constexpr bool host_is_little_endian =
    __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#else
#error "Unsupported host byte order"
#endif

inline constexpr uint16_t swap_bytes_16(uint16_t val)noexcept{
    return (val<<8) | (val>>8);
}
inline constexpr uint32_t swap_bytes_32(uint32_t val)noexcept{
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}
// ntohs /ntohl

inline constexpr uint16_t ntohs(uint16_t val) noexcept{
    if constexpr (host_is_little_endian){
        return swap_bytes_16(val);
    }else{
        return val;
    }
}

inline constexpr uint32_t ntohl(uint32_t val) noexcept{
    if constexpr (host_is_little_endian){
        return swap_bytes_32(val);
    }else{
        return val;
    }
}

}