#pragma once
#include <new>
#include <atomic>
#include <array>
#include <cstddef>
#include <utility>

template <typename T,size_t capacity>
class Spscringbuffer{
 static_assert(capacity>=2,"capacity must be at least 2");
 static_assert((capacity & (capacity - 1)) == 0,"capacity must be power of two");
 public:
    using value_type=T;
    Spscringbuffer() = default;
    ~Spscringbuffer(){
        const size_t read_idx = read_index_.load(std::memory_order_relaxed);
        const size_t write_idx = write_index_.load(std::memory_order_relaxed);
        for (size_t index = read_idx; index != write_idx; ++index) {
            T* ptr = reinterpret_cast<T*>(&storage_[index & index_mask]);
            ptr->~T();
        }
    }
    //disable copying
    Spscringbuffer(const Spscringbuffer&)=delete;
    Spscringbuffer& operator=(const Spscringbuffer&) = delete;
    // enable moving
    Spscringbuffer(Spscringbuffer&&) noexcept = delete;
    Spscringbuffer& operator=(Spscringbuffer&&) noexcept = delete;

template<typename... Args>
    bool emplace(Args&&... args){
        const size_t write_idx=write_index_.load(std::memory_order_relaxed);
        if(write_idx - cached_read_index_==capacity){
           cached_read_index_=read_index_.load(std::memory_order_acquire);
           if(write_idx-cached_read_index_==capacity){
              return false;
           }
        }
        T* ptr=reinterpret_cast<T*>(&storage_[write_idx & index_mask]);
        ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
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
        ptr->~T();
        read_index_.store(read_idx+1,std::memory_order_release);
        return true;
    }
[[nodiscard]]bool empty()const noexcept{
    return read_index_.load(std::memory_order_relaxed)==write_index_.load(std::memory_order_relaxed);
}
[[nodiscard]]size_t size()const noexcept{
    size_t write=write_index_.load(std::memory_order_relaxed);
    size_t read=read_index_.load(std::memory_order_relaxed);
    return write - read;
}


    private:
    static constexpr size_t index_mask=capacity-1;
    static constexpr size_t cachelinesz=64;

    //use aligned storage 
    struct alignas(alignof(T)) storagetype{
        alignas(alignof(T))std::byte data[sizeof(T)];
    };
    alignas(cachelinesz) std::atomic<size_t>read_index_{0};
    size_t cached_write_index_{0};
    alignas(cachelinesz) std::atomic<size_t>write_index_{0};
    size_t cached_read_index_{0};
    std::array<storagetype, capacity> storage_{};
};