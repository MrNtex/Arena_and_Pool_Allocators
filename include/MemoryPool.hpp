#pragma once
#include <memory>
#include <new>
#include <type_traits>
#include <algorithm>
#include <stdlib.h>
#include <cstdlib>

inline void* malligned_alloc(std::size_t alignment, std::size_t size) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

inline void malligned_free(void* ptr) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

inline std::size_t align_size(std::size_t size, std::size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

template <std::size_t TotalSize>
struct Arena {
    std::byte* buffer;
    void* current;
    std::size_t space_left;

    Arena() : space_left(TotalSize) {
        std::size_t alignment = alignof(std::max_align_t);
        std::size_t size = align_size(TotalSize, alignment);

        buffer = static_cast<std::byte*>(malligned_alloc(alignment, size));
        if (!buffer) throw std::bad_alloc();
        current = buffer;
    }

    ~Arena() { malligned_free(buffer); }
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        static_assert(std::is_trivially_destructible_v<T>, "Arena only supports trivially destructible types.");
        if (std::align(alignof(T), sizeof(T), current, space_left)) {
            T* obj = new (current) T(std::forward<Args>(args)...);
            current = static_cast<std::byte*>(current) + sizeof(T);
            space_left -= sizeof(T);
            return obj;
        }
        throw std::bad_alloc();
    }

    void* allocate_raw(std::size_t size, std::size_t alignment) {
        if (std::align(alignment, size, current, space_left)) {
            void* result = current;
            current = static_cast<std::byte*>(current) + size;
            space_left -= size;
            return result;
        }
        throw std::bad_alloc();
    }

    void reset() noexcept {
        current = buffer;
        space_left = TotalSize;
    }
};

template<typename T, std::size_t PoolSize = 1024>
class TypedPoolAllocator {
    struct Block { Block* next; };
    static constexpr std::size_t BlockSize = std::max(sizeof(T), sizeof(Block));
    static constexpr std::size_t BlockAlignment = std::max(alignof(T), alignof(Block));

    std::byte* buffer;
    Block* head{ nullptr };

public:
    TypedPoolAllocator() {
        std::size_t total_bytes = align_size(BlockSize * PoolSize, BlockAlignment);
        buffer = static_cast<std::byte*>(malligned_alloc(BlockAlignment, total_bytes));
        if (!buffer) throw std::bad_alloc();

        for (std::size_t i = 0; i < PoolSize; ++i) {
            auto* node = new (buffer + i * BlockSize) Block{ head };
            head = node;
        }
    }

    ~TypedPoolAllocator() { malligned_free(buffer); }

    TypedPoolAllocator(const TypedPoolAllocator&) = delete;
    TypedPoolAllocator& operator=(const TypedPoolAllocator&) = delete;

    template<typename... Args>
    T* allocate(Args&&... args) {
        if (!head) throw std::bad_alloc();
        Block* free_block = head;
        head = head->next;
        return new (free_block) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        Block* node = new (ptr) Block{ head };
        head = node;
    }
};

template<std::size_t TargetBlockSize, std::size_t Count>
class SizeClassPool {
private:
    static constexpr std::size_t Alignment = std::min(alignof(std::max_align_t), TargetBlockSize);
    union Block {
        Block* next;
        alignas(Alignment) std::byte data[TargetBlockSize];
    };
    static constexpr std::size_t ActualBlockSize = sizeof(Block);

    Block* buffer;
    Block* head{ nullptr };

public:
    SizeClassPool() {
        std::size_t total_bytes = align_size(ActualBlockSize * Count, alignof(Block));
        buffer = static_cast<Block*>(malligned_alloc(alignof(Block), total_bytes));
        if (!buffer) throw std::bad_alloc();

        for (std::size_t i = 0; i < Count; ++i) {
            auto* node = new (&buffer[i]) Block{ head };
            head = node;
        }
    }

    ~SizeClassPool() { malligned_free(buffer); }

    SizeClassPool(const SizeClassPool&) = delete;
    SizeClassPool& operator=(const SizeClassPool&) = delete;

    void* allocate() {
        if (!head) throw std::bad_alloc();
        Block* free_block = head;
        head = head->next;
        return free_block->data;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;
        Block* node = new (ptr) Block{ head };
        head = node;
    }
};

template<std::size_t PoolSize>
class MemoryPool {
private:
    SizeClassPool<8, PoolSize> pool_8;
    SizeClassPool<16, PoolSize> pool_16;
    SizeClassPool<64, PoolSize> pool_64;
    SizeClassPool<256, PoolSize> pool_256;

public:
    void* allocate(std::size_t size) {
        if (size <= 8) return pool_8.allocate();
        if (size <= 16) return pool_16.allocate();
        if (size <= 64) return pool_64.allocate();
        if (size <= 256) return pool_256.allocate();
        return ::operator new(size);
    }

    void deallocate(void* ptr, std::size_t size) {
        if (size <= 8) pool_8.deallocate(ptr);
        else if (size <= 16) pool_16.deallocate(ptr);
        else if (size <= 64) pool_64.deallocate(ptr);
        else if (size <= 256) pool_256.deallocate(ptr);
        else ::operator delete(ptr);
    }
};