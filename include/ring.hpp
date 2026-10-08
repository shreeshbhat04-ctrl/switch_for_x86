#pragma once
#include <new>
#include <atomic>
#include <memory>
#include <concepts>
#include <bit>
#include <utility>
#include <optional>

template <typename T,size_t capacity>
class alignas(std::hardware_constructive_interference_size)Spscringbuffer{
 static_assert(capacity>=2,"capacity must be at least 2");
 static_assert(std::has_single_bit(capacity),"capac must be power of two");
 public:
    using value_type=T;
    Spscringbuffer():storage_(std::make_unique<storagetype[]>(capacity)){}
    ~Spscringbuffer(){
        T dummy;
        while(pop(dummy));
    }
    //disable copying
    Spscringbuffer(const Spscringbuffer&)=delete;
    Spscringbuffer& operator=(const Spscringbuffer&) = delete;
    // enable moving
    Spscringbuffer(Spscringbuffer&&) noexcept = default;
    Spscringbuffer& operator=(Spscringbuffer&&) noexcept = default;

template<typename... Args>
    bool emplace(Args&... args){
        const size_t write_idx=write_index_.load(std::memory_order_relaxed);
        if(write_idx - cached_read_index_==capacity){
           cached_read_index_=read_index_.load(std::memory_order_acquire);
           if(write_idx-cached_read_index_==capacity){
              return false;
           }
        }
        T* ptr=reinterpret_cast<T*>(&storage_[write_idx & index_mask]);
        std::construct_at(ptr,std::forward<Args>(args)...);
        write_index_.store(write_idx+1,std::memory_order_release);
        return true;
    }
   bool push(T&& item){
    return emplace(std::move(item));
   }
   bool push(const T& item){
    return emplace(item);
   }
    bool pop(T& item){
        const size_t read_idx=read_index_.load(std::memory_order_relaxed);
        if(read_idx==cached_write_index_){
            cached_write_index_=write_index_.load(std::memory_order_acquire);
            if(read_idx==cached_write_index_){
            return false;
            }
        }
        T* ptr=reinterpret_cast<T*>(&storage_[read_idx & index_mask]);
        item=std::move(*ptr);
        std::destroy_at(ptr);
        read_index_.store(read_idx+1,std::memory_order_relaxed);
        return true;
    }
[[nodiscard]]bool empty()const noexcept{
    return read_index_.load(std::memory_order_relaxed)==write_index_.load(std::memory_order_relaxed);
}
[[nodiscard]]size_t size()const noexcept{
    size_t write=write_index_.load(std::memory_order_relaxed);
    size_t read=read_index_.load(std::memory_order_relaxed);
    return (write>=read)?(write-read):(capacity-(read-write));
}


    private:
    static constexpr size_t index_mask=capacity-1;
    static constexpr size_t cachelinesz=std::hardware_destructive_interference_size;

    //use aligned storage 
    struct alignas(alignof(T)) storagetype{
        alignas(alignof(T))std::byte data[sizeof(T)];
    };
    std::unique_ptr<storagetype[]>storage;
    alignas(cachelinesz) std::atomic<size_t>read_index_{0};
    alignas(cachelinesz) std::atomic<size_t>write_index_{0};

    alignas(cachelinesz) size_t cached_read_index_{0};
    alignas(cachelinesz) size_t cached_write_index_{0};
};